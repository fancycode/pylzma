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
#include <cStringIO.h>
#include <7zip/Common/MyWindows.h>
#include <7zip/7zip/IStream.h>
#include <7zip/7zip/Compress/LZMA/LZMAEncoder.h>

#include "pylzma.h"
#include "pylzma_streams.h"
#include "pylzma_compress.h"

int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
                           int literalContextBits, int literalPosBits, int algorithm,
                           int fastBytes, int eos, int multithreading);

typedef struct {
    PyObject_HEAD
    NCompress::NLZMA::CEncoder *encoder;
    CInStream *inStream;
    COutStream *outStream;
    PyObject *inFile;
} CCompressionFileObject;

static char *doc_compfile_read = \
    "docstring is todo\n";

static PyObject *pylzma_compfile_read(CCompressionFileObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int length, bufsize=0;
    
    if (!PyArg_ParseTuple(args, "|l", &bufsize))
        return NULL;
    
    while (!bufsize || self->outStream->getMaxRead() < bufsize)
    {
        UInt64 processedInSize;
        UInt64 processedOutSize;
        INT32 finished;
        
        Py_BEGIN_ALLOW_THREADS
        self->encoder->CodeOneBlock(&processedInSize, &processedOutSize, &finished);
        Py_END_ALLOW_THREADS
        if (finished)
            break;
    }
    
    if (bufsize)
        length = min(bufsize, self->outStream->getMaxRead());
    else
        length = self->outStream->getMaxRead();
    result = PyString_FromStringAndSize((const char *)self->outStream->getReadPtr(), length);
    if (result == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    self->outStream->increaseReadPos(length);
    
exit:
    return result;
}

PyMethodDef pylzma_compfile_methods[] = {
    {"read",   (PyCFunction)pylzma_compfile_read, METH_VARARGS, doc_compfile_read},
    {NULL, NULL},
};

static void pylzma_compfile_dealloc(CCompressionFileObject *self)
{
    DEC_AND_NULL(self->inFile);
    DELETE_AND_NULL(self->encoder);
    DELETE_AND_NULL(self->inStream);
    PyObject_Del(self);
}

static PyObject *pylzma_compfile_getattr(CCompressionFileObject *self, char *attrname)
{
    return Py_FindMethod(pylzma_compfile_methods, (PyObject *)self, attrname);
}

static int pylzma_compfile_setattr(CCompressionFileObject *self, char *attrname, PyObject *value)
{
    // disable setting of attributes
    PyErr_Format(PyExc_AttributeError, "no attribute named '%s'", attrname);
    return -1;
}

PyTypeObject CompressionFileObject_Type = {
  //PyObject_HEAD_INIT(&PyType_Type)
  PyObject_HEAD_INIT(NULL)
  0,
  "LZMACompressFile",                   /* char *tp_name; */
  sizeof(CCompressionFileObject),       /* int tp_basicsize; */
  0,                                    /* int tp_itemsize;       // not used much */
  (destructor)pylzma_compfile_dealloc,  /* destructor tp_dealloc; */
  NULL,                                 /* printfunc  tp_print;   */
  (getattrfunc)pylzma_compfile_getattr, /* getattrfunc  tp_getattr; // __getattr__ */
  (setattrfunc)pylzma_compfile_setattr, /* setattrfunc  tp_setattr;  // __setattr__ */
  NULL,                                 /* cmpfunc  tp_compare;  // __cmp__ */
  NULL,                                 /* reprfunc  tp_repr;    // __repr__ */
  NULL,                                 /* PyNumberMethods *tp_as_number; */
  NULL,                                 /* PySequenceMethods *tp_as_sequence; */
  NULL,                                 /* PyMappingMethods *tp_as_mapping; */
  NULL,                                 /* hashfunc tp_hash;     // __hash__ */
  NULL,                                 /* ternaryfunc tp_call;  // __call__ */
  NULL,                                 /* reprfunc tp_str;      // __str__ */
};

#ifdef __cplusplus
extern "C"
#endif
const char doc_compressfile[] = \
    "compressfile(file) -- Returns object that compresses the contents of file.";

#ifdef __cplusplus
extern "C" {
#endif

PyObject *pylzma_compressfile(PyObject *self, PyObject *args, PyObject *kwargs)
{
    CCompressionFileObject *result=NULL;
    PyObject *inFile;
    NCompress::NLZMA::CEncoder *encoder;
    
    // possible keywords for this function
    static char *kwlist[] = {"infile", "dictionary", "fastBytes", "literalContextBits",
                             "literalPosBits", "posBits", "algorithm", "eos", "multithreading", NULL};
    int dictionary = 23;         // [0,28], default 23 (8MB)
    int fastBytes = 128;         // [5,255], default 128
    int literalContextBits = 3;  // [0,8], default 3
    int literalPosBits = 0;      // [0,4], default 0
    int posBits = 2;             // [0,4], default 2
    int eos = 1;                 // write "end of stream" marker?
    int multithreading = 1;      // use multithreading if available?
    int algorithm = 2;
    int res;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iiiiiiii", kwlist, &inFile, &dictionary, &fastBytes,
                                                                 &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos, &multithreading))
        return NULL;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    
    if (PyString_Check(inFile)) {
        // create new cStringIO object from string
        inFile = PycStringIO->NewInput(inFile);
        if (inFile == NULL)
        {
            PyErr_NoMemory();
            return NULL;
        }
    } else if (!PyObject_HasAttrString(inFile, "read")) {
        PyErr_SetString(PyExc_ValueError, "first parameter must be a file-like object");
        return NULL;
    } else
        // protect object from being refcounted out...
        Py_XINCREF(inFile);
    
    encoder = new NCompress::NLZMA::CEncoder();
    CHECK_NULL(encoder);
    
    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos, multithreading) != 0))
    {
        Py_XDECREF(inFile);
        DELETE_AND_NULL(encoder);
        PyErr_Format(PyExc_TypeError, "Can't set coder properties: %d", res);
        goto exit;
    }
    
    result = PyObject_New(CCompressionFileObject, &CompressionFileObject_Type);
    if (result == NULL)
    {
        Py_XDECREF(inFile);
        DELETE_AND_NULL(encoder);
        PyErr_NoMemory();
        goto exit;
    }
    
    result->inFile = inFile;
    result->encoder = encoder;
    result->inStream = new CInStream(inFile);
    if (result->inStream == NULL)
    {
        DEC_AND_NULL(result->inFile);
        DELETE_AND_NULL(encoder);
        DEC_AND_NULL(result);
        PyErr_NoMemory();
        goto exit;
    }
    result->outStream = new COutStream();
    if (result->outStream == NULL)
    {
        DEC_AND_NULL(result->inFile);
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
