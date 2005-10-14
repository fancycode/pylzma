/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2005 by Joachim Bauch, mail@joachim-bauch.de
 * 7-Zip Copyright (C) 1999-2005 Igor Pavlov
 * LZMA SDK Copyright (C) 1999-2005 Igor Pavlov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * $Id$
 *
 */

#include <stdio.h>
#include "Platform.h"
#include "pylzma_encoder.h"
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>
#include <7zip/7zip/Compress/LZ/LZInWindow.h>

namespace NCompress {
namespace NLZMA {

HRESULT CPYLZMAEncoder::CodeOneBlock(UINT64 *inSize, UINT64 *outSize, INT32 *finished, bool flush)
{
  if (_inStream != 0)
  {
    RINOK(_matchFinder->Init(_inStream));
    _inStream = 0;
    state = 0;
  }
  
  *finished = 1;
  if (state > 0 && state < 4)
      _matchFinder->ResetStreamEndReached();
  switch (state)
  {
    case 0: goto state0;
    case 1: goto state1;
    case 2: goto state2;
    case 3: goto state3;
    case 4: goto state4;
  }

state0:  
  if (_finished)
  {
    return S_OK;
  }
  _finished = true;

  progressPosValuePrev = nowPos64;
  if (nowPos64 == 0)
  {
state1:
    if (_matchFinder->GetNumAvailableBytes() == 0)
    {
      state = 1;
      return S_OK;
    }
    ReadMatchDistances();
    UINT32 posState = UINT32(nowPos64) & _posStateMask;
    _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceLiteralIndex);
    _state.UpdateChar();
    BYTE curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
    _literalEncoder.Encode(&_rangeEncoder, UINT32(nowPos64), _previousByte, 
      false, 0, curByte);
    _previousByte = curByte;
    _additionalOffset--;
    nowPos64++;
  }
state2:
  if (_matchFinder->GetNumAvailableBytes() == 0)
  {
    state = 2;
    return S_OK;
  }
  while(true)
  {
state4:
    posState = UINT32(nowPos64) & _posStateMask;

    if (_fastMode)
      len = GetOptimumFast(pos, UINT32(nowPos64));
    else
      len = GetOptimum(pos, UINT32(nowPos64));

    if(len == 1 && pos == (UINT32)(-1))
    {
      _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceLiteralIndex);
      _state.UpdateChar();
      BYTE matchByte = 0;
      if(_peviousIsMatch)
        matchByte = _matchFinder->GetIndexByte(0 - _repDistances[0] - 1 - _additionalOffset);
      BYTE curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
      _literalEncoder.Encode(&_rangeEncoder, UINT32(nowPos64), _previousByte, _peviousIsMatch, 
          matchByte, curByte);
      _previousByte = curByte;
      _peviousIsMatch = false;
    }
    else
    {
      _peviousIsMatch = true;
      _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceMatchIndex);
      if(pos < kNumRepDistances)
      {
        _matchChoiceEncoders[_state.Index].Encode(&_rangeEncoder, kMatchChoiceRepetitionIndex);
        if(pos == 0)
        {
          _matchRepChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 0);
          if(len == 1)
            _matchRepShortChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, 0);
          else
            _matchRepShortChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, 1);
        }
        else
        {
          _matchRepChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 1);
          if (pos == 1)
            _matchRep1ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 0);
          else
          {
            _matchRep1ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 1);
            _matchRep2ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, pos - 2);
          }
        }
        if (len == 1)
          _state.UpdateShortRep();
        else
        {
          _repMatchLenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState);
          _state.UpdateRep();
        }

        UINT32 distance = _repDistances[pos];
        if (pos != 0)
        {
          for(UINT32 i = pos; i >= 1; i--)
            _repDistances[i] = _repDistances[i - 1];
          _repDistances[0] = distance;
        }
      }
      else
      {
        _matchChoiceEncoders[_state.Index].Encode(&_rangeEncoder, kMatchChoiceDistanceIndex);
        _state.UpdateMatch();
        _lenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState);
        pos -= kNumRepDistances;
        UINT32 posSlot = GetPosSlot(pos);
        UINT32 lenToPosState = GetLenToPosState(len);
        _posSlotEncoder[lenToPosState].Encode(&_rangeEncoder, posSlot);
        
        if (posSlot >= kStartPosModelIndex)
        {
          UINT32 footerBits = ((posSlot >> 1) - 1);
          UINT32 posReduced = pos - ((2 | (posSlot & 1)) << footerBits);

          if (posSlot < kEndPosModelIndex)
            _posEncoders[posSlot - kStartPosModelIndex].Encode(&_rangeEncoder, posReduced);
          else
          {
            _rangeEncoder.EncodeDirectBits(posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
            _posAlignEncoder.Encode(&_rangeEncoder, posReduced & kAlignMask);
            if (!_fastMode)
              if (--_alignPriceCount == 0)
                FillAlignPrices();
          }
        }
        UINT32 distance = pos;
        for(UINT32 i = kNumRepDistances - 1; i >= 1; i--)
          _repDistances[i] = _repDistances[i - 1];
        _repDistances[0] = distance;
      }
      _previousByte = _matchFinder->GetIndexByte(len - 1 - _additionalOffset);
    }
    _additionalOffset -= len;
    nowPos64 += len;
    if (!_fastMode)
      if (nowPos64 - lastPosSlotFillingPos >= (1 << 9))
      {
        FillPosSlotPrices();
        FillDistancesPrices();
        lastPosSlotFillingPos = nowPos64;
      }
    if (_additionalOffset == 0)
    {
      *inSize = nowPos64;
      *outSize = _rangeEncoder.GetProcessedSize();
state3:
      if (_matchFinder->GetNumAvailableBytes() == 0)
      {
        state = 3;
        return S_OK;
      }
      if (nowPos64 - progressPosValuePrev >= (1 << 12))
      {
        _finished = false;
        *finished = 0;
        state = 0;
        return S_OK;
      }
    }
    state = 4;
  }
}

HRESULT CPYLZMAEncoder::FinishStream()
{
    _finished = true;
    _matchFinder->ReleaseStream();
    WriteEndMarker(UINT32(nowPos64) & _posStateMask);
    return Flush();
}

}}
