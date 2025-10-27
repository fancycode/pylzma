/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2015 by Joachim Bauch, mail@joachim-bauch.de
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

#include "../sdk/C/7zVersion.h"
#include "../sdk/C/Sha256.h"
#include "../sdk/C/Aes.h"
#include "../sdk/C/Bra.h"
#include "../sdk/C/Bcj2.h"
#include "../sdk/C/Delta.h"
#include "../sdk/C/Ppmd7.h"

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
#include "pylzma_streams.h"

#if defined(WITH_THREAD) && !defined(PYLZMA_USE_GILSTATE)
PyInterpreterState* _pylzma_interpreterState = NULL;
#endif

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

const char
doc_calculate_key[] = \
    "calculate_key(password, cycles, salt=None, digest='sha256') -- Calculate decryption key.";

static PyObject *
pylzma_calculate_key(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *password;
    PARSE_LENGTH_TYPE pwlen;
    int cycles;
    PyObject *pysalt=NULL;
    char *salt;
    Py_ssize_t saltlen;
    char *digest="sha256";
    char key[32];
    // possible keywords for this function
    static char *kwlist[] = {"password", "cycles", "salt", "digest", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#i|Os", kwlist, &password, &pwlen, &cycles, &pysalt, &digest))
        return NULL;

    if (pysalt == Py_None) {
        pysalt = NULL;
    } else if (!PyBytes_Check(pysalt)) {
        PyErr_Format(PyExc_TypeError, "salt must be a string, got a %s", pysalt->ob_type->tp_name);
        return NULL;
    }

    if (strcmp(digest, "sha256") != 0) {
        PyErr_Format(PyExc_TypeError, "digest %s is unsupported", digest);
        return NULL;
    }

    if (pysalt != NULL) {
        salt = PyBytes_AS_STRING(pysalt);
        saltlen = PyBytes_Size(pysalt);
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
        CSha256 sha;
        long round;
        int i;
        long rounds = (long) 1 << cycles;
        unsigned char temp[8] = { 0,0,0,0,0,0,0,0 };
        Py_BEGIN_ALLOW_THREADS
        Sha256_Init(&sha);
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

    return PyBytes_FromStringAndSize(key, 32);
}

const char
doc_bcj_x86_convert[] = \
    "bcj_x86_convert(data) -- Perform BCJ x86 conversion.";

static PyObject *
pylzma_bcj_x86_convert(PyObject *self, PyObject *args)
{
    char *data;
    PARSE_LENGTH_TYPE length;
    int encoding=0;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s#|i", &data, &length, &encoding)) {
        return NULL;
    }

    if (!length) {
        return PyBytes_FromString("");
    }

    result = PyBytes_FromStringAndSize(data, length);
    if (result != NULL) {
        UInt32 state;
        Py_BEGIN_ALLOW_THREADS
        x86_Convert_Init(state);
        x86_Convert((Byte *) PyBytes_AS_STRING(result), length, 0, &state, encoding);
        Py_END_ALLOW_THREADS
    }

    return result;
}

#define DEFINE_BCJ_CONVERTER(id, name) \
const char \
doc_bcj_##id##_convert[] = \
    "bcj_" #id "_convert(data) -- Perform BCJ " #name " conversion."; \
\
static PyObject * \
pylzma_bcj_##id##_convert(PyObject *self, PyObject *args) \
{ \
    char *data; \
    PARSE_LENGTH_TYPE length; \
    int encoding=0; \
    PyObject *result; \
     \
    if (!PyArg_ParseTuple(args, "s#|i", &data, &length, &encoding)) { \
        return NULL; \
    } \
     \
    if (!length) { \
        return PyBytes_FromString(""); \
    } \
     \
    result = PyBytes_FromStringAndSize(data, length); \
    if (result != NULL) { \
        Py_BEGIN_ALLOW_THREADS \
        name##_Convert((Byte *) PyBytes_AS_STRING(result), length, 0, encoding); \
        Py_END_ALLOW_THREADS \
    } \
     \
    return result; \
}

DEFINE_BCJ_CONVERTER(arm, ARM);
DEFINE_BCJ_CONVERTER(armt, ARMT);
DEFINE_BCJ_CONVERTER(ppc, PPC);
DEFINE_BCJ_CONVERTER(sparc, SPARC);
DEFINE_BCJ_CONVERTER(ia64, IA64);

const char
doc_bcj2_decode[] =
    "bcj2_decode(main_data, call_data, jump_data, rc_data[, dest_len]) -- Decode BCJ2 streams.";

static PyObject *
pylzma_bcj2_decode(PyObject *self, PyObject *args)
{
    char *main_data, *call_data, *jump_data, *rc_data;
    PARSE_LENGTH_TYPE main_length, call_length, jump_length, rc_length;
    PARSE_LENGTH_TYPE dest_len = -1;
    CBcj2Dec dec;
    SRes res;
    PyObject *result;
    int i;

    if (!PyArg_ParseTuple(args, "s#s#s#s#|n", &main_data, &main_length,
        &call_data, &call_length, &jump_data, &jump_length,
        &rc_data, &rc_length, &dest_len)) {
        return NULL;
    }
    if (dest_len == -1) {
        dest_len = main_length;
    }

    if (!dest_len) {
        return PyBytes_FromString("");
    }

    result = PyBytes_FromStringAndSize(NULL, dest_len);
    if (!result) {
        return NULL;
    }

    memset(&dec, 0, sizeof(dec));
    dec.bufs[BCJ2_STREAM_MAIN] = (const Byte *) main_data;
    dec.lims[BCJ2_STREAM_MAIN] = (const Byte *) (main_data + main_length);
    dec.bufs[BCJ2_STREAM_CALL] = (const Byte *) call_data;
    dec.lims[BCJ2_STREAM_CALL] = (const Byte *) (call_data + call_length);
    dec.bufs[BCJ2_STREAM_JUMP] = (const Byte *) jump_data;
    dec.lims[BCJ2_STREAM_JUMP] = (const Byte *) (jump_data + jump_length);
    dec.bufs[BCJ2_STREAM_RC] = (const Byte *) rc_data;
    dec.lims[BCJ2_STREAM_RC] = (const Byte *) (rc_data + rc_length);

    dec.dest = (Byte *) PyBytes_AS_STRING(result);
    dec.destLim = dec.dest + dest_len;
    Bcj2Dec_Init(&dec);

    Py_BEGIN_ALLOW_THREADS
    res = Bcj2Dec_Decode(&dec);
    Py_END_ALLOW_THREADS
    if (res != SZ_OK) {
        goto error;
    }

    // All input streams must have been processed.
    for (i = 0; i < 4; i++) {
        if (dec.bufs[i] != dec.lims[i]) {
            goto error;
        }
    }

    // Conversion must be finished and the output buffer filled completely.
    if (!Bcj2Dec_IsFinished(&dec)) {
        goto error;
    } else if (dec.dest != dec.destLim || dec.state != BCJ2_STREAM_MAIN) {
        goto error;
    }
    return result;

error:
    Py_DECREF(result);
    PyErr_SetString(PyExc_TypeError, "bcj2 decoding failed");
    return NULL;
}

const char
doc_delta_decode[] =
    "delta_decode(data, delta) -- Decode Delta streams.";

static PyObject *
pylzma_delta_decode(PyObject *self, PyObject *args)
{
    char *data;
    PARSE_LENGTH_TYPE length;
    unsigned int delta;
    Byte state[DELTA_STATE_SIZE];
    Byte *tmp;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s#I", &data, &length, &delta)) {
        return NULL;
    }

    if (!delta) {
        PyErr_SetString(PyExc_TypeError, "delta must be non-zero");
        return NULL;
    }

    if (!length) {
        return PyBytes_FromString("");
    }

    result = PyBytes_FromStringAndSize(data, length);
    if (!result) {
        return NULL;
    }

    Delta_Init(state);
    tmp = (Byte *) PyBytes_AS_STRING(result);
    Py_BEGIN_ALLOW_THREADS
    Delta_Decode(state, delta, tmp, length);
    Py_END_ALLOW_THREADS
    return result;
}

