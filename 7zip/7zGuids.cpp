// DLLExports.cpp

// #include "StdAfx.h"

#ifdef WIN32
#  include <objbase.h>
#  include <initguid.h>
#endif

#include "../Platform.h"

#define INITGUID
#include "7zip/ICoder.h"
#include "7zip/IPassword.h"
#include "7zip/IProgress.h"
#include "7zip/Archive/IArchive.h"
#include "7zip/Compress/LZ/IMatchFinder.h"
