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

#include "pylzma.h"
#include "pylzma_compress.h"
#include "pylzma_decompress.h"
#include "pylzma_decompressobj.h"
#include "pylzma_compressobj.h"
#include "pylzma_compressfile.h"

static void insint(PyObject *d, char *name, int value)
{
    PyObject *v = PyInt_FromLong((long) value);
    if (!v || PyDict_SetItemString(d, name, v))
        PyErr_Clear();

    Py_XDECREF(v);
}

PyMethodDef methods[] = {
    // exported functions
    {"compress",      (PyCFunction)pylzma_compress,      METH_VARARGS | METH_KEYWORDS, (char *)&doc_compress},
    {"decompress",    (PyCFunction)pylzma_decompress,    METH_VARARGS | METH_KEYWORDS, (char *)&doc_decompress},
    // XXX: compression through an object doesn't work, yet
    //{"compressobj",   (PyCFunction)pylzma_compressobj,   METH_VARARGS | METH_KEYWORDS, (char *)&doc_compressobj},
    {"decompressobj", (PyCFunction)pylzma_decompressobj, METH_VARARGS,                 (char *)&doc_decompressobj},
    {"compressfile",  (PyCFunction)pylzma_compressfile,  METH_VARARGS | METH_KEYWORDS, (char *)&doc_compressfile},
    {NULL, NULL},
};

DL_EXPORT(void) initpylzma(void)
{
    PyObject *m, *d;
    
    m = Py_InitModule("pylzma", methods);
    d = PyModule_GetDict(m);
    PycString_IMPORT;
}