const char
doc_delta_encode[] =
    "delta_encode(data, delta) -- Encode Delta streams.";

static PyObject *
pylzma_delta_encode(PyObject *self, PyObject *args)
{
    char *data;
    PARSE_LENGTH_TYPE length;
    unsigned int delta;
    Byte state[DELTA_STATE_SIZE];
    Byte *tmp;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s#I", &data, &length, &delta)) {
        return NULL;
    }

    if (!delta) {
        PyErr_SetString(PyExc_TypeError, "delta must be non-zero");
        return NULL;
    }

    if (!length) {
        return PyBytes_FromString("");
    }

    result = PyBytes_FromStringAndSize(data, length);
    if (!result) {
        return NULL;
    }

    Delta_Init(state);
    tmp = (Byte *) PyBytes_AS_STRING(result);
    Py_BEGIN_ALLOW_THREADS
    Delta_Encode(state, delta, tmp, length);
    Py_END_ALLOW_THREADS
    return result;
}

const char
doc_ppmd_decompress[] =
    "ppmd_decompress(data, properties, outsize) -- Decompress PPMd stream.";

typedef struct
{
    IByteIn vt;
    const Byte *cur;
    const Byte *end;
    const Byte *begin;
    UInt64 processed;
    BoolInt extra;
    SRes res;
    const ILookInStream *inStream;
} CByteInToLook;

