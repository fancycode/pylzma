/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2010 by Joachim Bauch, mail@joachim-bauch.de
 * 7-Zip Copyright (C) 1999-2010 Igor Pavlov
 * LZMA SDK Copyright (C) 1999-2010 Igor Pavlov
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

#include "pylzma.h"
#include "pylzma_decompress_compat.h"
#include "pylzma_decompressobj_compat.h"

static const char doc_decomp_decompress[] = \
    "decompress(data[, bufsize]) -- Returns a string containing the up to bufsize decompressed bytes of the data.\n" \
    "After calling, some of the input data may be available in internal buffers for later processing.";

static PyObject *pylzma_decomp_decompress(CCompatDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    char *data;
    int length, old_length, start_total_out, res, max_length=BLOCK_SIZE;
    
    if (!PyArg_ParseTuple(args, "s#|i", &data, &length, &max_length))
        return NULL;

    if (max_length < 0)
    {
        PyErr_SetString(PyExc_ValueError, "bufsize must be greater than zero");
        return NULL;
    }
    
    start_total_out = self->stream.totalOut;
    if (self->unconsumed_length > 0) {
        self->unconsumed_tail = (char *)realloc(self->unconsumed_tail, self->unconsumed_length + length);
        self->stream.next_in = (Byte *)self->unconsumed_tail;
        memcpy(self->stream.next_in + self->unconsumed_length, data, length);
    } else
        self->stream.next_in = (Byte *)data;
    
    self->stream.avail_in = self->unconsumed_length + length;
    
    if (max_length && max_length < length)
        length = max_length;
    
    if (!(result = PyString_FromStringAndSize(NULL, length)))
        return NULL;
    
    self->stream.next_out = (unsigned char *)PyString_AS_STRING(result);
    self->stream.avail_out = length;
    
    Py_BEGIN_ALLOW_THREADS
    res = lzmaCompatDecode(&self->stream);
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
        res = lzmaCompatDecode(&self->stream);
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
        if (self->stream.avail_in > 0)
        {
            if (self->stream.avail_in != self->unconsumed_length)
                self->unconsumed_tail = (char *)realloc(self->unconsumed_tail, self->stream.avail_in);
            
            if (!self->unconsumed_tail) {
                PyErr_NoMemory();
                DEC_AND_NULL(result);
                goto exit;
            }
            memcpy(self->unconsumed_tail, self->stream.next_in, self->stream.avail_in);
        } else
            FREE_AND_NULL(self->unconsumed_tail);
        self->unconsumed_length = self->stream.avail_in;
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

static const char doc_decomp_reset[] = \
    "reset() -- Resets the decompression object.";

static PyObject *pylzma_decomp_reset(CCompatDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    
    lzmaCompatInit(&self->stream);
    FREE_AND_NULL(self->unconsumed_tail);
    self->unconsumed_length = 0;
    
    Py_DECREF(self->unused_data);
    self->unused_data = PyString_FromString("");
    CHECK_NULL(self->unused_data);
    
    result = Py_None;
    Py_XINCREF(result);
    
exit:
    return result;
}

PyMethodDef pylzma_decomp_compat_methods[] = {
    {"decompress", (PyCFunction)pylzma_decomp_decompress, METH_VARARGS, (char *)&doc_decomp_decompress},
    {"reset",      (PyCFunction)pylzma_decomp_reset,      METH_VARARGS, (char *)&doc_decomp_reset},
    {NULL, NULL},
};

static void pylzma_decomp_dealloc(CCompatDecompressionObject *self)
{
    free_lzma_stream(&self->stream);
    FREE_AND_NULL(self->unconsumed_tail);
    DEC_AND_NULL(self->unused_data);
    PyObject_Del(self);
}

static PyObject *pylzma_decomp_getattr(CCompatDecompressionObject *self, char *attrname)
{
    if (strcmp(attrname, "unused_data") == 0) {
        Py_INCREF(self->unused_data);
        return self->unused_data;
    } else
        return Py_FindMethod(pylzma_decomp_compat_methods, (PyObject *)self, attrname);
}

static int pylzma_decomp_setattr(CCompatDecompressionObject *self, char *attrname, PyObject *value)
{
    // disable setting of attributes
    PyErr_Format(PyExc_AttributeError, "no attribute named '%s'", attrname);
    return -1;
}

PyTypeObject CompatDecompressionObject_Type = {
  //PyObject_HEAD_INIT(&PyType_Type)
  PyObject_HEAD_INIT(NULL)
  0,
  "LZMACompatDecompress",              /* char *tp_name; */
  sizeof(CCompatDecompressionObject),  /* int tp_basicsize; */
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

const char doc_decompressobj_compat[] = \
    "decompressobj_compat() -- Returns object that can be used for decompression.";

PyObject *pylzma_decompressobj_compat(PyObject *self, PyObject *args)
{
    CCompatDecompressionObject *result=NULL;
    
    if (!PyArg_ParseTuple(args, ""))
        goto exit;
    
    result = PyObject_New(CCompatDecompressionObject, &CompatDecompressionObject_Type);
    CHECK_NULL(result);
    
    result->unconsumed_tail = NULL;
    result->unconsumed_length = 0;

    result->unused_data = PyString_FromString("");
    if (result->unused_data == NULL)
    {
        PyErr_NoMemory();
        PyObject_Del(result);
        result = NULL;
        goto exit;
    }    
    
    memset(&result->stream, 0, sizeof(result->stream));
    lzmaCompatInit(&result->stream);
    
exit:
    
    return (PyObject *)result;
}
