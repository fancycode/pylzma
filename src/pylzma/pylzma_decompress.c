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

#include "../sdk/LzmaDec.h"

#include "pylzma.h"
#include "pylzma_streams.h"

const char
doc_decompress[] = \
    "decompress(data[, maxlength]) -- Decompress the data, returning a string containing the decompressed data. "\
    "If the string has been compressed without an EOS marker, you must provide the maximum length as keyword parameter.\n" \
    "decompress(data, bufsize[, maxlength]) -- Decompress the data using an initial output buffer of size bufsize. "\
    "If the string has been compressed without an EOS marker, you must provide the maximum length as keyword parameter.\n";

PyObject *
pylzma_decompress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    unsigned char *data;
    Byte *tmp;
    int length, bufsize=BLOCK_SIZE, avail, totallength=-1;
    PyObject *result=NULL;
    CLzmaDec state;
    ELzmaStatus status;
    size_t srcLen, destLen;
    int res;
    CMemoryOutStream outStream;
    // possible keywords for this function
    static char *kwlist[] = {"data", "bufsize", "maxlength", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|ii", kwlist, &data, &length, &bufsize, &totallength))
        return NULL;
    
    if (totallength != -1) {
        // We know the decompressed size, run simple case
        result = PyString_FromStringAndSize(NULL, totallength);
        if (result == NULL) {
            return NULL;
        }
        
        tmp = (Byte *) PyString_AS_STRING(result);
        srcLen = length - LZMA_PROPS_SIZE;
        destLen = totallength;
        Py_BEGIN_ALLOW_THREADS
        res = LzmaDecode(tmp, &destLen, (Byte *) (data + LZMA_PROPS_SIZE), &srcLen, data, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &allocator);
        Py_END_ALLOW_THREADS
        if (res != SZ_OK) {
            Py_DECREF(result);
            result = NULL;
            PyErr_Format(PyExc_TypeError, "Error while decompressing: %d", res);
        } else if (destLen < (size_t) totallength) {
            _PyString_Resize(&result, destLen);
        }
        return result;
    }
    
    CreateMemoryOutStream(&outStream);
    tmp = (Byte *) malloc(bufsize);
    if (tmp == NULL) {
        return PyErr_NoMemory();
    }
    
    LzmaDec_Construct(&state);
    res = LzmaDec_Allocate(&state, data, LZMA_PROPS_SIZE, &allocator);
    if (res != SZ_OK) {
        PyErr_SetString(PyExc_TypeError, "Incorrect stream properties");
        goto exit;
    }
    
    data += LZMA_PROPS_SIZE;
    avail = length - LZMA_PROPS_SIZE;
    Py_BEGIN_ALLOW_THREADS
    LzmaDec_Init(&state);
    for (;;) {
        srcLen = avail;
        destLen = bufsize;
        
        res = LzmaDec_DecodeToBuf(&state, tmp, &destLen, data, &srcLen, LZMA_FINISH_ANY, &status);
        data += srcLen;
        avail -= srcLen;
        if (res == SZ_OK && destLen > 0 && outStream.s.Write(&outStream, tmp, destLen) != destLen) {
            res = SZ_ERROR_WRITE;
        }
        if (res != SZ_OK || status == LZMA_STATUS_FINISHED_WITH_MARK || status == LZMA_STATUS_NEEDS_MORE_INPUT) {
            break;
        }
        
    }
    Py_END_ALLOW_THREADS
    
    if (status == LZMA_STATUS_NEEDS_MORE_INPUT) {
        PyErr_SetString(PyExc_ValueError, "data error during decompression");
    } else if (res != SZ_OK) {
        PyErr_Format(PyExc_TypeError, "Error while decompressing: %d", res);
    } else {
        result = PyString_FromStringAndSize((const char *) outStream.data, outStream.size);
    }
    
exit:
    if (outStream.data != NULL) {
        free(outStream.data);
    }
    LzmaDec_Free(&state, &allocator);
    free(tmp);
    
    return result;
}
