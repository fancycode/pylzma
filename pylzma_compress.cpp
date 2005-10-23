/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2005 by Joachim Bauch, mail@joachim-bauch.de
 * 7-Zip Copyright (C) 1999-2005 Igor Pavlov
 * LZMA SDK Copyright (C) 1999-2005 Igor Pavlov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * $Id$
 *
 */

#include <Python.h>
#include <7zip/Common/MyWindows.h>
#include <7zip/7zip/IStream.h>
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>

#include "pylzma.h"
#include "pylzma_streams.h"

int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
    int literalContextBits, int literalPosBits, int algorithm, int fastBytes, int eos,
    int multithreading, const char *matchfinder)
{
    wchar_t tmp[10];
    unsigned int i;
    encoder->SetWriteEndMarkerMode(eos ? true : false);
    
    PROPID propIDs[] = 
    {
        NCoderPropID::kDictionarySize,
        NCoderPropID::kPosStateBits,
        NCoderPropID::kLitContextBits,
        NCoderPropID::kLitPosBits,
        NCoderPropID::kAlgorithm,
        NCoderPropID::kNumFastBytes,
        NCoderPropID::kMatchFinder
#ifdef COMPRESS_MF_MT
        , NCoderPropID::kMultiThread
#endif
    };
    const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
    PROPVARIANT props[kNumProps];
    // NCoderProp::kDictionarySize;
    props[0].vt = VT_UI4;
    props[0].ulVal = 1 << dictionary;
    // NCoderProp::kPosStateBits;
    props[1].vt = VT_UI4;
    props[1].ulVal = posBits;
    // NCoderProp::kLitContextBits;
    props[2].vt = VT_UI4;
    props[2].ulVal = literalContextBits;
    // NCoderProp::kLitPosBits;
    props[3].vt = VT_UI4;
    props[3].ulVal = literalPosBits;
    // NCoderProp::kAlgorithm;
    props[4].vt = VT_UI4;
    props[4].ulVal = algorithm;
    // NCoderProp::kNumFastBytes;
    props[5].vt = VT_UI4;
    props[5].ulVal = fastBytes;
    // NCoderProp::kMatchFinder;
    if (strlen(matchfinder) > (sizeof(tmp)/2)-1)
        return 1;
    
    props[6].vt = VT_BSTR;
    for (i=0; i<strlen(matchfinder); i++)
        tmp[i] = matchfinder[i];
    tmp[i] = 0;
    props[6].bstrVal = (BSTR)(const wchar_t *)&tmp;
#ifdef COMPRESS_MF_MT
    // NCoderPropID::kMultiThread
    props[7].vt = VT_BOOL;
    props[7].boolVal = (multithreading != 0) ? VARIANT_TRUE : VARIANT_FALSE;
#endif
    
    return encoder->SetCoderProperties(propIDs, props, kNumProps);
}

#ifdef __cplusplus
extern "C"
#endif
const char doc_compress[] = \
    "compress(string, dictionary=23, fastBytes=128, literalContextBits=3, literalPosBits=0, posBits=2, algorithm=2, eos=1, multithreading=1, matchfinder='bt4') -- Compress the data in string using the given parameters, returning a string containing the compressed data.";

#ifdef __cplusplus
extern "C" {
#endif

PyObject *pylzma_compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *result = NULL;
    NCompress::NLZMA::CEncoder *encoder = NULL;
    CInStream *inStream = NULL;
    COutStream *outStream = NULL;
    int res;    
    // possible keywords for this function
    static char *kwlist[] = {"data", "dictionary", "fastBytes", "literalContextBits",
                             "literalPosBits", "posBits", "algorithm", "eos", "multithreading", "matchfinder", NULL};
    int dictionary = 23;         // [0,28], default 23 (8MB)
    int fastBytes = 128;         // [5,255], default 128
    int literalContextBits = 3;  // [0,8], default 3
    int literalPosBits = 0;      // [0,4], default 0
    int posBits = 2;             // [0,4], default 2
    int eos = 1;                 // write "end of stream" marker?
    int multithreading = 1;      // use multithreading if available?
    char *matchfinder = "bt4";   // matchfinder algorithm
    int algorithm = 2;
    char *data;
    int length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|iiiiiiiis", kwlist, &data, &length, &dictionary, &fastBytes,
                                                                  &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos, &multithreading, &matchfinder))
        return NULL;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    CHECK_RANGE(algorithm,          0,   2, "algorithm must be between 0 and 2");
    
    encoder = new NCompress::NLZMA::CEncoder();
    CHECK_NULL(encoder);

    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos, multithreading, matchfinder) != 0))
    {
        PyErr_SetString(PyExc_TypeError, "can't set coder properties");
        goto exit;
    }
    
    inStream = new CInStream((BYTE *)data, length);
    CHECK_NULL(inStream);
    outStream = new COutStream();
    CHECK_NULL(outStream);
    
    Py_BEGIN_ALLOW_THREADS
    encoder->SetStreams(inStream, outStream, 0, 0);
    encoder->WriteCoderProperties(outStream);
    while(true)
    {
        UInt64 processedInSize;
        UInt64 processedOutSize;
        Int32 finished;
        if ((res = encoder->CodeOneBlock(&processedInSize, &processedOutSize, &finished)) != S_OK)
        {
            PyErr_Format(PyExc_TypeError, "Error during compressing: %d", res);
            goto exit;
        }
        if (finished != 0)
            break;
    }
    Py_END_ALLOW_THREADS
    
    result = PyString_FromStringAndSize((const char *)outStream->getData(), outStream->getLength());
    
exit:
    DELETE_AND_NULL(encoder);
    DELETE_AND_NULL(inStream);
    
    return result;
}

#ifdef __cplusplus
}
#endif
