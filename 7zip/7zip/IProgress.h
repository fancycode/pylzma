// Interface/IProgress.h

// #pragma once

#ifndef __IPROGRESS_H
#define __IPROGRESS_H

// {23170F69-40C1-278A-0000-000000050000}
DEFINE_GUID(IID_IProgress, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000050000")
IProgress: public IUnknown
{
public:
  STDMETHOD(SetTotal)(UINT64 total) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *completeValue) PURE;
};

/*
// {23170F69-40C1-278A-0000-000000050002}
DEFINE_GUID(IID_IProgress2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x02);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000000050002")
IProgress2: public IUnknown
{
public:
  STDMETHOD(SetTotal)(const UINT64 *total) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *completeValue) PURE;
};
*/

#endif
