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
    
#define BLOCK_SIZE 65536

#include "pylzma.h"
#include "pylzma_streams.h"
#include "pylzma_encoder.h"

#include "Platform.h"  
#include <7zip/LzmaDecode.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define CHECK_NULL(a) if ((a) == NULL) { PyErr_NoMemory(); goto exit; }
#define DEC_AND_NULL(a) { Py_XDECREF(a); a = NULL; }
#define DELETE_AND_NULL(a) if (a != NULL) { delete a; a = NULL; }
#define CHECK_RANGE(x, a, b, msg) if ((x) < (a) || (x) > (b)) { PyErr_SetString(PyExc_ValueError, msg); return NULL; }

static void free_lzma_stream(lzma_stream *stream)
{
    if (stream->dynamicData)
        lzmafree(stream->dynamicData);
    stream->dynamicData = NULL;
    
    if (stream->dictionary)
        lzmafree(stream->dictionary);
    stream->dictionary = NULL;
}

static int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
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

static char *doc_compress = \
    "compress(string, dictionary=23, fastBytes=128, literalContextBits=3, literalPosBits=0, posBits=2, algorithm=2, eos=1) -- Compress the data in string using the given parameters, returning a string containing the compressed data.";

static PyObject *pylzma_compress(PyObject *self, PyObject *args, PyObject *kwargs)
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

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|lllllll", kwlist, &data, &length, &dictionary, &fastBytes,
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

static char *doc_comp_compress = \
    "docstring is todo\n";

