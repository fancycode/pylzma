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

#ifndef ___PYLZMA__H___
#define ___PYLZMA__H___

#include <Python.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define BLOCK_SIZE 65536

#define CHECK_NULL(a) if ((a) == NULL) { PyErr_NoMemory(); goto exit; }
#define DEC_AND_NULL(a) { Py_XDECREF(a); a = NULL; }
#define DELETE_AND_NULL(a) if (a != NULL) { delete a; a = NULL; }
#define FREE_AND_NULL(a) if (a != NULL) { free(a); a = NULL; }
#define CHECK_RANGE(x, a, b, msg) if ((x) < (a) || (x) > (b)) { PyErr_SetString(PyExc_ValueError, msg); return NULL; }

#endif
