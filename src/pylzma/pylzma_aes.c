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

#include "../7zip/C/Aes.h"
#include "../sdk/Types.h"

typedef struct {
    PyObject_HEAD
    int offset;
    UInt32 aes[AES_NUM_IVMRK_WORDS+3];
} CAESDecryptObject;

int
aesdecrypt_init(CAESDecryptObject *self, PyObject *args, PyObject *kwargs)
{
    char *key=NULL;
    int keylength=0;
    char *iv=NULL;
    int ivlength=0;
    
    // possible keywords for this function
    static char *kwlist[] = {"key", "iv", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s#s#", kwlist, &key, &keylength, &iv, &ivlength))
        return -1;
    
    self->offset = ((0 - (unsigned)(ptrdiff_t)self->aes) & 0xF) / sizeof(UInt32);
    if (keylength > 0) {
        if (keylength != 16 && keylength != 24 && keylength != 32) {
            PyErr_Format(PyExc_TypeError, "key must be 16, 24 or 32 bytes, got %d", keylength);
            return -1;
        }

        Aes_SetKey_Dec(self->aes + self->offset + 4, (Byte *) key, keylength); 
    }
    if (ivlength > 0) {
        if (ivlength != AES_BLOCK_SIZE) {
            PyErr_Format(PyExc_TypeError, "iv must be %d bytes, got %d", AES_BLOCK_SIZE, ivlength);
            return -1;
        }

        AesCbc_Init(self->aes + self->offset, (Byte *) iv); 
    }
    return 0;
}

static char 
doc_aesdecrypt_decrypt[] = \
    "decrypt(data) -- Decrypt given data.";

static PyObject *
aesdecrypt_decrypt(CAESDecryptObject *self, PyObject *args)
{
    char *data;
    int length;
    PyObject *result;
    char *out;
    int outlength;
    
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    
    if (length % AES_BLOCK_SIZE) {
        PyErr_Format(PyExc_TypeError, "data must be a multiple of %d bytes, got %d", AES_BLOCK_SIZE, length);
        return NULL;
    }
    
    result = PyString_FromStringAndSize(data, length);
    out = PyString_AS_STRING(result);
    outlength = PyString_Size(result);
    Py_BEGIN_ALLOW_THREADS
    g_AesCbc_Decode(self->aes + self->offset, (Byte *) out, outlength / AES_BLOCK_SIZE);
    Py_END_ALLOW_THREADS
    return result;
}

PyMethodDef
aesdecrypt_methods[] = {
    {"decrypt",     (PyCFunction)aesdecrypt_decrypt,    METH_VARARGS,  doc_aesdecrypt_decrypt},
    {NULL, NULL},
};

PyTypeObject
CAESDecrypt_Type = {
    //PyObject_HEAD_INIT(&PyType_Type)
    PyObject_HEAD_INIT(NULL)
    0,
    "pylzma.AESDecrypt",                 /* char *tp_name; */
    sizeof(CAESDecryptObject),           /* int tp_basicsize; */
    0,                                   /* int tp_itemsize;       // not used much */
    NULL,                                /* destructor tp_dealloc; */
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
    "AES decryption class",                 /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    aesdecrypt_methods,                  /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)aesdecrypt_init,           /* tp_init */
    0,                                   /* tp_alloc */
    0,                                   /* tp_new */
};
