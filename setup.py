#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004 by Joachim Bauch, mail@joachim-bauch.de
# LZMA SDK Copyright (C) 1999-2004 Igor Pavlov
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id$
#
import sys, os
from warnings import warn
from distutils.core import setup, Extension

PYTHON_VERSION=sys.version[:3]
PYTHON_PREFIX=sys.prefix

class UnsupportedPlatformWarning(Warning):
    pass

# set this to any true value to enable multithreaded compression
ENABLE_MULTITHREADING = 1

if os.name == 'posix':
    # This is the directory, your Python is installed in. It must contain the header and include files.
    PYTHON_INCLUDE_DIR="%s/include/python%s" % (PYTHON_PREFIX, PYTHON_VERSION)
    PYTHON_LIB_DIR="%s/lib/python%s" % (PYTHON_PREFIX, PYTHON_VERSION)
    libraries=[]
else:
    PYTHON_INCLUDE_DIR="%s\\include" % (PYTHON_PREFIX)
    PYTHON_LIB_DIR="%s\\libs" % (PYTHON_PREFIX)
    libraries=['user32', 'oleaut32']

include_dirs = [
PYTHON_INCLUDE_DIR,
".",
]

library_dirs = [
PYTHON_LIB_DIR,
".",
]

mt_platforms = (
    'win32',
)

if ENABLE_MULTITHREADING and not sys.platform in mt_platforms:
    warn("""\
Multithreading is not supported on the platform "%s",
please contact mail@joachim-bauch.de for more informations.""" % (sys.platform), UnsupportedPlatformWarning)
    ENABLE_MULTITHREADING = 0    

descr = "Python bindings for the LZMA library by Igor Pavlov."
try: version = open('version.txt', 'rb').read().strip()
except: version = 'unknown'
modules = ['py7zlib']
c_files = ['pylzma.c', 'pylzma_decompressobj.c', 'pylzma_compressfile.cpp',
           'pylzma_decompress.c', 'pylzma_compress.cpp']
macros = [('COMPRESS_MF_BT', 1), ('EXCLUDE_COM', 1), ('COMPRESS_LZMA', 1), ('_NO_CRYPTO', 1)]
if 'win' in sys.platform:
    macros.append(('WIN32', 1))
if ENABLE_MULTITHREADING:
    macros.append(('COMPRESS_MF_MT', 1))
lzma_files = ('7zip/LzmaDecode.c', '7zip/7zip/Compress/LZMA/LZMAEncoder.cpp',
    '7zip/7zip/Compress/RangeCoder/RangeCoderBit.cpp', '7zip/Common/CRC.cpp',
    '7zip/7zip/Compress/LZ/LZInWindow.cpp', '7zip/7zGuids.cpp',
    '7zip/7zip/Common/OutBuffer.cpp', '7zip/Common/Alloc.cpp', )
if ENABLE_MULTITHREADING:
    lzma_files += ('7zip/7zip/Compress/LZ/MT/MT.cpp', '7zip/OS/Synchronization.cpp', )
join = os.path.join
normalize = os.path.normpath
c_files += map(lambda x: normalize(join('.', x)), lzma_files)
extens=[Extension('pylzma', c_files, include_dirs=include_dirs, libraries=libraries,
                  library_dirs=library_dirs, define_macros=macros)] 

setup (name = "pylzma",
       version = version,
       description = descr,
       author = "Joachim Bauch",
       author_email = "mail@joachim-bauch.de",
       url = "http://www.joachim-bauch.de",
       py_modules=modules,
       ext_modules=extens,
       )

sys.exit(0)
