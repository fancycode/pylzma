// Windows/Thread.h

#ifndef __WINDOWS_THREAD_H
#define __WINDOWS_THREAD_H

#include "Handle.h"
#include "Defs.h"

#ifdef _WIN32
#define CThread CWindowsThread
#else
#define CThread CPosixThread
#endif

namespace NWindows {

#ifdef _WIN32
class CWindowsThread: public CHandle
{
  bool IsOpen() const { return _handle != 0; }
public:
  bool Create(LPSECURITY_ATTRIBUTES threadAttributes, 
      SIZE_T stackSize, LPTHREAD_START_ROUTINE startAddress,
      LPVOID parameter, DWORD creationFlags, LPDWORD threadId)
  {
    _handle = ::CreateThread(threadAttributes, stackSize, startAddress,
        parameter, creationFlags, threadId);
    return (_handle != NULL);
  }
  bool Create(LPTHREAD_START_ROUTINE startAddress, LPVOID parameter)
  {
    DWORD threadId;
    return Create(NULL, 0, startAddress, parameter, 0, &threadId);
  }
  
  DWORD Resume()
    { return ::ResumeThread(_handle); }
  DWORD Suspend()
    { return ::SuspendThread(_handle); }
  bool Terminate(DWORD exitCode)
    { return BOOLToBool(::TerminateThread(_handle, exitCode)); }
  
  int GetPriority()
    { return ::GetThreadPriority(_handle); }
  bool SetPriority(int priority)
    { return BOOLToBool(::SetThreadPriority(_handle, priority)); }

  bool Wait() 
  { 
    if (!IsOpen())
      return true;
    return (::WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0); 
  }

};
#endif

#ifndef _WIN32

#include <pthread.h>

class CPosixThread: public CHandle
{
  bool IsOpen() const { return thread != NULL; }
public:
  ~CPosixThread()
  {
    if (IsOpen())
      Terminate(0);
  };
  
  bool Create(LPTHREAD_START_ROUTINE startAddress, LPVOID parameter)
  {
    thread = NULL;
    threadProc = startAddress;
    threadParameter = parameter;
    return (pthread_create(&thread, NULL, MyThreadProc, this) == 0);
  }
  
  DWORD Resume()
    { return 0; }
  DWORD Suspend()
    { return 0; }
  bool Terminate(DWORD exitCode)
  {
    if (!IsOpen())
      return true;
    
    // make sure the thread is running
    Resume();
    if (pthread_cancel(thread) != 0)
      return false;
    
    thread = NULL;
    return true;
  }
  
  int GetPriority()
    { return 0; }
  bool SetPriority(int priority)
    { return false; }

  bool Wait() 
  { 
    if (!IsOpen())
      return true;
    
    return (pthread_join(thread, NULL) == 0); 
  }
private:
  pthread_t thread;
  LPTHREAD_START_ROUTINE threadProc;
  LPVOID threadParameter;

  static void *MyThreadProc(void *klass)
  {
    CPosixThread *thread = (CPosixThread *)(klass);
    thread->threadProc(thread->threadParameter);
    return NULL;
  }
};
#endif

}

#endif
