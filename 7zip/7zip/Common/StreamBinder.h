// StreamBinder.h

#pragma once

#ifndef __STREAMBINDER_H
#define __STREAMBINDER_H

#include "../IStream.h"
#include "../../OS/Synchronization.h"

class CStreamBinder
{
  NOS::NSynchronization::CManualResetEvent *_allBytesAreWritenEvent;
  NOS::NSynchronization::CManualResetEvent *_thereAreBytesToReadEvent;
  NOS::NSynchronization::CManualResetEvent *_readStreamIsClosedEvent;
  UINT32 _bufferSize;
  const void *_buffer;
public:
  // bool ReadingWasClosed;
  UINT64 ProcessedSize;
  CStreamBinder():
    _allBytesAreWritenEvent(NULL), 
    _thereAreBytesToReadEvent(NULL),
    _readStreamIsClosedEvent(NULL)
    {}
  ~CStreamBinder();
  void CreateEvents();

  void CreateStreams(ISequentialInStream **inStream, 
      ISequentialOutStream **outStream);
  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  void CloseRead();

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
  void CloseWrite();
  void ReInit();
};

#endif

