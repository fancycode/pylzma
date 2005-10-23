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
#include <Platform.h>
#include <7zip/7zip/IStream.h>
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>

#include "pylzma.h"
#include "pylzma_streams.h"
#include "pylzma_compress.h"
#include "pylzma_encoder.h"

int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
    int literalContextBits, int literalPosBits, int algorithm, int fastBytes, int eos,
    int multithreading, const char *matchfinder);

typedef struct {
    PyObject_HEAD
    NCompress::NLZMA::CPYLZMAEncoder *encoder;
    CInStream *inStream;
    COutStream *outStream;
} CCompressionObject;

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

#ifdef __cplusplus
extern "C"
#endif
const char doc_compressobj[] = \
    "compressobj() -- Returns object that can be used for compression.";

#ifdef __cplusplus
extern "C" {
#endif

PyObject *pylzma_compressobj(PyObject *self, PyObject *args, PyObject *kwargs)
{
    CCompressionObject *result=NULL;
    NCompress::NLZMA::CPYLZMAEncoder *encoder;
    
    // possible keywords for this function
    static char *kwlist[] = {"dictionary", "fastBytes", "literalContextBits",
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
    int res;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiiiiiiis", kwlist, &dictionary, &fastBytes,
                                                                 &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos, &multithreading, &matchfinder))
        return NULL;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    
    encoder = new NCompress::NLZMA::CPYLZMAEncoder();
    CHECK_NULL(encoder);
    
    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos, multithreading, matchfinder) != 0))
    {
        delete encoder;
        PyErr_SetString(PyExc_TypeError, "can't set coder properties");
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
        goto exit;
    }
    result->outStream = new COutStream();
    if (result->outStream == NULL)
    {
        DELETE_AND_NULL(encoder);
        DELETE_AND_NULL(result->inStream);
        DEC_AND_NULL(result);
        PyErr_NoMemory();
        goto exit;
    }

    Py_BEGIN_ALLOW_THREADS
    encoder->SetStreams(result->inStream, result->outStream, 0, 0);
    encoder->WriteCoderProperties(result->outStream);
    Py_END_ALLOW_THREADS
    
exit:
    
    return (PyObject *)result;
}

#ifdef __cplusplus
}
#endif
