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
#include <src/LzmaStateDecode.h>

#include "pylzma.h"
#include "pylzma_decompress.h"
#include "pylzma_decompressobj.h"

int pylzma_decomp_init(CDecompressionObject *self, PyObject *args, PyObject *kwargs)
{
    int max_length = -1;
    
    // possible keywords for this function
    static char *kwlist[] = {"maxlength", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &max_length))
        return -1;
    
    if (max_length == 0 || max_length < -1) {
        PyErr_SetString(PyExc_ValueError, "the decompressed size must be greater than zero");
        return -1;
    }
    
    self->unconsumed_tail = NULL;
    self->unconsumed_length = 0;
    self->need_properties = 1;
    self->max_length = max_length;
    self->total_out = 0;
    memset(&self->state, 0, sizeof(self->state));
    return 0;
}

static const char doc_decomp_decompress[] = \
    "decompress(data[, bufsize]) -- Returns a string containing the up to bufsize decompressed bytes of the data.\n" \
    "After calling, some of the input data may be available in internal buffers for later processing.";

static PyObject *pylzma_decomp_decompress(CDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    unsigned char *data, *next_in, *next_out;
    int length, res, avail_out_param=BLOCK_SIZE;
    SizeT avail_in, avail_out;
    unsigned char properties[LZMA_PROPERTIES_SIZE];
    SizeT inProcessed, outProcessed;
    
    if (!PyArg_ParseTuple(args, "s#|i", &data, &length, &avail_out_param)){
        return NULL;
    }

    if (avail_out_param <= 0) {
        PyErr_SetString(PyExc_ValueError, "bufsize must be greater than zero");
        return NULL;
    }
    
    avail_out = avail_out_param;
    if (self->unconsumed_length > 0) {
        self->unconsumed_tail = (unsigned char *) realloc(self->unconsumed_tail, self->unconsumed_length + length);
        next_in = (unsigned char *) self->unconsumed_tail;
        memcpy(next_in + self->unconsumed_length, data, length);
    } else {
        next_in = data;
    }
    
    avail_in = self->unconsumed_length + length;
    
    if (self->need_properties && avail_in < sizeof(properties)) {
        // we need enough bytes to read the properties
        if (!self->unconsumed_length) {
            self->unconsumed_tail = (unsigned char *) malloc(length);
            memcpy(self->unconsumed_tail, data, length);
        }
        self->unconsumed_length += length;
        
        return PyString_FromString("");
    }

    if (self->need_properties) {
        self->need_properties = 0;
        memcpy(&properties, next_in, sizeof(properties));
        avail_in -= sizeof(properties);
        next_in += sizeof(properties);
        if (self->unconsumed_length >= sizeof(properties)-length) {
            self->unconsumed_length -= sizeof(properties)-length;
            if (self->unconsumed_length > 0) {
                memmove(self->unconsumed_tail, self->unconsumed_tail+sizeof(properties), self->unconsumed_length);
                self->unconsumed_tail = (unsigned char *) realloc(self->unconsumed_tail, self->unconsumed_length);
            } else {
                FREE_AND_NULL(self->unconsumed_tail);
            }
        }
        
        if (LzmaDecodeProperties(&self->state.Properties, properties, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK)
        {
            PyErr_SetString(PyExc_TypeError, "Incorrect stream properties");
            return NULL;
        }
        
        self->state.Probs = (CProb *) malloc(LzmaGetNumProbs(&self->state.Properties) * sizeof(CProb));
        if (self->state.Probs == NULL) {
            return PyErr_NoMemory();
        }
        
        if (self->state.Properties.DictionarySize == 0) {
            self->state.Dictionary = 0;
        } else {
            self->state.Dictionary = (unsigned char *) malloc(self->state.Properties.DictionarySize);
            if (self->state.Dictionary == NULL) {
                FREE_AND_NULL(self->state.Probs);
                return PyErr_NoMemory();
            }
        }
    
        LzmaDecoderInit(&self->state);
    }

    if (avail_in == 0) {
        // no more bytes to decompress
        return PyString_FromString("");
    }
    
    result = PyString_FromStringAndSize(NULL, avail_out);
    if (result == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    
    next_out = (unsigned char *) PyString_AS_STRING(result);
    Py_BEGIN_ALLOW_THREADS
    // Decompress until EOS marker is reached
    res = LzmaDecode(&self->state, next_in, avail_in, &inProcessed,
                     next_out, avail_out, &outProcessed, 0);
    Py_END_ALLOW_THREADS
    self->total_out += outProcessed;
    next_in += inProcessed;
    avail_in -= inProcessed;
    
    if (res != LZMA_RESULT_OK) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_ValueError, "data error during decompression");
        goto exit;
    }

    // Not all of the compressed data could be accomodated in the output buffer
    // of specified size. Return the unconsumed tail in an attribute.
    if (avail_in > 0) {
        if (self->unconsumed_tail == NULL) {
            // data are in "data"
            self->unconsumed_tail = (unsigned char *) malloc(avail_in);
            if (self->unconsumed_tail == NULL) {
                Py_DECREF(result);
                PyErr_NoMemory();
                goto exit;
            }
            memcpy(self->unconsumed_tail, next_in, avail_in);
        } else {
            memmove(self->unconsumed_tail, next_in, avail_in);
            self->unconsumed_tail = (unsigned char *) realloc(self->unconsumed_tail, avail_in);
        }
    } else {
        FREE_AND_NULL(self->unconsumed_tail);
    }
    
    self->unconsumed_length = avail_in;
    _PyString_Resize(&result, outProcessed);
    
exit:
    return result;
}

