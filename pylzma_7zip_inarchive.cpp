/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004 by Joachim Bauch, mail@joachim-bauch.de
 * LZMA SDK Copyright (C) 1999-2004 Igor Pavlov
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

#include <stdio.h>
#include "Platform.h"
#include "pylzma.h"
#include "pylzma_7zip_inarchive.h"
#include "pylzma_filestreams.h"
#include <7zip/7zip/Archive/7z/7zHandler.h>
#include <7zip/Common/MyCom.h>

class COpenCallbacks;

typedef struct {
    PyObject_HEAD
    NArchive::N7z::CHandler *handler;
    CFileInStream *inStream;
    COpenCallbacks *callbacks;
    PyObject *inFile;
} C7ZipInArchiveObject;

class COpenCallbacks :
    public IArchiveOpenCallback,
    public CMyUnknownImp
{
private:
    C7ZipInArchiveObject *m_object;

public:
    MY_UNKNOWN_IMP

    COpenCallbacks(C7ZipInArchiveObject *object)
    {
        m_object = object;
        Py_XINCREF(m_object);
    }
    
    virtual ~COpenCallbacks()
    {
        Py_XDECREF(m_object);
    }
    
    STDMETHOD(SetTotal)(const UINT64 *files, const UINT64 *bytes)
    {
        if (files)
            printf("TotalFiles: %d\n", *files);
        if (bytes)
            printf("TotalBytes: %d\n", *bytes);
        return S_OK;
    }

    STDMETHOD(SetCompleted)(const UINT64 *files, const UINT64 *bytes)
    {
        if (files)
            printf("CompletedFiles: %d\n", *files);
        if (bytes)
            printf("CompletedBytes: %d\n", *bytes);
        return S_OK;
    }
};

static PyObject *pylzma_7zip_inarchive_getcount(C7ZipInArchiveObject *self, PyObject *args)
{
    UINT32 count;

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    
    self->handler->GetNumberOfItems(&count);
    return Py_BuildValue("i", count);
}

static PyObject *pylzma_7zip_inarchive_getpropcount(C7ZipInArchiveObject *self, PyObject *args)
{
    UINT32 count;

    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    
    self->handler->GetNumberOfProperties(&count);
    return Py_BuildValue("i", count);
}

PyMethodDef pylzma_7zip_inarchive_methods[] = {
    {"GetNumberOfItems",      (PyCFunction)pylzma_7zip_inarchive_getcount, METH_VARARGS},
    {"GetNumberOfProperties", (PyCFunction)pylzma_7zip_inarchive_getpropcount, METH_VARARGS},
    {NULL, NULL},
};

static void pylzma_7zip_inarchive_dealloc(C7ZipInArchiveObject *self)
{
    self->handler->Close();
    DEC_AND_NULL(self->inFile);
    DELETE_AND_NULL(self->handler);
    DELETE_AND_NULL(self->inStream);
    DELETE_AND_NULL(self->callbacks);
    PyObject_Del(self);
}

static PyObject *pylzma_7zip_inarchive_getattr(C7ZipInArchiveObject *self, char *attrname)
{
    return Py_FindMethod(pylzma_7zip_inarchive_methods, (PyObject *)self, attrname);
}

static int pylzma_7zip_inarchive_setattr(C7ZipInArchiveObject *self, char *attrname, PyObject *value)
{
    // disable setting of attributes
    PyErr_Format(PyExc_AttributeError, "no attribute named '%s'", attrname);
    return -1;
}

PyTypeObject C7ZipInArchive_Type = {
  //PyObject_HEAD_INIT(&PyType_Type)
  PyObject_HEAD_INIT(NULL)
  0,
  "7zInArchive",                        /* char *tp_name; */
  sizeof(C7ZipInArchiveObject),       /* int tp_basicsize; */
  0,                                    /* int tp_itemsize;       // not used much */
  (destructor)pylzma_7zip_inarchive_dealloc,  /* destructor tp_dealloc; */
  NULL,                                 /* printfunc  tp_print;   */
  (getattrfunc)pylzma_7zip_inarchive_getattr, /* getattrfunc  tp_getattr; // __getattr__ */
  (setattrfunc)pylzma_7zip_inarchive_setattr, /* setattrfunc  tp_setattr;  // __setattr__ */
  NULL,                                 /* cmpfunc  tp_compare;  // __cmp__ */
  NULL,                                 /* reprfunc  tp_repr;    // __repr__ */
  NULL,                                 /* PyNumberMethods *tp_as_number; */
  NULL,                                 /* PySequenceMethods *tp_as_sequence; */
  NULL,                                 /* PyMappingMethods *tp_as_mapping; */
  NULL,                                 /* hashfunc tp_hash;     // __hash__ */
  NULL,                                 /* ternaryfunc tp_call;  // __call__ */
  NULL,                                 /* reprfunc tp_str;      // __str__ */
};

#ifdef __cplusplus
extern "C"
#endif
const char doc_7zip_inarchive[] = \
    "InArchive(file) -- Returns archive object.";

#ifdef __cplusplus
extern "C" {
#endif

PyObject *pylzma_7zip_inarchive(PyObject *self, PyObject *args)
{
    C7ZipInArchiveObject *result=NULL;
    PyObject *inFile;
    NArchive::N7z::CHandler *handler;
    PY_LONG_LONG max_check=1024;

    if (!PyArg_ParseTuple(args, "O|K", &inFile, &max_check))
        return NULL;

    if (!PyObject_HasAttrString(inFile, "read"))
    {
        PyErr_SetString(PyExc_ValueError, "file-like object must provide a method 'read'");
        return NULL;
    }

    Py_XINCREF(inFile);
    handler = new NArchive::N7z::CHandler();
    CHECK_NULL(handler);

    result = PyObject_New(C7ZipInArchiveObject, &C7ZipInArchive_Type);
    if (result == NULL)
    {
        Py_XDECREF(inFile);
        DELETE_AND_NULL(handler);
        PyErr_NoMemory();
        goto exit;
    }
    result->handler = handler;
    result->inFile = inFile;
    result->callbacks = new COpenCallbacks(result);
    result->inStream = new CFileInStream(inFile);
    if (result->inStream == NULL)
    {
        DEC_AND_NULL(result->inFile);
        DELETE_AND_NULL(handler);
        DEC_AND_NULL(result);
        PyErr_NoMemory();
        goto exit;
    }

    result->handler->Open(result->inStream, (const UINT64 *)&max_check, result->callbacks);

exit:
    return (PyObject *)result;
}

#ifdef __cplusplus
}
#endif
