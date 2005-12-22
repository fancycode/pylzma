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
#include <7zip/LzmaStateDecode.h>

#include "pylzma.h"

void free_lzma_state(CLzmaDecoderState *state)
{
    if (state->Probs)
        free(state->Probs);
    state->Probs = NULL;
    
    if (state->Dictionary)
        free(state->Dictionary);
    state->Dictionary = NULL;
}

const char doc_decompress[] = \
    "decompress(data[, maxlength]) -- Decompress the data, returning a string containing the decompressed data. "\
    "If the string has been compressed without an EOS marker, you must provide the maximum length as keyword parameter.\n" \
    "decompress(data, bufsize[, maxlength]) -- Decompress the data using an initial output buffer of size bufsize. "\
    "If the string has been compressed without an EOS marker, you must provide the maximum length as keyword parameter.\n";

PyObject *pylzma_decompress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    unsigned char *data, *tmp;
    int length, blocksize=BLOCK_SIZE, outsize, outavail, totallength=-1, bufsize;
    PyObject *result = NULL, *output=NULL;
    CLzmaDecoderState state;
    unsigned char properties[LZMA_PROPERTIES_SIZE];
    int res;
    // possible keywords for this function
    static char *kwlist[] = {"data", "bufsize", "maxlength", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|ll", kwlist, &data, &length, &blocksize, &totallength))
        return NULL;
    
    memset(&state, 0, sizeof(state));
    if (!(output = PyString_FromStringAndSize(NULL, blocksize)))
    {
        PyErr_NoMemory();
        goto exit;
    }
    
    // Initialize LZMA state decoder
    memcpy(&properties, data, sizeof(properties));
    data += sizeof(properties);
    length -= sizeof(properties);
    if (LzmaDecodeProperties(&state.Properties, properties, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK)
    {
        PyErr_SetString(PyExc_TypeError, "Incorrect stream properties");
        goto exit;
    }

    state.Probs = (CProb *)malloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
    if (state.Probs == 0) {
        PyErr_NoMemory();
        goto exit;
    }
    
    if (state.Properties.DictionarySize == 0)
        state.Dictionary = 0;
    else {
        state.Dictionary = (unsigned char *)malloc(state.Properties.DictionarySize);
        if (state.Dictionary == 0) {
            free(state.Probs);
            state.Probs = NULL;
            PyErr_NoMemory();
            goto exit;
        }
    }

    LzmaDecoderInit(&state);
    
    // decompress data
    tmp = (unsigned char *)PyString_AS_STRING(output);
    outsize = 0;
    outavail = blocksize;
    while (1)
    {
        SizeT inProcessed, outProcessed;
        int finishDecoding = 1;
        bufsize = outavail;
        
        Py_BEGIN_ALLOW_THREADS
        if (totallength != -1)
            // We know the total size of the decompressed string
            res = LzmaDecode(&state, data, length, &inProcessed,
                             tmp, outavail > totallength ? totallength : outavail, &outProcessed, finishDecoding);
        else
            // Decompress until EOS marker is reached
            res = LzmaDecode(&state, data, length, &inProcessed,
                             tmp, outavail, &outProcessed, finishDecoding);
        Py_END_ALLOW_THREADS
        
        if (res != 0) {
            PyErr_SetString(PyExc_ValueError, "data error during decompression");
            goto exit;
        }

        length -= inProcessed;
        data += inProcessed;
        outsize += outProcessed;
        tmp += outProcessed;
        outavail -= outProcessed;
        if (totallength != -1)
            totallength -= outProcessed;
        
        if (length > 0 || outProcessed == bufsize) {
            // Target block is full, increase size...
            if (_PyString_Resize(&output, outsize+outavail+BLOCK_SIZE) != 0)
                goto exit;
            
            outavail += BLOCK_SIZE;
            tmp = (unsigned char *)&PyString_AS_STRING(output)[outsize];
        } else
            // Finished decompressing
            break;
    }

    // Decrease length of result to total output size
    if (_PyString_Resize(&output, outsize) != 0)
        goto exit;
    
    result = output;
    output = NULL;
    
exit:
    free_lzma_state(&state);
    Py_XDECREF(output);
    
    return result;
}