static PyObject *pylzma_comp_compress(CCompressionObject *self, PyObject *args)
{
    PyObject *result = NULL;
    char *data;
    int length, bufsize=BLOCK_SIZE, finished;
    UINT64 inSize, outSize;
    
    if (!PyArg_ParseTuple(args, "s#|l", &data, &length, &bufsize))
        return NULL;
    
    if (!self->inStream->AppendData((BYTE *)data, length))
    {
        PyErr_NoMemory();
        goto exit;
    }
    
    while (true)
    {
        self->encoder->CodeOneBlock(&inSize, &outSize, &finished, false);
        //printf("2: %d %d %d %d\n", inSize, outSize, finished, self->outStream->getMaxRead());
        if (finished || self->outStream->getMaxRead() >= bufsize)
            break;
    }
    
    length = min(self->outStream->getMaxRead(), bufsize);
    result = PyString_FromStringAndSize((const char *)self->outStream->getReadPtr(), length);
    if (result == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    self->outStream->increaseReadPos(length);
    
exit:
    return result;
}

static char *doc_comp_flush = \
    "flush() -- Finishes the compression and returns any remaining compressed data.";

static PyObject *pylzma_comp_flush(CCompressionObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int finished=0;
    UINT64 inSize, outSize;
    
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    
    printf("flush: %d\n", self->inStream->getAvailIn());
    while (!finished)
    {
        self->encoder->CodeOneBlock(&inSize, &outSize, &finished, true);
    }
    
    self->encoder->FinishStream();    
    result = PyString_FromStringAndSize(NULL, self->outStream->getMaxRead());
    self->outStream->Read(PyString_AS_STRING(result), self->outStream->getMaxRead());
    
    return result;
}

PyMethodDef pylzma_comp_methods[] = {
    {"compress",   (PyCFunction)pylzma_comp_compress, METH_VARARGS, doc_comp_compress},
    {"flush",      (PyCFunction)pylzma_comp_flush,    METH_VARARGS, doc_comp_flush},
    {NULL, NULL},
};

static void pylzma_comp_dealloc(CCompressionObject *self)
{
    DELETE_AND_NULL(self->encoder);
    DELETE_AND_NULL(self->inStream);
    DELETE_AND_NULL(self->outStream);
    PyObject_Del(self);
}

static PyObject *pylzma_comp_getattr(CCompressionObject *self, char *attrname)
{
    return Py_FindMethod(pylzma_comp_methods, (PyObject *)self, attrname);
}

static int pylzma_comp_setattr(CCompressionObject *self, char *attrname, PyObject *value)
{
    // disable setting of attributes
    PyErr_Format(PyExc_AttributeError, "no attribute named '%s'", attrname);
    return -1;
}

PyTypeObject CompressionObject_Type = {
  //PyObject_HEAD_INIT(&PyType_Type)
  PyObject_HEAD_INIT(NULL)
  0,
  "LZMACompress",                      /* char *tp_name; */
  sizeof(CCompressionObject),          /* int tp_basicsize; */
  0,                                   /* int tp_itemsize;       // not used much */
  (destructor)pylzma_comp_dealloc,     /* destructor tp_dealloc; */
  NULL,                                /* printfunc  tp_print;   */
  (getattrfunc)pylzma_comp_getattr,    /* getattrfunc  tp_getattr; // __getattr__ */
  (setattrfunc)pylzma_comp_setattr,    /* setattrfunc  tp_setattr;  // __setattr__ */
  NULL,                                /* cmpfunc  tp_compare;  // __cmp__ */
  NULL,                                /* reprfunc  tp_repr;    // __repr__ */
  NULL,                                /* PyNumberMethods *tp_as_number; */
  NULL,                                /* PySequenceMethods *tp_as_sequence; */
  NULL,                                /* PyMappingMethods *tp_as_mapping; */
  NULL,                                /* hashfunc tp_hash;     // __hash__ */
  NULL,                                /* ternaryfunc tp_call;  // __call__ */
  NULL,                                /* reprfunc tp_str;      // __str__ */
};

static char *doc_compressobj = \
    "compressobj() -- Returns object that can be used for compression.";

static PyObject *pylzma_compressobj(PyObject *self, PyObject *args, PyObject *kwargs)
{
    CCompressionObject *result=NULL;
    NCompress::NLZMA::CPYLZMAEncoder *encoder;
    
    // possible keywords for this function
    static char *kwlist[] = {"dictionary", "fastBytes", "literalContextBits",
                             "literalPosBits", "posBits", "algorithm", "eos", NULL};
    int dictionary = 23;         // [0,28], default 23 (8MB)
    int fastBytes = 128;         // [5,255], default 128
    int literalContextBits = 3;  // [0,8], default 3
    int literalPosBits = 0;      // [0,4], default 0
    int posBits = 2;             // [0,4], default 2
    int eos = 1;                 // write "end of stream" marker?
    int algorithm = 2;
    int res;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|lllllll", kwlist, &dictionary, &fastBytes,
                                                               &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos))
        return NULL;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    
    encoder = new NCompress::NLZMA::CPYLZMAEncoder();
    CHECK_NULL(encoder);
    
    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos) != 0))
    {
        delete encoder;
        PyErr_Format(PyExc_TypeError, "Can't set coder properties: %d", res);
        goto exit;
    }
    
    result = PyObject_New(CCompressionObject, &CompressionObject_Type);
    if (result == NULL)
    {
        DELETE_AND_NULL(encoder);
        PyErr_NoMemory();
        goto exit;
    }
    
    result->encoder = encoder;
    result->inStream = new CInStream();
    if (result->inStream == NULL)
    {
        DELETE_AND_NULL(encoder);
        DEC_AND_NULL(result);
        PyErr_NoMemory();
    }
    result->outStream = new COutStream();
    if (result->outStream == NULL)
    {
        DELETE_AND_NULL(encoder);
        DELETE_AND_NULL(result->inStream);
        DEC_AND_NULL(result);
        PyErr_NoMemory();
    }

    Py_BEGIN_ALLOW_THREADS
    encoder->SetStreams(result->inStream, result->outStream, 0, 0);
    encoder->WriteCoderProperties(result->outStream);
    Py_END_ALLOW_THREADS
    
exit:
    
    return (PyObject *)result;
}

static char *doc_decompress = \
    "decompress(string) -- Decompress the data in string, returning a string containing the decompressed data.\n" \
    "decompress(string, bufsize) -- Decompress the data in string using an initial output buffer of size bufsize.\n";

