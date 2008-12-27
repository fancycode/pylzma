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

#include "pylzma.h"
#include "pylzma_compress.h"
#include "pylzma_decompress.h"
#include "pylzma_decompressobj.h"
#include "pylzma_compressobj.h"
#include "pylzma_compressfile.h"
#ifdef WITH_COMPAT
#include "pylzma_decompress_compat.h"
#include "pylzma_decompressobj_compat.h"
#endif

#if defined(WITH_THREAD) && !defined(PYLZMA_USE_GILSTATE)
PyInterpreterState* _pylzma_interpreterState = NULL;
#endif

PyMethodDef methods[] = {
    // exported functions
    {"compress",      (PyCFunction)pylzma_compress,      METH_VARARGS | METH_KEYWORDS, (char *)&doc_compress},
    {"decompress",    (PyCFunction)pylzma_decompress,    METH_VARARGS | METH_KEYWORDS, (char *)&doc_decompress},
    // XXX: compression through an object doesn't work, yet
    //{"compressobj",   (PyCFunction)pylzma_compressobj,   METH_VARARGS | METH_KEYWORDS, (char *)&doc_compressobj},
#ifdef WITH_COMPAT
    // compatibility functions
    {"decompress_compat",    (PyCFunction)pylzma_decompress_compat,    METH_VARARGS | METH_KEYWORDS, (char *)&doc_decompress_compat},
    {"decompressobj_compat", (PyCFunction)pylzma_decompressobj_compat, METH_VARARGS,                 (char *)&doc_decompressobj_compat},
#endif
    {NULL, NULL},
};

PyMODINIT_FUNC initpylzma(void)
{
    PyObject *m;

    CDecompressionObject_Type.tp_new = PyType_GenericNew;
    
    if (PyType_Ready(&CDecompressionObject_Type) < 0)
        return;
    
    if (PyType_Ready(&CCompressionFileObject_Type) < 0)
        return;
    
    m = Py_InitModule("pylzma", methods);
    
    Py_INCREF(&CDecompressionObject_Type);
    PyModule_AddObject(m, "decompressobj", (PyObject *)&CDecompressionObject_Type);
    
    Py_INCREF(&CCompressionFileObject_Type);
    PyModule_AddObject(m, "compressfile", (PyObject *)&CCompressionFileObject_Type);
    
    PycString_IMPORT;

#if defined(WITH_THREAD)
    PyEval_InitThreads();

#if !defined(PYLZMA_USE_GILSTATE)
    /* Save the current interpreter, so compressing file objects works. */
    _pylzma_interpreterState = PyThreadState_Get()->interp;
#endif

#endif
}
