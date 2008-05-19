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

#ifndef ___PYLZMA_STREAMS__H___
#define ___PYLZMA_STREAMS__H___

#include <Python.h>
#include <7zip/Common/MyWindows.h>
#include <7zip/7zip/IStream.h>
#include <7zip/Common/MyCom.h>

class CInStream :
    public ISequentialInStream,
    public CMyUnknownImp
{
private:
    BYTE *next_in;
    UINT32 avail_in;
    BYTE *origin;
    UINT32 original_size;
    UINT32 free_space;
    bool allocated;
    PyObject *sourceFile;

public:
    MY_UNKNOWN_IMP

    CInStream(BYTE *data, UINT32 length)
    {
        SetData(data, length);
        allocated = false;
    }
    
    CInStream()
    {
        SetData(NULL, 0);
    }

    CInStream(PyObject *source)
    {
        SetData(NULL, 0);
        sourceFile = source;
    }

    virtual ~CInStream()
    {
        if (allocated)
            free(origin);
    }
    
    void SetData(BYTE *data, UINT32 length)
    {
        sourceFile = NULL;
        origin = data;
        next_in = data;
        avail_in = length;
        original_size = length;
        free_space = 0;
    }

    int getAvailIn() { return avail_in; }
    
    bool AppendData(BYTE *data, UINT32 length)
    {
        void *insert;
        if (origin == NULL || free_space < (UINT32)length) {
            // we need to resize the temporary buffer
            int add = length - free_space;
            int ofs = next_in - origin;
            origin = (BYTE *)realloc(origin, original_size + add);
            if (origin == NULL)
                return false;
            allocated = true;
            free_space += add;
            original_size += add;
            next_in = &origin[ofs];
            insert = &origin[avail_in];
        } else
            insert = &next_in[avail_in];
        memcpy(insert, data, length);
        avail_in += length;
        free_space -= length;
        return true;
    }
    
    STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize)
    {
        return ReadPart(data, size, processedSize);
    }
    
    STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize)
    {
        if (sourceFile)
        {
            // read from file-like object
            HRESULT res = E_FAIL;
            // we might be calling into Python code from outside the main thread, so block threads...
            START_BLOCK_THREADS
            PyObject *result = PyObject_CallMethod(sourceFile, "read", "l", size);
            if (result == NULL)
                goto exit;
            
            if (!PyString_Check(result))
            {
                PyObject *str = PyObject_Str(result);
                Py_XDECREF(result);
                if (str == NULL)
                    goto exit;
                result = str;
            }
            
            memcpy(data, PyString_AS_STRING(result), PyString_Size(result));
            if (processedSize)
                *processedSize = PyString_Size(result);
            
            Py_XDECREF(result);
            res = S_OK;
            
exit:
            END_BLOCK_THREADS
            return res;
        }
        
        if (processedSize)
            *processedSize = 0;
        
        while (size)
        {
            if (!avail_in)
                return S_OK;
            
            UINT32 len = size < avail_in ? size : avail_in;
            memcpy(data, next_in, len);
            avail_in -= len;
            size -= len;
            next_in += len;

            if (allocated)
            {
                memmove(origin, next_in, avail_in);
                next_in = origin;
                free_space += len;
            }

            data = (BYTE *)(data) + len;
            if (processedSize)
                *processedSize += len;
        }
        return S_OK;
    }    
};

class COutStream :
    public ISequentialOutStream,
    public CMyUnknownImp
{
private:
    BYTE *buffer;
    BYTE *next_out;
    UINT32 avail_out;
    UINT32 count;
    UINT32 readpos;

public:
    MY_UNKNOWN_IMP

    COutStream()
    {
        buffer = (BYTE *)malloc(BLOCK_SIZE);
        next_out = buffer;
        avail_out = BLOCK_SIZE;
        count = 0;
        readpos = 0;
    }
    
    virtual ~COutStream()
    {
        if (buffer)
            free(buffer);
        buffer = NULL;
    }
    
    char *getData()
    {
        return (char *)buffer;
    }
    
    int getLength()
    {
        return count;
    }

    int getMaxRead()
    {
        return count - readpos;
    }
    
    void Read(void *dest, UINT32 size)
    {
        memcpy(dest, &buffer[readpos], size);
        increaseReadPos(size);
    }
    
    void *getReadPtr()
    {
        return &buffer[readpos];
    }
    
    void increaseReadPos(UINT32 count)
    {
        readpos += count;
    }
    
    STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize)
    {
        return WritePart(data, size, processedSize);
    }
    
    STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize)
    {
        if (processedSize)
            *processedSize = 0;
        
        while (size)
        {
            if (!avail_out)
            {
                buffer = (BYTE *)realloc(buffer, count + BLOCK_SIZE);
                avail_out += BLOCK_SIZE;
                next_out = &buffer[count];
            }
            
            UINT32 len = size < avail_out ? size : avail_out;
            memcpy(next_out, data, len);
            avail_out -= len;
            size -= len;
            next_out += len;
            count += len;
            data = (BYTE *)(data) + len;
            if (processedSize)
                *processedSize += len;
        }
        
        return S_OK;
    }    
};

#endif