static const char doc_decomp_flush[] = \
    "flush() -- Return remaining data.";

static PyObject *pylzma_decomp_flush(CDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    int res;
    SizeT avail_out, outsize;
    unsigned char *tmp;
    SizeT inProcessed, outProcessed;
    
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    if (self->max_length != -1) {
        avail_out = self->max_length - self->total_out;
    } else {
        avail_out = BLOCK_SIZE;
    }
    
    if (avail_out == 0) {
        // no more remaining data
        return PyString_FromString("");
    }
    
    result = PyString_FromStringAndSize(NULL, avail_out);
    if (result == NULL) {
        return NULL;
    }
    
    tmp = (unsigned char *)PyString_AS_STRING(result);
    outsize = 0;
    while (1) {
        Py_BEGIN_ALLOW_THREADS
        if (self->unconsumed_length == 0) {
            // No remaining data
            res = LzmaDecode(&self->state, (unsigned char *)"", 0, &inProcessed,
                             tmp, avail_out, &outProcessed, 1);
        } else {
            // Decompress remaining data
            res = LzmaDecode(&self->state, self->unconsumed_tail, self->unconsumed_length, &inProcessed,
                             tmp, avail_out, &outProcessed, 1);
            self->unconsumed_length -= inProcessed;
            if (self->unconsumed_length > 0)
                memmove(self->unconsumed_tail, self->unconsumed_tail + inProcessed, self->unconsumed_length);
            else
                FREE_AND_NULL(self->unconsumed_tail);
        }
        Py_END_ALLOW_THREADS
        
        if (res != LZMA_RESULT_OK) {
            PyErr_SetString(PyExc_ValueError, "data error during decompression");
            DEC_AND_NULL(result);
            goto exit;
        }
        
        self->total_out += outProcessed;
        outsize += outProcessed;
        if (outProcessed < avail_out || (outProcessed == avail_out && self->max_length != -1)) {
            break;
        }
        
        if (self->max_length != -1) {
            PyErr_SetString(PyExc_ValueError, "not enough input data for decompression");
            DEC_AND_NULL(result);
            goto exit;
        }
        
        avail_out -= outProcessed;
        
        // Output buffer is full, might be more data for decompression
        if (_PyString_Resize(&result, outsize+BLOCK_SIZE) != 0) {
            goto exit;
        }
        
        avail_out += BLOCK_SIZE;
        tmp = (unsigned char *)PyString_AS_STRING(result) + outsize;
    }
    
    if (outsize != PyString_GET_SIZE(result)) {
        _PyString_Resize(&result, outsize);
    }
    
exit:
    return result;
}

static const char doc_decomp_reset[] = \
    "reset([maxlength]) -- Resets the decompression object.";

static PyObject *pylzma_decomp_reset(CDecompressionObject *self, PyObject *args, PyObject *kwargs)
{
    int max_length = -1;
    
    // possible keywords for this function
    static char *kwlist[] = {"maxlength", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &max_length))
        return NULL;
    
    free_lzma_state(&self->state);
    memset(&self->state, 0, sizeof(self->state));
    FREE_AND_NULL(self->unconsumed_tail);
    self->unconsumed_length = 0;
    self->need_properties = 1;
    self->total_out = 0;
    self->max_length = max_length;
    
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef pylzma_decomp_methods[] = {
    {"decompress", (PyCFunction)pylzma_decomp_decompress, METH_VARARGS, (char *)&doc_decomp_decompress},
    {"flush",      (PyCFunction)pylzma_decomp_flush,      METH_VARARGS, (char *)&doc_decomp_flush},
    {"reset",      (PyCFunction)pylzma_decomp_reset,      METH_VARARGS | METH_KEYWORDS, (char *)&doc_decomp_reset},
    {NULL},
};

static void pylzma_decomp_dealloc(CDecompressionObject *self)
{
    free_lzma_state(&self->state);
    FREE_AND_NULL(self->unconsumed_tail);
    self->ob_type->tp_free((PyObject*)self);
}

PyTypeObject CDecompressionObject_Type = {
    //PyObject_HEAD_INIT(&PyType_Type)
    PyObject_HEAD_INIT(NULL)
    0,
    "pylzma.decompressobj",              /* char *tp_name; */
    sizeof(CDecompressionObject),        /* int tp_basicsize; */
    0,                                   /* int tp_itemsize;       // not used much */
    (destructor)pylzma_decomp_dealloc,   /* destructor tp_dealloc; */
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
    Py_TPFLAGS_DEFAULT,                  /*tp_flags*/
    "Decompression class",               /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    pylzma_decomp_methods,               /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)pylzma_decomp_init,        /* tp_init */
    0,                                   /* tp_alloc */
    0,                                   /* tp_new */
};
