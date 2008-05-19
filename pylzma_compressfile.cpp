/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2008 by Joachim Bauch, mail@joachim-bauch.de
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
#include "pylzma_compressfile.h"

int set_encoder_properties(NCompress::NLZMA::CEncoder *encoder, int dictionary, int posBits,
    int literalContextBits, int literalPosBits, int algorithm, int fastBytes, int eos,
    int multithreading, const char *matchfinder);

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

static PyMethodDef pylzma_compfile_methods[] = {
    {"read",   (PyCFunction)pylzma_compfile_read, METH_VARARGS, doc_compfile_read},
    {NULL, NULL},
};

static void pylzma_compfile_dealloc(CCompressionFileObject *self)
{
    DEC_AND_NULL(self->inFile);
    DELETE_AND_NULL(self->encoder);
    DELETE_AND_NULL(self->inStream);
    self->ob_type->tp_free((PyObject*)self);
}

#ifdef __cplusplus
extern "C" {
#endif

static PyObject* pylzma_compfile_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CCompressionFileObject *self;
    
    self = (CCompressionFileObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->encoder = NULL;
        self->inStream = NULL;
        self->outStream = NULL;
        self->inFile = NULL;
    }
    
    return (PyObject*)self;
}

int pylzma_compfile_init(CCompressionFileObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *inFile;
    NCompress::NLZMA::CEncoder *encoder;
    int result = -1;
    
    // possible keywords for this function
    static char *kwlist[] = {"infile", "dictionary", "fastBytes", "literalContextBits",
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
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iiiiiiiis", kwlist, &inFile, &dictionary, &fastBytes,
                                                                 &literalContextBits, &literalPosBits, &posBits, &algorithm, &eos, &multithreading, &matchfinder))
        return -1;
    
    CHECK_RANGE(dictionary,         0,  28, "dictionary must be between 0 and 28");
    CHECK_RANGE(fastBytes,          5, 255, "fastBytes must be between 5 and 255");
    CHECK_RANGE(literalContextBits, 0,   8, "literalContextBits must be between 0 and 8");
    CHECK_RANGE(literalPosBits,     0,   4, "literalPosBits must be between 0 and 4");
    CHECK_RANGE(posBits,            0,   4, "posBits must be between 0 and 4");
    CHECK_RANGE(algorithm,          0,   2, "algorithm must be between 0 and 2");
    
    if (PyString_Check(inFile)) {
        // create new cStringIO object from string
        inFile = PycStringIO->NewInput(inFile);
        if (inFile == NULL)
        {
            PyErr_NoMemory();
            return -1;
        }
    } else if (!PyObject_HasAttrString(inFile, "read")) {
        PyErr_SetString(PyExc_ValueError, "first parameter must be a file-like object");
        return -1;
    } else
        // protect object from being refcounted out...
        Py_XINCREF(inFile);
    
    encoder = new NCompress::NLZMA::CEncoder();
    CHECK_NULL(encoder);
    
    if ((res = set_encoder_properties(encoder, dictionary, posBits, literalContextBits, literalPosBits, algorithm, fastBytes, eos, multithreading, matchfinder) != 0))
    {
        Py_XDECREF(inFile);
        DELETE_AND_NULL(encoder);
        PyErr_SetString(PyExc_TypeError, "can't set coder properties");
        goto exit;
    }
    
    self->inFile = inFile;
    self->encoder = encoder;
    self->inStream = new CInStream(inFile);
    if (self->inStream == NULL)
    {
        DEC_AND_NULL(self->inFile);
        DELETE_AND_NULL(encoder);
        DEC_AND_NULL(self);
        PyErr_NoMemory();
        goto exit;
    }
    self->outStream = new COutStream();
    if (self->outStream == NULL)
    {
        DEC_AND_NULL(self->inFile);
        DELETE_AND_NULL(encoder);
        DELETE_AND_NULL(self->inStream);
        DEC_AND_NULL(self);
        PyErr_NoMemory();
        goto exit;
    }

    Py_BEGIN_ALLOW_THREADS
    encoder->SetStreams(self->inStream, self->outStream, 0, 0);
    encoder->WriteCoderProperties(self->outStream);
    Py_END_ALLOW_THREADS
    result = 0;
    
exit:
    return result;
}

#ifdef __cplusplus
}
#endif

PyTypeObject CCompressionFileObject_Type = {
    //PyObject_HEAD_INIT(&PyType_Type)
    PyObject_HEAD_INIT(NULL)
    0,
    "pylzma.compressfile",                  /* char *tp_name; */
    sizeof(CCompressionFileObject),      /* int tp_basicsize; */
    0,                                   /* int tp_itemsize;       // not used much */
    (destructor)pylzma_compfile_dealloc, /* destructor tp_dealloc; */
    NULL,                                /* printfunc  tp_print;   */
    NULL,                                /* getattrfunc  tp_getattr; // __getattr__ */
    NULL,                                /* setattrfunc  tp_setattr;  // __setattr__ */
    NULL,                                /* cmpfunc  tp_compare;  // __cmp__ */
    NULL,                                /* reprfunc  tp_repr;    // __repr__ */
    NULL,                                /* PyNumberMethods *tp_as_number; */
    NULL,                                /* PySequenceMethods *tp_as_sequence; */
    NULL,                                /* PyMappingMethods *tp_as_mapping; */
    NULL,                                /* hashfunc tp_hash;     // __hash__ */
    NULL,                                /* ternaryfunc tp_call;  // __call__ */
    NULL,                                /* reprfunc tp_str;      // __str__ */
    0,                                   /* tp_getattro*/
    0,                                   /* tp_setattro*/
    0,                                   /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,  /*tp_flags*/
    "File compression class",           /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    pylzma_compfile_methods,             /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)pylzma_compfile_init,      /* tp_init */
    0,                                   /* tp_alloc */
    pylzma_compfile_new,                 /* tp_new */
};
