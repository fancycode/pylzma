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

#ifndef ___PYLZMA_FILESTREAMS__H___
#define ___PYLZMA_FILESTREAMS__H___

#include <Python.h>
#include <7zip/7zip/IStream.h>
#include <7zip/Common/MyCom.h>

class CFileInStream :
    public IInStream,
    public CMyUnknownImp
{
private:
    PyObject *m_file;

public:
    MY_UNKNOWN_IMP

    CFileInStream(PyObject *file)
    {
        m_file = file;
        Py_XINCREF(m_file);
    }
    
    virtual ~CFileInStream()
    {
        Py_XDECREF(m_file);
    }
    
    STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize)
    {
        return ReadPart(data, size, processedSize);
    }
    
    STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize)
    {
        // read from file-like object
        PyObject *result = PyObject_CallMethod(m_file, "read", "l", size);
        if (result == NULL)
            return E_FAIL;
        
        if (!PyString_Check(result))
        {
            PyObject *str = PyObject_Str(result);
            Py_XDECREF(result);
            if (str == NULL)
                return E_FAIL;
            result = str;
        }
        
        memcpy(data, PyString_AS_STRING(result), PyString_Size(result));
        if (processedSize)
            *processedSize = PyString_Size(result);
        
        Py_XDECREF(result);
        return S_OK;
    }    

    STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition)
    {
        PyObject *result = PyObject_CallMethod(m_file, "seek", "Ki", offset, seekOrigin);
        if (result == NULL)
            return E_FAIL;

        Py_XDECREF(result);
        if (newPosition)
        {
            result = PyObject_CallMethod(m_file, "tell", "");
            if (result == NULL)
                return E_FAIL;
            *newPosition = PyLong_AsLongLong(result);
            Py_XDECREF(result);
        }
        
        return S_OK;
    }
};

#endif