static Byte
ReadByte(const IByteIn *pp) {
    CByteInToLook *p = CONTAINER_FROM_VTBL(pp, CByteInToLook, vt);
    if (p->cur != p->end) {
        return *p->cur++;
    }

    if (p->res == SZ_OK) {
        size_t size = p->cur - p->begin;
        p->processed += size;
        p->res = ILookInStream_Skip(p->inStream, size);
        size = (1 << 25);
        p->res = ILookInStream_Look(p->inStream, (const void **)&p->begin, &size);
        p->cur = p->begin;
        p->end = p->begin + size;
        if (size != 0) {
            return *p->cur++;;
        }
    }
    p->extra = True;
    return 0;
}

static PyObject *
pylzma_ppmd_decompress(PyObject *self, PyObject *args)
{
    char *data;
    PARSE_LENGTH_TYPE length;
    char *props;
    PARSE_LENGTH_TYPE propssize;
    unsigned int outsize;
    PyObject *result;
    Byte *tmp;
    unsigned order;
    UInt32 memSize;
    CPpmd7 ppmd;
    CPpmd7z_RangeDec rc;
    CByteInToLook s;
    SRes res = SZ_OK;
    CMemoryLookInStream stream;

    if (!PyArg_ParseTuple(args, "s#s#I", &data, &length, &props, &propssize, &outsize)) {
        return NULL;
    }

    if (propssize != 5) {
        PyErr_Format(PyExc_TypeError, "properties must be exactly 5 bytes, got %ld", propssize);
        return NULL;
    }

    order = props[0];
    memSize = GetUi32(props + 1);
    if (order < PPMD7_MIN_ORDER ||
        order > PPMD7_MAX_ORDER ||
        memSize < PPMD7_MIN_MEM_SIZE ||
        memSize > PPMD7_MAX_MEM_SIZE) {
        PyErr_SetString(PyExc_TypeError, "unsupporter compression properties");
        return NULL;
    }

    if (!outsize) {
        return PyBytes_FromString("");
    }

    Ppmd7_Construct(&ppmd);
    if (!Ppmd7_Alloc(&ppmd, memSize, &allocator)) {
        return PyErr_NoMemory();
    }
    Ppmd7_Init(&ppmd, order);

    result = PyBytes_FromStringAndSize(NULL, outsize);
    if (!result) {
        return NULL;
    }

    CreateMemoryLookInStream(&stream, (Byte*) data, length);
    tmp = (Byte *) PyBytes_AS_STRING(result);
    Py_BEGIN_ALLOW_THREADS
    Ppmd7z_RangeDec_CreateVTable(&rc);
    s.vt.Read = ReadByte;
    s.inStream = &stream.s;
    s.begin = s.end = s.cur = NULL;
    s.extra = False;
    s.res = SZ_OK;
    s.processed = 0;
    rc.Stream = &s.vt;
    if (!Ppmd7z_RangeDec_Init(&rc)) {
        res = SZ_ERROR_DATA;
    } else if (s.extra) {
        res = (s.res != SZ_OK ? s.res : SZ_ERROR_DATA);
    } else {
        SizeT i;
        for (i = 0; i < outsize; i++) {
            int sym = Ppmd7_DecodeSymbol(&ppmd, &rc.vt);
            if (s.extra || sym < 0) {
                break;
            }
            tmp[i] = (Byte)sym;
        }
        if (i != outsize) {
            res = (s.res != SZ_OK ? s.res : SZ_ERROR_DATA);
        } else if (s.processed + (s.cur - s.begin) != length || !Ppmd7z_RangeDec_IsFinishedOK(&rc)) {
            res = SZ_ERROR_DATA;
        }
    }
    Py_END_ALLOW_THREADS
    Ppmd7_Free(&ppmd, &allocator);
    if (res != SZ_OK) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_TypeError, "error during decompression");
        result = NULL;
    }
    return result;
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
    // BCJ
    {"bcj_x86_convert",     (PyCFunction)pylzma_bcj_x86_convert,    METH_VARARGS,   (char *)&doc_bcj_x86_convert},
    {"bcj_arm_convert",     (PyCFunction)pylzma_bcj_arm_convert,    METH_VARARGS,   (char *)&doc_bcj_arm_convert},
    {"bcj_armt_convert",    (PyCFunction)pylzma_bcj_armt_convert,   METH_VARARGS,   (char *)&doc_bcj_armt_convert},
    {"bcj_ppc_convert",     (PyCFunction)pylzma_bcj_ppc_convert,    METH_VARARGS,   (char *)&doc_bcj_ppc_convert},
    {"bcj_sparc_convert",   (PyCFunction)pylzma_bcj_sparc_convert,  METH_VARARGS,   (char *)&doc_bcj_sparc_convert},
    {"bcj_ia64_convert",    (PyCFunction)pylzma_bcj_ia64_convert,   METH_VARARGS,   (char *)&doc_bcj_ia64_convert},
    {"bcj2_decode", (PyCFunction)pylzma_bcj2_decode,   METH_VARARGS,   (char *)&doc_bcj2_decode},
    // Delta
    {"delta_decode", (PyCFunction)pylzma_delta_decode,   METH_VARARGS,   (char *)&doc_delta_decode},
    {"delta_encode", (PyCFunction)pylzma_delta_encode,   METH_VARARGS,   (char *)&doc_delta_encode},
    // PPMd
    {"ppmd_decompress", (PyCFunction)pylzma_ppmd_decompress,   METH_VARARGS,   (char *)&doc_ppmd_decompress},
    {NULL, NULL},
};

