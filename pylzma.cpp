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

#include <Python.h>
    
#define BLOCK_SIZE 65536
  
#include "Platform.h"  
#include <7zip/7zip/IStream.h>
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>
#include <7zip/Common/MyCom.h>
#include <7zip/LzmaDecode.h>

class CInStream :
    public ISequentialInStream,
    public CMyUnknownImp
{
private:
    BYTE *next_in;
    UINT avail_in;
    UINT pos;

public:
    MY_UNKNOWN_IMP

    CInStream(BYTE *data, int length)
    {
        next_in = data;
        avail_in = length;
        pos = 0;
    }
    
    virtual LONG Read(void *data, UINT32 size, UINT32 *processedSize)
    {
        return ReadPart(data, size, processedSize);
    }
    
    virtual LONG ReadPart(void *data, UINT32 size, UINT32 *processedSize)
    {
        if (processedSize)
            *processedSize = 0;
        
        while (size)
        {
            if (!avail_in)
                return S_OK;
            
            UINT32 len = size < avail_in ? size : avail_in;
            memcpy(data, next_in, len);
            avail_in -= len;
            size -= len;
            next_in += len;
            data = PBYTE(data) + len;
            if (processedSize)
                *processedSize += len;
        }
        
        return S_OK;
    }    
};

class COutStream :
    public ISequentialOutStream,
    public CMyUnknownImp
{
private:
    BYTE *buffer;
    BYTE *next_out;
    UINT avail_out;
    UINT count;

public:
    MY_UNKNOWN_IMP

    COutStream()
    {
        buffer = (BYTE *)malloc(BLOCK_SIZE);
        next_out = buffer;
        avail_out = BLOCK_SIZE;
        count = 0;
    }
    
    virtual ~COutStream()
    {
        if (buffer)
            free(buffer);
        buffer = NULL;
    }
    
    char *getData()
    {
        return (char *)buffer;
    }
    
    int getLength()
    {
        return count;
    }

    virtual LONG Write(const void *data, UINT32 size, UINT32 *processedSize)
    {
        return WritePart(data, size, processedSize);
    }
    
    virtual LONG WritePart(const void *data, UINT32 size, UINT32 *processedSize)
    {
        if (processedSize)
            *processedSize = 0;
        
        while (size)
        {
            if (!avail_out)
            {
                buffer = (BYTE *)realloc(buffer, count + BLOCK_SIZE);
                avail_out += BLOCK_SIZE;
                next_out = &buffer[count];
            }
            
            UINT32 len = size < avail_out ? size : avail_out;
            memcpy(next_out, data, len);
            avail_out -= len;
            size -= len;
            next_out += len;
            count += len;
            data = PBYTE(data) + len;
            if (processedSize)
                *processedSize += len;
        }
        
        return S_OK;
    }    
};

static PyObject *perform_compression(char *data, int length, int dictionary, int fastBytes, int literalContextBits,
				     int literalPosBits, int posBits, int algorithm, int eos)
{
    PyObject *result = NULL;
    NCompress::NLZMA::CEncoder *encoder;
    CInStream *inStream = new CInStream((BYTE *)data, length);
    COutStream *outStream = new COutStream();
    int res;

    encoder = new NCompress::NLZMA::CEncoder();
    encoder->SetWriteEndMarkerMode(true);

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
    if ((res = encoder->SetCoderProperties(propIDs, props, kNumProps)) != 0)
    {
        PyErr_Format(PyExc_TypeError, "Can't set coder properties: %d", res);
        goto exit;
    }
    
    encoder->SetStreams(inStream, outStream, 0, 0);
    encoder->WriteCoderProperties(outStream);
    encoder->CodeReal(inStream, outStream, NULL, 0, 0);
    
    result = PyString_FromStringAndSize((const char *)outStream->getData(), outStream->getLength());
    
exit:
    if (encoder != NULL)
        delete encoder;
    if (inStream != NULL)
        delete inStream;
    if (outStream != NULL)
        delete outStream;
    return result;
}

static PyObject *perform_decompression(char *data, int length)
{
    PyObject *result = NULL;
    lzma_stream stream;
    int size=BLOCK_SIZE, res;
    char *output=(char *)malloc(BLOCK_SIZE);
    
    if (!output)
    {
        PyErr_NoMemory();
        goto exit;
    }
    
    memset(&stream, 0, sizeof(stream));
    lzmaInit(&stream);
    stream.next_in = (Byte *)data;
    stream.avail_in = length;
    stream.next_out = (Byte *)output;
    stream.avail_out = BLOCK_SIZE;
    
    // decompress data
    while ((res = lzmaDecode(&stream)) != LZMA_STREAM_END)
    {
        if (res == LZMA_NOT_ENOUGH_MEM)
        {
            // out of memory during decompression
            PyErr_NoMemory();
            goto exit;
        } else if (res == LZMA_DATA_ERROR) {
            PyErr_SetString(PyExc_ValueError, "data error during decompression");
            goto exit;
        } else if (res == LZMA_OK) {
            output = (char *)realloc(output, size+BLOCK_SIZE);
            stream.avail_out += BLOCK_SIZE;
            stream.next_out = (Byte *)&output[stream.totalOut];
        } else {
            PyErr_Format(PyExc_ValueError, "unknown return code from lzmaDecode: %d", res);
            goto exit;
        }
        
        // if we exit here, decompression finished without returning LZMA_STREAM_END
        // XXX: why is this sometimes?
        if (stream.avail_in == 0)
            break;
    }

    result = PyString_FromStringAndSize(output, stream.totalOut);
    
exit:
    if (output != NULL)
        free(output);
    
    return result;
}

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

#ifdef __cplusplus
extern "C" {
#endif

DL_EXPORT(void) initpylzma(void)
{
    PyObject *m, *d;
    
    m = Py_InitModule("pylzma", methods);
    d = PyModule_GetDict(m);
}

#ifdef __cplusplus
}
#endif
