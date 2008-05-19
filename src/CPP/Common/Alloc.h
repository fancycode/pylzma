// Common/Alloc.h

#ifndef __COMMON_ALLOC_H
#define __COMMON_ALLOC_H

#include <stddef.h>

void *MyAlloc(size_t size) throw();
void MyFree(void *address) throw();

#ifdef _WIN32

bool SetLargePageSize();

void *MidAlloc(size_t size) throw();
void MidFree(void *address) throw();
#ifdef MEM_LARGE_PAGES
void *BigAlloc(size_t size) throw();
void BigFree(void *address) throw();
#else
#define BigAlloc(size) MidAlloc(size)
#define BigFree(address) MidFree(address)
#endif

#else

#define MidAlloc(size) MyAlloc(size)
#define MidFree(address) MyFree(address)
#define BigAlloc(size) MyAlloc(size)
#define BigFree(address) MyFree(address)

#endif

#endif