static void *Alloc(ISzAllocPtr p, size_t size) { (void)p; return malloc(size); }
static void Free(ISzAllocPtr p, void *address) { (void)p; free(address); }
ISzAlloc allocator = { Alloc, Free };

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef
pylzma_module = {
   PyModuleDef_HEAD_INIT,
   "pylzma",
   NULL,
   -1,
   methods
};
#define RETURN_MODULE_ERROR     return NULL
#else
#define RETURN_MODULE_ERROR     return
#endif

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
PyInit_pylzma(void)
#else
initpylzma(void)
#endif
{
    PyObject *m;

    CDecompressionObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CDecompressionObject_Type) < 0)
        RETURN_MODULE_ERROR;
#if 0
    CCompressionObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CCompressionObject_Type) < 0)
        RETURN_MODULE_ERROR;
#endif
    CCompressionFileObject_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CCompressionFileObject_Type) < 0)
        RETURN_MODULE_ERROR;

    CAESDecrypt_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&CAESDecrypt_Type) < 0)
        RETURN_MODULE_ERROR;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&pylzma_module);
#else
    m = Py_InitModule("pylzma", methods);
#endif

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

#ifdef PYLZMA_VERSION
    PyModule_AddStringConstant(m, "__version__", STRINGIZE_VALUE_OF(PYLZMA_VERSION));
#else
    PyModule_AddStringConstant(m, "__version__", "unreleased");
#endif

    AesGenTables();
    pylzma_init_compfile();

#if defined(WITH_THREAD)
#if PY_VERSION_HEX < 0x03090000
    PyEval_InitThreads();
#endif

#if !defined(PYLZMA_USE_GILSTATE)
    /* Save the current interpreter, so compressing file objects works. */
    _pylzma_interpreterState = PyThreadState_Get()->interp;
#endif

#endif
#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
