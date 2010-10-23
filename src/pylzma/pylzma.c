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

#include "../sdk/7zVersion.h"
#include "../7zip/C/Sha256.h"
#include "../7zip/C/Aes.h"

#include "pylzma.h"
#include "pylzma_compress.h"
#include "pylzma_decompress.h"
#include "pylzma_decompressobj.h"
#if 0
#include "pylzma_compressobj.h"
#endif
#include "pylzma_compressfile.h"
#include "pylzma_aes.h"
#ifdef WITH_COMPAT
#include "pylzma_decompress_compat.h"
#include "pylzma_decompressobj_compat.h"
#endif

#if defined(WITH_THREAD) && !defined(PYLZMA_USE_GILSTATE)
PyInterpreterState* _pylzma_interpreterState = NULL;
#endif

const char
doc_calculate_key[] = \
    "calculate_key(password, cycles, salt=None, digest='sha256') -- Calculate decryption key.";

static PyObject *
pylzma_calculate_key(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *password;
    int pwlen;
    int cycles;
    PyObject *pysalt=NULL;
    char *salt;
    int saltlen;
    char *digest="sha256";
    char key[32];
    // possible keywords for this function
    static char *kwlist[] = {"password", "cycles", "salt", "digest", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#i|Os", kwlist, &password, &pwlen, &cycles, &pysalt, &digest))
        return NULL;
    
    if (pysalt == Py_None) {
        pysalt = NULL;
    } else if (!PyString_Check(pysalt)) {
        PyErr_Format(PyExc_TypeError, "salt must be a string, got a %s", pysalt->ob_type->tp_name);
        return NULL;
    }
    
    if (strcmp(digest, "sha256") != 0) {
        PyErr_Format(PyExc_TypeError, "digest %s is unsupported", digest);
        return NULL;
    }
    
    if (pysalt != NULL) {
        salt = PyString_AS_STRING(pysalt);
        saltlen = PyString_Size(pysalt);
    } else {
        salt = NULL;
        saltlen = 0;
    }
    
    if (cycles == 0x3f) {
        int pos;
        int i;
        for (pos = 0; pos < saltlen; pos++)
            key[pos] = salt[pos];
        for (i = 0; i<pwlen && pos < 32; i++)
            key[pos++] = password[i];
        for (; pos < 32; pos++)
            key[pos] = 0;
    } else {
        Py_BEGIN_ALLOW_THREADS
        CSha256 sha;
        Sha256_Init(&sha);
        long round;
        int i;
        long rounds = (long) 1 << cycles;
        unsigned char temp[8] = { 0,0,0,0,0,0,0,0 };
        for (round = 0; round < rounds; round++) {
            Sha256_Update(&sha, (Byte *) salt, saltlen);
            Sha256_Update(&sha, (Byte *) password, pwlen);
            Sha256_Update(&sha, (Byte *) temp, 8);
            for (i = 0; i < 8; i++)
                if (++(temp[i]) != 0)
                    break;
        }
        Sha256_Final(&sha, (Byte *) &key);
        Py_END_ALLOW_THREADS
    }
    
    return PyString_FromStringAndSize(key, 32);
}

PyMethodDef
methods[] = {
    // exported functions
    {"compress",      (PyCFunction)pylzma_compress,      METH_VARARGS | METH_KEYWORDS, (char *)&doc_compress},
    {"decompress",    (PyCFunction)pylzma_decompress,    METH_VARARGS | METH_KEYWORDS, (char *)&doc_decompress},
#ifdef WITH_COMPAT
    // compatibility functions
    {"decompress_compat",    (PyCFunction)pylzma_decompress_compat,    METH_VARARGS | METH_KEYWORDS, (char *)&doc_decompress_compat},
    {"decompressobj_compat", (PyCFunction)pylzma_decompressobj_compat, METH_VARARGS,                 (char *)&doc_decompressobj_compat},
#endif
    {"calculate_key",   (PyCFunction)pylzma_calculate_key,  METH_VARARGS | METH_KEYWORDS,   (char *)&doc_calculate_key},
    {NULL, NULL},
};

static void *Alloc(void *p, size_t size) { p = p; return malloc(size); }
static void Free(void *p, void *address) { p = p; free(address); }
ISzAlloc allocator = { Alloc, Free };

PyMODINIT_FUNC
initpylzma(void)
{
    PyObject *m;

    CDecompressionObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CDecompressionObject_Type) < 0)
        return;
#if 0
    CCompressionObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CCompressionObject_Type) < 0)
        return;
#endif
    CCompressionFileObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CCompressionFileObject_Type) < 0)
        return;

    CAESDecrypt_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CAESDecrypt_Type) < 0)
        return;

    m = Py_InitModule("pylzma", methods);

    Py_INCREF(&CDecompressionObject_Type);
    PyModule_AddObject(m, "decompressobj", (PyObject *)&CDecompressionObject_Type);
#if 0
    Py_INCREF(&CCompressionObject_Type);
    PyModule_AddObject(m, "compressobj", (PyObject *)&CCompressionObject_Type);
#endif   
    Py_INCREF(&CCompressionFileObject_Type);
    PyModule_AddObject(m, "compressfile", (PyObject *)&CCompressionFileObject_Type);

    Py_INCREF(&CAESDecrypt_Type);
    PyModule_AddObject(m, "AESDecrypt", (PyObject *)&CAESDecrypt_Type);

    PyModule_AddIntConstant(m, "SDK_VER_MAJOR", MY_VER_MAJOR);
    PyModule_AddIntConstant(m, "SDK_VER_MINOR", MY_VER_MINOR);
    PyModule_AddIntConstant(m, "SDK_VER_BUILD ", MY_VER_BUILD);
    PyModule_AddStringConstant(m, "SDK_VERSION", MY_VERSION);
    PyModule_AddStringConstant(m, "SDK_DATE", MY_DATE);
    PyModule_AddStringConstant(m, "SDK_COPYRIGHT", MY_COPYRIGHT);
    PyModule_AddStringConstant(m, "SDK_VERSION_COPYRIGHT_DATE", MY_VERSION_COPYRIGHT_DATE);

    AesGenTables();
    pylzma_init_compfile();

#if defined(WITH_THREAD)
    PyEval_InitThreads();

#if !defined(PYLZMA_USE_GILSTATE)
    /* Save the current interpreter, so compressing file objects works. */
    _pylzma_interpreterState = PyThreadState_Get()->interp;
#endif

#endif
}