static PyObject *pylzma_decompress(PyObject *self, PyObject *args)
{
    char *data;
    int length, blocksize=BLOCK_SIZE;
    PyObject *result = NULL;
    lzma_stream stream;
    int res;
    char *output;
    
    if (!PyArg_ParseTuple(args, "s#|l", &data, &length, &blocksize))
        return NULL;
    
    memset(&stream, 0, sizeof(stream));
    if (!(output = (char *)malloc(blocksize)))
    {
        PyErr_NoMemory();
        goto exit;
    }
    
    lzmaInit(&stream);
    stream.next_in = (Byte *)data;
    stream.avail_in = length;
    stream.next_out = (Byte *)output;
    stream.avail_out = blocksize;
    
    // decompress data
    while (true)
    {
        Py_BEGIN_ALLOW_THREADS
        res = lzmaDecode(&stream);
        Py_END_ALLOW_THREADS
        
        if (res == LZMA_STREAM_END) {
            break;
        } else if (res == LZMA_NOT_ENOUGH_MEM) {
            // out of memory during decompression
            PyErr_NoMemory();
            goto exit;
        } else if (res == LZMA_DATA_ERROR) {
            PyErr_SetString(PyExc_ValueError, "data error during decompression");
            goto exit;
        } else if (res == LZMA_OK) {
            // check if we need to adjust the output buffer
            if (stream.avail_out == 0)
            {
                output = (char *)realloc(output, blocksize+BLOCK_SIZE);
                stream.avail_out = BLOCK_SIZE;
                stream.next_out = (Byte *)&output[blocksize];
                blocksize += BLOCK_SIZE;
            };
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
    free_lzma_stream(&stream);
    if (output != NULL)
        free(output);
    
    return result;
}

static char *doc_decomp_decompress = \
    "decompress(data[, bufsize]) -- Returns a string containing the up to bufsize decompressed bytes of the data.\n" \
    "After calling, some of the input data may be available in internal buffers for later processing.";

static PyObject *pylzma_decomp_decompress(CDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    char *data;
    int length, old_length, start_total_out, res, max_length=BLOCK_SIZE;
    
    if (!PyArg_ParseTuple(args, "s#|l", &data, &length, &max_length))
        return NULL;

    if (max_length < 0)
    {
        PyErr_SetString(PyExc_ValueError, "bufsize must be greater than zero");
        return NULL;
    }
    
    start_total_out = self->stream.totalOut;
    self->stream.next_in = (Byte *)data;
    self->stream.avail_in = length;
    
    if (max_length && max_length < length)
        length = max_length;
    
    if (!(result = PyString_FromStringAndSize(NULL, length)))
        return NULL;
    
    self->stream.next_out = (unsigned char *)PyString_AS_STRING(result);
    self->stream.avail_out = length;
    
    Py_BEGIN_ALLOW_THREADS
    res = lzmaDecode(&self->stream);
    Py_END_ALLOW_THREADS
    
    while (res == LZMA_OK && self->stream.avail_out == 0)
    {
        if (max_length && length >= max_length)
            break;
        
        old_length = length;
        length <<= 1;
        if (max_length && length > max_length)
            length = max_length;
        
        if (_PyString_Resize(&result, length) < 0)
            goto exit;
        
        self->stream.avail_out = length - old_length;
        self->stream.next_out = (Byte *)PyString_AS_STRING(result) + old_length;
        
        Py_BEGIN_ALLOW_THREADS
        res = lzmaDecode(&self->stream);
        Py_END_ALLOW_THREADS
    }
        
    if (res == LZMA_NOT_ENOUGH_MEM) {
        // out of memory during decompression
        PyErr_NoMemory();
        DEC_AND_NULL(result);
        goto exit;
    } else if (res == LZMA_DATA_ERROR) {
        PyErr_SetString(PyExc_ValueError, "data error during decompression");
        DEC_AND_NULL(result);
        goto exit;
    } else if (res != LZMA_OK && res != LZMA_STREAM_END) {
        PyErr_Format(PyExc_ValueError, "unknown return code from lzmaDecode: %d", res);
        DEC_AND_NULL(result);
        goto exit;
    }

    /* Not all of the compressed data could be accomodated in the output buffer
    of specified size. Return the unconsumed tail in an attribute.*/
    if (max_length != 0) {
        Py_DECREF(self->unconsumed_tail);
        self->unconsumed_tail = PyString_FromStringAndSize((char *)self->stream.next_in, self->stream.avail_in);
        if (!self->unconsumed_tail) {
            PyErr_NoMemory();
            DEC_AND_NULL(result);
            goto exit;
        }
    }

    /* The end of the compressed data has been reached, so set the
       unused_data attribute to a string containing the remainder of the
       data in the string.  Note that this is also a logical place to call
       inflateEnd, but the old behaviour of only calling it on flush() is
       preserved.
    */
    if (res == LZMA_STREAM_END) {
        Py_XDECREF(self->unused_data);  /* Free original empty string */
        self->unused_data = PyString_FromStringAndSize((char *)self->stream.next_in, self->stream.avail_in);
        if (self->unused_data == NULL) {
            PyErr_NoMemory();
            DEC_AND_NULL(result);
            goto exit;
        }
    }
    
    _PyString_Resize(&result, self->stream.totalOut - start_total_out);
    
exit:
    return result;    
}

static char *doc_decomp_reset = \
    "reset() -- Resets the decompression object.";

static PyObject *pylzma_decomp_reset(CDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    
    lzmaInit(&self->stream);

    Py_DECREF(self->unconsumed_tail);
    self->unconsumed_tail = PyString_FromString("");
    CHECK_NULL(self->unconsumed_tail);

    Py_DECREF(self->unused_data);
    self->unused_data = PyString_FromString("");
    CHECK_NULL(self->unused_data );
    
    result = Py_None;

exit:    
    Py_XINCREF(result);
    return result;
}

PyMethodDef pylzma_decomp_methods[] = {
    {"decompress", (PyCFunction)pylzma_decomp_decompress, METH_VARARGS, doc_decomp_decompress},
    {"reset",      (PyCFunction)pylzma_decomp_reset,      METH_VARARGS, doc_decomp_reset},
    {NULL, NULL},
};

static void pylzma_decomp_dealloc(CDecompressionObject *self)
{
    free_lzma_stream(&self->stream);
    DEC_AND_NULL(self->unconsumed_tail);
    DEC_AND_NULL(self->unused_data);
    PyObject_Del(self);
}

static PyObject *pylzma_decomp_getattr(CDecompressionObject *self, char *attrname)
{
    if (strcmp(attrname, "unconsumed_tail") == 0) {
        Py_INCREF(self->unconsumed_tail);
        return self->unconsumed_tail;
    } else if (strcmp(attrname, "unused_data") == 0) {
        Py_INCREF(self->unused_data);
        return self->unused_data;
    } else
        return Py_FindMethod(pylzma_decomp_methods, (PyObject *)self, attrname);
}

static int pylzma_decomp_setattr(CDecompressionObject *self, char *attrname, PyObject *value)
{
    // disable setting of attributes
    PyErr_Format(PyExc_AttributeError, "no attribute named '%s'", attrname);
    return -1;
}

PyTypeObject DecompressionObject_Type = {
  //PyObject_HEAD_INIT(&PyType_Type)
  PyObject_HEAD_INIT(NULL)
  0,
  "LZMADecompress",                    /* char *tp_name; */
  sizeof(CDecompressionObject),        /* int tp_basicsize; */
  0,                                   /* int tp_itemsize;       // not used much */
  (destructor)pylzma_decomp_dealloc,   /* destructor tp_dealloc; */
  NULL,                                /* printfunc  tp_print;   */
  (getattrfunc)pylzma_decomp_getattr,  /* getattrfunc  tp_getattr; // __getattr__ */
  (setattrfunc)pylzma_decomp_setattr,  /* setattrfunc  tp_setattr;  // __setattr__ */
  NULL,                                /* cmpfunc  tp_compare;  // __cmp__ */
  NULL,                                /* reprfunc  tp_repr;    // __repr__ */
  NULL,                                /* PyNumberMethods *tp_as_number; */
  NULL,                                /* PySequenceMethods *tp_as_sequence; */
  NULL,                                /* PyMappingMethods *tp_as_mapping; */
  NULL,                                /* hashfunc tp_hash;     // __hash__ */
  NULL,                                /* ternaryfunc tp_call;  // __call__ */
  NULL,                                /* reprfunc tp_str;      // __str__ */
};

static char *doc_decompressobj = \
    "decompressobj() -- Returns object that can be used for decompression.";

static PyObject *pylzma_decompressobj(PyObject *self, PyObject *args)
{
    CDecompressionObject *result=NULL;
    
    if (!PyArg_ParseTuple(args, ""))
        goto exit;
    
    result = PyObject_New(CDecompressionObject, &DecompressionObject_Type);
    CHECK_NULL(result);
    
    result->unconsumed_tail = PyString_FromString("");
    if (result->unconsumed_tail == NULL)
    {
        PyErr_NoMemory();
        PyObject_Del(result);
        result = NULL;
        goto exit;
    }
    
    result->unused_data = PyString_FromString("");
    if (result->unused_data == NULL)
    {
        PyErr_NoMemory();
        DEC_AND_NULL(result->unconsumed_tail);
        PyObject_Del(result);
        result = NULL;
        goto exit;
    }    
    
    memset(&result->stream, 0, sizeof(result->stream));
    lzmaInit(&result->stream);
    
exit:
    
    return (PyObject *)result;
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
    {"compress",      (PyCFunction)pylzma_compress,      METH_VARARGS | METH_KEYWORDS, doc_compress},
    {"decompress",    (PyCFunction)pylzma_decompress,    METH_VARARGS,                 doc_decompress},
    // XXX: compression through an object doesn't work, yet
    //{"compressobj",   (PyCFunction)pylzma_compressobj,   METH_VARARGS | METH_KEYWORDS, doc_compressobj},
    {"decompressobj", (PyCFunction)pylzma_decompressobj, METH_VARARGS,                 doc_decompressobj},
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
