// Windows/Synchronization.cpp

#ifndef _WIN32
#include <time.h>
#include <stdio.h>
#endif

#include "StdAfx.h"

#include "Synchronization.h"

#ifdef _WIN32

namespace NWindows {
namespace NSynchronization {

CEvent::CEvent(bool manualReset, bool initiallyOwn, LPCTSTR name,
    LPSECURITY_ATTRIBUTES securityAttributes)
{
  if (!Create(manualReset, initiallyOwn, name, securityAttributes))
    throw "CreateEvent error";
}

}}

#endif

#ifndef _WIN32

// Return current time in milliseconds
static DWORD GetTickCount()
{
    struct timespec ts;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    return (DWORD)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}

__stdcall DWORD WaitForMultipleObjects(DWORD nCount, const NWindows::NSynchronization::CEvent* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
  DWORD end;
  NWindows::NSynchronization::CEvent event;

  printf("WaitForMultipleObjects\n");
  if (dwMilliseconds == INFINITE)
    end = INFINITE;
  else
    end = GetTickCount() + dwMilliseconds;
  
  while (true) {
    if (end != INFINITE && end < GetTickCount())
      break;
      
    if (bWaitAll) {
      bool allSet = true;
      for (DWORD i=0; i<nCount; i++) {
        event = lpHandles[i];
        if (!event.IsSet()) {
          allSet = false;
          break;
        }
      }
      
      if (allSet) {
        for (DWORD i=0; i<nCount; i++) {
          event = lpHandles[i];
          if (event.IsAutoReset())
            event.Reset();
        }
        
        printf("All set\n");
        return WAIT_OBJECT_0;
      }
    } else {
      for (DWORD i=0; i<nCount; i++) {
        event = lpHandles[i];
        if (event.IsSet()) {
          if (event.IsAutoReset())
            event.Reset();
        
          printf("Found: %d\n", i);
          return WAIT_OBJECT_0 + i;
        }
      }
    }
  }
  
  printf("Timeout\n");
  return WAIT_TIMEOUT;
}

#endif
