/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004 by Joachim Bauch, mail@joachim-bauch.de
 * LZMA SDK Copyright (C) 1999-2004 Igor Pavlov
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
#include <Platform.h>
#include <7zip/7zip/IStream.h>
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>

#include "pylzma.h"
#include "pylzma_streams.h"

int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
    int literalContextBits, int literalPosBits, int algorithm, int fastBytes, int eos)
{
    encoder->SetWriteEndMarkerMode(eos ? true : false);
    
    PROPID propIDs[] = 
    {
        NCoderPropID::kDictionarySize,
        NCoderPropID::kPosStateBits,
        NCoderPropID::kLitContextBits,
        NCoderPropID::kLitPosBits,
        NCoderPropID::kAlgorithm,
        NCoderPropID::kNumFastBytes
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
    
    return encoder->SetCoderProperties(propIDs, props, kNumProps);
}

#ifdef __cplusplus
extern "C"
#endif
const char doc_compress[] = \
    "compress(string, dictionary=23, fastBytes=128, literalContextBits=3, literalPosBits=0, posBits=2, algorithm=2, eos=1) -- Compress the data in string using the given parameters, returning a string containing the compressed data.";

#ifdef __cplusplus
extern "C" {
#endif

PyObject *pylzma_compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *result = NULL;
    NCompress::NLZMA::CEncoder *encoder;
    CInStream *inStream = NULL;
    COutStream *outStream = NULL;
    int res;    
    // possible keywords for this function
    static char *kwlist[] = {"data", "dictionary", "fastBytes", "literalContextBits",
                             "literalPosBits", "posBits", "algorithm", "eos", NULL};
    int dictionary = 23;         // [0,28], default 23 (8MB)
    int fastBytes = 128;         // [5,255], default 128
    int literalContextBits = 3;  // [0,8], default 3
    int literalPosBits = 0;      // [0,4], default 0
    int posBits = 2;             // [0,4], default 2
    int eos = 1;                 // write "end of stream" marker?
    int algorithm = 2;
    char *data;
    int length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|iiiiiii", kwlist, &data, &length, &dictionary, &fastBytes,
                                                                &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos))
        return NULL;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    
    encoder = new NCompress::NLZMA::CEncoder();
    CHECK_NULL(encoder);

    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos) != 0))
    {
        PyErr_Format(PyExc_TypeError, "Can't set coder properties: %d", res);
        goto exit;
    }
    
    inStream = new CInStream((BYTE *)data, length);
    CHECK_NULL(inStream);
    outStream = new COutStream();
    CHECK_NULL(outStream);
    
    Py_BEGIN_ALLOW_THREADS
    encoder->SetStreams(inStream, outStream, 0, 0);
    encoder->WriteCoderProperties(outStream);
    encoder->CodeReal(inStream, outStream, NULL, 0, 0);
    Py_END_ALLOW_THREADS
    
    result = PyString_FromStringAndSize((const char *)outStream->getData(), outStream->getLength());
    
exit:
    DELETE_AND_NULL(encoder);
    DELETE_AND_NULL(inStream);
    DELETE_AND_NULL(outStream);
    
    return result;
}

#ifdef __cplusplus
}
#endif
