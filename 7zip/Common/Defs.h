// Common/Defs.h

#ifndef __COMMON_DEFS_H
#define __COMMON_DEFS_H

#include "../../Platform.h"

template <class T> inline T MyMin(T a, T b)
  {  return a < b ? a : b; }
template <class T> inline T MyMax(T a, T b)
  {  return a > b ? a : b; }

template <class T> inline int MyCompare(T a, T b)
  {  return a < b ? -1 : (a == b ? 0 : 1); }

inline int BoolToInt(bool value)
  { return (value ? 1: 0); }

inline bool IntToBool(int value)
  { return (value != 0); }

inline bool BOOLToBool(BOOL value)
  { return (value != FALSE); }

inline BOOL BoolToBOOL(bool value)
  { return (value ? TRUE: FALSE); }

inline VARIANT_BOOL BoolToVARIANT_BOOL(bool value)
  { return (value ? VARIANT_TRUE: VARIANT_FALSE); }

inline bool VARIANT_BOOLToBool(VARIANT_BOOL value)
  { return (value != VARIANT_FALSE); }

#endif
