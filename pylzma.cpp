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
 */

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Python.h>

#ifdef _WIN32

// We can use the default 7-zip code for de-/compressing on Windows

#ifdef __cplusplus
}
#endif

#include <initguid.h>
#include <7zip/ICoder.h>
#include <Windows/PropVariant.h>
#include <7zip/Common/StreamObjects.h>
#include <7zip/Compress/LZMA/LZMADecoder.h>
#include <7zip/Compress/LZMA/LZMAEncoder.h>

using namespace NWindows;

static PyObject *perform_compression(char *data, int length, int dictionary, int fastBytes, int literalContextBits,
				     int literalPosBits, int posBits, int algorithm, int eos)
{
    HRESULT res;

    NCompress::NLZMA::CEncoder *encoderSpec = new NCompress::NLZMA::CEncoder;
    CMyComPtr<ICompressCoder> encoder = encoderSpec;
    
    CSequentialInStreamImp *inStreamSpec = new CSequentialInStreamImp;
    inStreamSpec->Init((const BYTE *)data, length);
    CMyComPtr<ISequentialInStream> inStream = inStreamSpec;

    CSequentialOutStreamImp *outStreamSpec = new CSequentialOutStreamImp;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    
    dictionary = 1 << dictionary;
    if (eos) encoderSpec->SetWriteEndMarkerMode(true);

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
    NWindows::NCOM::CPropVariant properties[kNumProps];

    properties[0] = UINT32(dictionary);
    properties[1] = UINT32(posBits);
    properties[2] = UINT32(literalContextBits);
   
    properties[3] = UINT32(literalPosBits);
    properties[4] = UINT32(algorithm);
    properties[5] = UINT32(fastBytes);

    if ((res = encoderSpec->SetCoderProperties(propIDs, properties, kNumProps)) != S_OK)
    {
        PyErr_Format(PyExc_ValueError, "Can't set coder properties: %d", res);
        return NULL;
    }

    encoderSpec->WriteCoderProperties(outStream);

    UINT64 fileSize;
    if (eos)
        fileSize = (UINT64)(INT64)-1;
    else
        fileSize = length;
    
    outStream->Write(&fileSize, sizeof(fileSize), 0);
    UINT64 longLength = length;
    if ((res = encoder->Code(inStream, outStream, &longLength, 0, 0)) != S_OK)
    {
        PyErr_Format(PyExc_ValueError, "Encoder error: %d", res);
        return NULL;
    }
    
    return PyString_FromStringAndSize((const char *)(const unsigned char *)outStreamSpec->GetBuffer(), outStreamSpec->GetSize());
}

static PyObject *perform_decompression(char *data, int length)
{
    HRESULT res;
    
    NCompress::NLZMA::CDecoder *decoderSpec = new NCompress::NLZMA::CDecoder;
    CMyComPtr<ICompressCoder> decoder = decoderSpec;
    
    CSequentialInStreamImp *inStreamSpec = new CSequentialInStreamImp;
    inStreamSpec->Init((const BYTE *)data, length);
    CMyComPtr<ISequentialInStream> inStream = inStreamSpec;

    if ((res = decoderSpec->SetDecoderProperties(inStream)) != S_OK)
    {
        PyErr_Format(PyExc_ValueError, "SetDecoderProperties error: %d", res);
        return NULL;
    }
    
    UINT32 processedSize;
    UINT64 fileSize;
    if ((res = inStream->Read(&fileSize, sizeof(fileSize), &processedSize)) != S_OK)
    {
        PyErr_Format(PyExc_ValueError, "ReadDecoderFileSize error: %d", res);
        return NULL;
    }   
    
    CSequentialOutStreamImp *outStreamSpec = new CSequentialOutStreamImp;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    
    if ((res = decoder->Code(inStream, outStream, 0, &fileSize, 0)) != S_OK)
    {
        PyErr_Format(PyExc_ValueError, "Decoder error: %d", res);
        return NULL;
    }   
    
    return PyString_FromStringAndSize((const char *)(const unsigned char *)outStreamSpec->GetBuffer(), outStreamSpec->GetSize());
}

#ifdef __cplusplus
    extern "C" {
#endif

#else

// Special LZMA handling on Linux

#include <Source/LzmaDecode.h>

static PyObject *perform_compression(char *data, int length, int dictionary, int fastBytes, int literalContextBits,
				     int literalPosBits, int posBits, int algorithm, int eos)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented on this platform.");
    return NULL;
}

