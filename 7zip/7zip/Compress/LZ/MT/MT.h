// MT_MF.h

#ifndef __MT_MF_H
#define __MT_MF_H

#include "../../../../Common/MyCom.h"

#include "../../../../OS/Thread.h"
#include "../../../../OS/Synchronization.h"

#include "../../../ICoder.h"
#include "../IMatchFinder.h"

const int kNumMTBlocks = 3;

class CMatchFinderMT: 
  public IMatchFinder,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(BYTE, GetIndexByte)(Int32 anIndex);
  STDMETHOD_(UINT32, GetMatchLen)(Int32 aIndex, UInt32 aBack, UInt32 aLimit);
  STDMETHOD_(UINT32, GetNumAvailableBytes)();
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances);
  STDMETHOD_(void, DummyLongestMatch)();

private:
public:
  CMyComPtr<IMatchFinder> m_MatchFinder;
  UINT32 m_MatchMaxLen;
  
  UINT32 m_BlockSize;
  // UINT32 m_BufferSize;
  UINT32 *m_Buffer;
  UINT32 *m_Buffers[kNumMTBlocks];

  bool m_NeedStart;
  UINT32 m_WriteBufferIndex;
  UINT32 m_ReadBufferIndex;

  NSynchronization::CAutoResetEvent m_StopWriting;
  NSynchronization::CAutoResetEvent m_WritingWasStopped;

  NSynchronization::CAutoResetEvent m_AskChangeBufferPos;
  NSynchronization::CAutoResetEvent m_CanChangeBufferPos;
  NSynchronization::CAutoResetEvent m_BufferPosWasChanged;

  NSynchronization::CManualResetEvent m_ExitEvent;
  // NSynchronization::CManualResetEvent m_NewStart;
  NSynchronization::CAutoResetEvent m_CanReadEvents[kNumMTBlocks];
  NSynchronization::CAutoResetEvent m_CanWriteEvents[kNumMTBlocks];
  UINT32 m_LimitPos[kNumMTBlocks];
  UINT32 m_NumAvailableBytes[kNumMTBlocks];

  UINT32 m_NumAvailableBytesCurrent;
  const BYTE *m_DataCurrentPos;
  
  UINT32 m_CurrentLimitPos;
  UINT32 m_CurrentPos;

  CThread m_Thread;
  // bool m_WriteWasClosed;
  UINT32 _multiThreadMult;
public:
  CMatchFinderMT();
  ~CMatchFinderMT();
  void Start();
  void FreeMem();
  HRESULT SetMatchFinder(IMatchFinder *aMatchFinder, 
      UINT32 multiThreadMult = 200);
};
 
#endif
