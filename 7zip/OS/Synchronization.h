// Windows/Synchronization.h

#ifndef __WINDOWS_SYNCHRONIZATION_H
#define __WINDOWS_SYNCHRONIZATION_H

#include "Defs.h"
#include "Handle.h"

namespace NWindows {
namespace NSynchronization {

#ifdef _WIN32
class CObject: public CHandle
{
public:
  bool Lock(DWORD timeoutInterval = INFINITE)
    { return (::WaitForSingleObject(_handle, timeoutInterval) == WAIT_OBJECT_0); }
};

class CBaseEvent: public CObject
{
public:
  bool Create(bool manualReset, bool initiallyOwn, LPCTSTR name = NULL,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL)
  {
    _handle = ::CreateEvent(securityAttributes, BoolToBOOL(manualReset),
        BoolToBOOL(initiallyOwn), name);
    return (_handle != 0);
  }

  bool Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenEvent(desiredAccess, BoolToBOOL(inheritHandle), name);
    return (_handle != 0);
  }

  bool Set() { return BOOLToBool(::SetEvent(_handle)); }
  bool Pulse() { return BOOLToBool(::PulseEvent(_handle)); }
  bool Reset() { return BOOLToBool(::ResetEvent(_handle)); }
};

class CEvent: public CBaseEvent
{
public:
  CEvent() {};
  CEvent(bool manualReset, bool initiallyOwn, 
      LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES securityAttributes = NULL);
};

class CManualResetEvent: public CEvent
{
public:
  CManualResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(true, initiallyOwn, name, securityAttributes) {};
};

class CAutoResetEvent: public CEvent
{
public:
  CAutoResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(false, initiallyOwn, name, securityAttributes) {};
};

class CMutex: public CObject
{
public:
  bool Create(bool initiallyOwn, LPCTSTR name = NULL,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL)
  {
    _handle = ::CreateMutex(securityAttributes, BoolToBOOL(initiallyOwn), name);
    return (_handle != 0);
  }
  bool Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenMutex(desiredAccess, BoolToBOOL(inheritHandle), name);
    return (_handle != 0);
  }
  bool Release() { return BOOLToBool(::ReleaseMutex(_handle)); }
};

class CMutexLock
{
  CMutex &_object;
public:
  CMutexLock(CMutex &object): _object(object) { _object.Lock(); } 
  ~CMutexLock() { _object.Release(); }
};

class CCriticalSection
{
  CRITICAL_SECTION _object;
  // void Initialize() { ::InitializeCriticalSection(&_object); }
  // void Delete() { ::DeleteCriticalSection(&_object); }
public:
  CCriticalSection() { ::InitializeCriticalSection(&_object); }
  ~CCriticalSection() { ::DeleteCriticalSection(&_object); }
  void Enter() { ::EnterCriticalSection(&_object); }
  void Leave() { ::LeaveCriticalSection(&_object); }
};

class CCriticalSectionLock
{
  CCriticalSection &_object;
  void Unlock()  { _object.Leave(); }
public:
  CCriticalSectionLock(CCriticalSection &object): _object(object) 
    {_object.Enter(); } 
  ~CCriticalSectionLock() { Unlock(); }
};

#endif

#ifndef _WIN32

// only implement classes that are actually used...

#include <pthread.h>
#include "../Common/MyWindows.h"

#define MAXIMUM_WAIT_OBJECTS   0x100
#define WAIT_OBJECT_0          0
#define WAIT_ABANDONED_0       (WAIT_OBJECT_0 | MAXIMUM_WAIT_OBJECTS)
#define WAIT_TIMEOUT           0xFFFFFFFE
#define INFINITE               0xFFFFFFFF
#define HANDLE NWindows::NSynchronization::CEvent

class CEvent: public CHandle
{
public:
  CEvent()
  {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&handle, NULL);
  };
  
  ~CEvent()
  {
    pthread_cond_destroy(&handle);
    pthread_mutex_destroy(&mutex);
  };

  bool Lock(DWORD timeoutInterval = INFINITE)
  {
    struct timespec wait;
    if (timeoutInterval == INFINITE)
      return (pthread_cond_wait(&handle, &mutex) == 0);
    
    wait.tv_sec = timeoutInterval / 1000;
    wait.tv_nsec = (timeoutInterval % 1000) * 1000000;
    return (pthread_cond_timedwait(&handle, &mutex, &wait) == 0);
  };
  
  bool Set() { return was_set = true; (pthread_cond_signal(&handle) == 0); };
  bool Reset() { was_set = false; return true; };
  bool IsSet() { return was_set; }
  bool IsAutoReset() { return false; }
  
protected:
  pthread_cond_t handle;
  pthread_mutex_t mutex;
  bool was_set;
};

class CManualResetEvent: public CEvent
{
public:
};

class CAutoResetEvent: public CEvent
{
public:
  bool Reset() { /* this is a noop */ return true; };
  bool IsAutoReset() { return true; }
};

#endif

}}

#ifndef _WIN32
__stdcall DWORD WaitForMultipleObjects(DWORD nCount, const NWindows::NSynchronization::CEvent* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
#endif

#endif