static PyObject *perform_decompression(char *data, int length)
{
    PyObject *result = NULL;
    unsigned int compressedSize, outSize, outSizeProcessed, lzmaInternalSize;
    void *inStream, *outStream=NULL, *lzmaInternalData=NULL;
    unsigned char properties[5];
    unsigned char prop0;
    int ii;
    int lc, lp, pb;
    int res;
    int pos=0;
    
    memcpy(&properties, data, sizeof(properties));
    pos += sizeof(properties);
    outSize = 0;
    for (int ii = 0; ii < 4; ii++)
    {
        unsigned char b;
        memcpy(&b, &data[pos], sizeof(b));
        pos += sizeof(b);
        if (pos > length)
        {
            PyErr_SetString(PyExc_ValueError, "end of stream");
            goto exit;
        }
        outSize += (unsigned int)(b) << (ii * 8);
    }

    if (outSize == 0xFFFFFFFF)
    {
        PyErr_SetString(PyExc_ValueError, "stream version is not supported");
        goto exit;
    }

    for (ii = 0; ii < 4; ii++)
    {
        unsigned char b;
        memcpy(&b, &data[pos], sizeof(b));
        pos += sizeof(b);
        if (pos > length)
        {
            PyErr_SetString(PyExc_ValueError, "end of stream");
            goto exit;
        }
        
        if (b != 0)
        {
            PyErr_SetString(PyExc_ValueError, "too long file");
            goto exit;
        }
    }
    
    compressedSize = length - 13;
    if (compressedSize > (unsigned int)(length-pos))
    {
        PyErr_SetString(PyExc_ValueError, "end of stream");
        goto exit;
    }

    inStream = &data[pos];
    pos += compressedSize;

    prop0 = properties[0];
    if (prop0 >= (9*5*5))
    {
        PyErr_SetString(PyExc_ValueError, "properties error");
        goto exit;
    }
    for (pb = 0; prop0 >= (9 * 5); 
        pb++, prop0 -= (9 * 5));
    for (lp = 0; prop0 >= 9; 
        lp++, prop0 -= 9);
    lc = prop0;
    
    lzmaInternalSize = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp)))* sizeof(CProb);

    outStream = malloc(outSize);
    if (!outStream)
    {
        PyErr_NoMemory();
        goto exit;
    }
    
    lzmaInternalData = malloc(lzmaInternalSize);
    if (!lzmaInternalData)
    {
        PyErr_NoMemory();
        goto exit;
    }

    res = LzmaDecode((unsigned char *)lzmaInternalData, lzmaInternalSize,
                     lc, lp, pb,
                     (unsigned char *)inStream, compressedSize,
                     (unsigned char *)outStream, outSize, &outSizeProcessed);
    outSize = outSizeProcessed;

    if (res != 0)
    {
        PyErr_Format(PyExc_ValueError, "error during decompression (%d)", res);
        goto exit;
    }
    
    result = PyString_FromStringAndSize((const char *)outStream, outSize);

exit:
    if (outStream != NULL)
        free(outStream);
    if (lzmaInternalData != NULL)
        free(lzmaInternalData);
    
    return result;
}

#endif

static PyObject *pylzma_compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    // possible keywords for this function
    static char *kwlist[] = {"data", "dictionary", "fastBytes", "literalContextBits",
                             "literalPosBits", "posBits", "algorithm", "eos", NULL};
    int dictionary = 23;         // [0,28], default 23 (8MB)
    int fastBytes = 128;         // [5,255], default 128
    int literalContextBits = 3;  // [0,8], default 3
    int literalPosBits = 0;      // [0,4], default 0
    int posBits = 2;             // [0,4], default 2
    int eos = 0;                 // write "end of stream" marker?
    int algorithm = 2;
    char *data;
    int length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|lllllll", kwlist, &data, &length, &dictionary, &fastBytes,
                                                                &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos))
        return NULL;
    
    // XXX: verify that parameters are in the correct ranges
    
    return perform_compression(data, length, dictionary, fastBytes, literalContextBits, literalPosBits, posBits, algorithm, eos);
}

static PyObject *pylzma_decompress(PyObject *self, PyObject *args)
{
    char *data;
    int length;
    
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    
    return perform_decompression(data, length);
}

static void insint(PyObject *d, char *name, int value)
{
    PyObject *v = PyInt_FromLong((long) value);
    if (!v || PyDict_SetItemString(d, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

PyMethodDef methods[] = {
    // exported functions
    {"compress",   (PyCFunction)pylzma_compress,   METH_VARARGS | METH_KEYWORDS},
    {"decompress", (PyCFunction)pylzma_decompress, METH_VARARGS},
    {NULL, NULL},
};

DL_EXPORT(void) initpylzma(void)
{
    PyObject *m, *d;
    
    m = Py_InitModule("pylzma", methods);
    d = PyModule_GetDict(m);
}

#ifdef __cplusplus
}
#endif
