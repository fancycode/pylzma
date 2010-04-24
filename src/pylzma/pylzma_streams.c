#include "pylzma.h"
#include "pylzma_streams.h"

static SRes
MemoryInStream_Read(void *p, void *buf, size_t *size)
{
    CMemoryInStream *self = (CMemoryInStream *) p;
    size_t toread = *size;
    if (toread > self->avail) {
        toread = self->avail;
    }
    memcpy(buf, self->data, toread);
    self->data += toread;
    self->avail -= toread;
    *size = toread;
    return SZ_OK;
}

void
CreateMemoryInStream(CMemoryInStream *stream, Byte *data, size_t size)
{
    stream->s.Read = MemoryInStream_Read;
    stream->data = data;
    stream->avail = size;
}

static SRes
PythonInStream_Read(void *p, void *buf, size_t *size)
{
    CPythonInStream *self = (CPythonInStream *) p;
    size_t toread = *size;
    PyObject *data;
    SRes res;
    
    START_BLOCK_THREADS
    data = PyObject_CallMethod(self->file, "read", "i", toread);
    if (data == NULL) {
        PyErr_Print();
        res = SZ_ERROR_READ;
    } else if (!PyString_Check(data)) {
        res = SZ_ERROR_READ;
    } else {
        *size = PyString_GET_SIZE(data);
        memcpy(buf, PyString_AS_STRING(data), min(*size, toread));
        res = SZ_OK;
    }
    Py_XDECREF(data);    
    END_BLOCK_THREADS
    return res;
}

void
CreatePythonInStream(CPythonInStream *stream, PyObject *file)
{
    stream->s.Read = PythonInStream_Read;
    stream->file = file;
}

static size_t
MemoryOutStream_Write(void *p, const void *buf, size_t size)
{
    CMemoryOutStream *self = (CMemoryOutStream *) p;
    self->data = (Byte *) realloc(self->data, self->size + size);
    if (self->data == NULL) {
        self->size = 0;
        return 0;
    }
    memcpy(self->data + self->size, buf, size);
    self->size += size;
    return size;
}

void
CreateMemoryOutStream(CMemoryOutStream *stream)
{
    stream->s.Write = MemoryOutStream_Write;
    stream->data = NULL;
    stream->size = 0;
}
