/*
 * includes header files depending on OS
 *
 * Taken from NSIS (http://nsis.sourceforge.net)
 * Copyright (c) 1999-2004 Nullsoft, Inc.
 *
 * $Id$
 *
 */

#ifndef ___PLATFORM__H___
#define ___PLATFORM__H___

#ifdef _WIN32
#  ifndef _WIN32_IE
#    define _WIN32_IE 0x0400
#  endif
#  include <Windows.h>
#else
#  include <string.h>
#  include <stdlib.h>
#  include <wchar.h>
#  include <wctype.h>

// Basic types
typedef unsigned char BYTE, *PBYTE, *LPBYTE;
typedef unsigned short WORD, *LPWORD;
typedef unsigned long DWORD, *LPDWORD;
typedef unsigned int UINT;
typedef unsigned int UINT32;
typedef int INT;
typedef int INT32;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef int BOOL;
typedef void VOID;
typedef void *LPVOID;
typedef char CHAR, *PCHAR, *LPCH, *PCH, *NPSTR, *LPSTR, *PSTR;
typedef wchar_t TCHAR, *BSTR, *LPCOLESTR;
typedef const char *LPCCH, *PCSTR, *LPCSTR, *LPCTSTR;
typedef wchar_t WCHAR, *PWCHAR, *LPWCH, *PWCH, *NWPSTR, *LPWSTR, *PWSTR;
typedef const wchar_t *LPCWCH, *PCWCH, *LPCWSTR, *PCWSTR;
typedef unsigned int UINT_PTR;
// basic stuff
typedef unsigned long HANDLE;
typedef unsigned long HKEY;
// some gdi
typedef unsigned long COLORREF;
typedef unsigned long HBRUSH;
// bool
#  define FALSE 0
#  define TRUE 1
// more
typedef WORD LANGID;
typedef WORD VARTYPE;

typedef short VARIANT_BOOL;
#define VARIANT_TRUE ((VARIANT_BOOL)-1)
#define VARIANT_FALSE ((VARIANT_BOOL)0)

#ifndef __cdecl
#define __cdecl
#endif

#ifndef FILETIME
/*
typedef struct STRUCT tagFILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;
*/
typedef UINT64 FILETIME;
#endif

#define MAX_PATH 260

#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

inline BSTR SysAllocString(LPCOLESTR src)
{
    BSTR result = (BSTR)malloc(wcslen(src)*sizeof(wchar_t));
    wcscpy(result, src);
    return result;
}

#define SysFreeString free
#define SysStringLen wcslen
#define SysStringByteLen wcslen

inline BSTR SysAllocStringByteLen(LPCOLESTR src, UINT32 len)
{
    BSTR result = (BSTR)malloc(len*sizeof(wchar_t));
    if (src != NULL)
        wcsncpy(result, src, min(wcslen(src), len));
    return result;
}

#define MyStringUpper(x) ((wchar_t)towupper((unsigned int)x))

#define AreFileApisANSI(x) 1
#define CP_ACP 0
#define CP_OEMCP 1

#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define NO_ERROR 0L

// XXX: implement this
int CloseHandle(HANDLE x)
{
    return 1;
}

BOOL SetFileAttributes(LPCTSTR a, DWORD b) { return TRUE; }
BOOL MoveFile(LPCTSTR a, LPCTSTR b) { return TRUE; }
BOOL RemoveDirectory(LPCTSTR a) { return TRUE; }
BOOL SetCurrentDirectory(LPCTSTR a) { return TRUE; }

BOOL FindNextChangeNotification(HANDLE x) { return TRUE; }
#define FindNextChangeNotificationW FindNextChangeNotification
HANDLE FindFirstChangeNotification(LPCTSTR name, BOOL sub, DWORD filter) { return 0; }
#define FindFirstChangeNotificationW FindFirstChangeNotification
DWORD GetCompressedFileSize(LPCTSTR x, LPDWORD y) { return 0; }
#define GetCompressedFileSizeW GetCompressedFileSize
#define TEXT(x) x

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR   cFileName[ MAX_PATH ];
    CHAR   cAlternateFileName[ 14 ];
#ifdef _MAC
    DWORD dwFileType;
    DWORD dwCreatorType;
    WORD  wFinderFlags;
#endif
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    WCHAR  cFileName[ MAX_PATH ];
    WCHAR  cAlternateFileName[ 14 ];
#ifdef _MAC
    DWORD dwFileType;
    DWORD dwCreatorType;
    WORD  wFinderFlags;
#endif
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

#define LPWIN32_FIND_DATA LPWIN32_FIND_DATAW

typedef void *LPSECURITY_ATTRIBUTES;
typedef size_t SIZE_T;
typedef void *LPTHREAD_START_ROUTINE;

HANDLE CreateThread(LPSECURITY_ATTRIBUTES threadAttributes,
      SIZE_T stackSize, LPTHREAD_START_ROUTINE startAddress,
      LPVOID parameter, DWORD creationFlags, LPDWORD threadId)
{
    return NULL;
}

DWORD ResumeThread(HANDLE x)
{
    return 0;
}

DWORD SuspendThread(HANDLE x)
{
    return 0;
}

BOOL TerminateThread(HANDLE x, DWORD exit)
{
    return TRUE;
}

int GetThreadPriority(HANDLE x)
{
    return 0;
}

BOOL SetThreadPriority(HANDLE x, int priority)
{
    return TRUE;
}

#define INFINITE -1
#define WAIT_OBJECT_0 0

DWORD WaitForSingleObject(DWORD x, DWORD timeout)
{
    return WAIT_OBJECT_0;
}

HANDLE CreateEvent(LPSECURITY_ATTRIBUTES securityAttributes, BOOL manualReset, BOOL initialOwn, LPCTSTR name)
{
    return 0;
}

HANDLE OpenEvent(DWORD desiredAccess, BOOL inheritHandle, LPCTSTR name)
{
    return 0;
}

BOOL SetEvent(HANDLE x)
{
    return TRUE;
}

BOOL PulseEvent(HANDLE x)
{
    return TRUE;
}

BOOL ResetEvent(HANDLE x)
{
    return TRUE;
}

HANDLE CreateMutex(LPSECURITY_ATTRIBUTES securityAttributes, BOOL initialOwn, LPCTSTR name)
{
    return 0;
}

HANDLE OpenMutex(DWORD x, BOOL inheritHandle, LPCTSTR name)
{
    return 0;
}

BOOL ReleaseMutex(HANDLE m)
{
    return FALSE;
}

typedef void *CRITICAL_SECTION;

void InitializeCriticalSection(CRITICAL_SECTION *cs)
{
}

void DeleteCriticalSection(CRITICAL_SECTION *cs)
{
}

void EnterCriticalSection(CRITICAL_SECTION *cs)
{
}

void LeaveCriticalSection(CRITICAL_SECTION *cs)
{
}

DWORD GetLastError(void)
{
    return 0;
}

void SetLastError(DWORD x)
{
}

HANDLE FindFirstFile(LPCTSTR name, LPWIN32_FIND_DATA x)
{
    return 0;
}

#define FindFirstFileW FindFirstFile

BOOL FindClose(LPWIN32_FIND_DATA x)
{
    return TRUE;
}

BOOL FindNextFile(HANDLE x, LPWIN32_FIND_DATA a)
{
    return TRUE;
}

#define FindNextFileW FindNextFile

#endif

#endif
