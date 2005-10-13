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

#ifndef ___PYLZMA_DECOMPRESSOBJ__H___
#define ___PYLZMA_DECOMPRESSOBJ__H___

#include <Python.h>
#include <7zip/LzmaStateDecode.h>

#define IN_BUFFER_SIZE (1 << 15)
#define OUT_BUFFER_SIZE (1 << 15)

typedef struct {
    PyObject_HEAD
    CLzmaDecoderState state;
    unsigned char in_buffer[IN_BUFFER_SIZE];
    unsigned int in_avail;
    unsigned char out_buffer[OUT_BUFFER_SIZE];
    unsigned int out_avail;
    unsigned int total_out;
    char *unconsumed_tail;
    int unconsumed_length;
    PyObject *unused_data;
} CDecompressionObject;

extern PyTypeObject CDecompressionObject_Type;

#define DecompressionObject_Check(v)   ((v)->ob_type == &CDecompressionObject_Type)

extern const char doc_decompressobj[];
PyObject *pylzma_decompressobj(PyObject *self, PyObject *args);

#endif
