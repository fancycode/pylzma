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
typedef const char *LPCCH, *PCSTR, *LPCSTR;
typedef unsigned short WCHAR, *PWCHAR, *LPWCH, *PWCH, *NWPSTR, *LPWSTR, *PWSTR;
typedef const unsigned short *LPCWCH, *PCWCH, *LPCWSTR, *PCWSTR;
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
#endif

#endif
