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
from distutils.core import setup, Extension

PYTHON_VERSION=sys.version[:3]
PYTHON_PREFIX=sys.prefix

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

descr = "Python bindings for the LZMA library by Igor Pavlov."
try: version = open('version.txt', 'rb').read().strip()
except: version = 'unknown'
modules = ['py7zlib']
c_files = ['pylzma.c', 'pylzma_encoder.cpp', 'pylzma_decompressobj.c', 'pylzma_compressobj.cpp',
           'pylzma_compressfile.cpp', 'pylzma_decompress.c', 'pylzma_compress.cpp',
           'pylzma_7zip_inarchive.cpp']
macros = [('COMPRESS_MF_BT', 1), ('EXCLUDE_COM', 1), ('COMPRESS_LZMA', 1), ('_NO_CRYPTO', 1)]
lzma_files = ('7zip/LzmaDecode.c', '7zip/7zip/Compress/LZMA/LZMAEncoder.cpp',
    '7zip/7zip/Compress/LZMA/LZMALen.cpp', '7zip/7zip/Compress/LZMA/LZMALiteral.cpp',
    '7zip/7zip/Compress/RangeCoder/RangeCoderBit.cpp', '7zip/Common/CRC.cpp',
    '7zip/7zip/Compress/LZ/LZInWindow.cpp', '7zip/7zGuids.cpp',
    '7zip/7zip/Common/OutBuffer.cpp', )
archive_7z_files = ('7zip/7zip/Archive/7z/7zIn.cpp', '7zip/Common/Vector.cpp',
    '7zip/7zip/Common/LimitedStreams.cpp', '7zip/OS/Synchronization.cpp', 
    '7zip/7zip/Common/StreamObjects.cpp', '7zip/7zip/Common/MultiStream.cpp', 
    '7zip/7zip/Archive/Common/CoderMixer2.cpp', '7zip/7zip/Archive/Common/CrossThreadProgress.cpp', 
    '7zip/7zip/Common/StreamBinder.cpp', '7zip/7zip/Archive/7z/7zMethodID.cpp', 
    '7zip/7zip/Archive/7z/7zEncode.cpp', '7zip/7zip/Common/InOutTempBuffer.cpp', 
    '7zip/7zip/Archive/7z/7zSpecStream.cpp', '7zip/OS/FileDir.cpp', 
    '7zip/OS/FileName.cpp', '7zip/OS/FileFind.cpp', '7zip/Common/String.cpp', 
    '7zip/Common/StringConvert.cpp', '7zip/OS/FileIO.cpp', '7zip/Common/Wildcard.cpp', 
    '7zip/7zip/Archive/7z/7zHeader.cpp', '7zip/7zip/Archive/7z/7zDecode.cpp',
    '7zip/7zip/Compress/LZMA/LZMADecoder.cpp', '7zip/7zip/Compress/LZ/LZOutWindow.cpp', 
    '7zip/7zip/Common/InBuffer.cpp', '7zip/7zip/Archive/7z/7zHandler.cpp', 
    '7zip/Common/IntToString.cpp', '7zip/7zip/Archive/Common/ItemNameUtils.cpp', 
    '7zip/7zip/Archive/7z/7zMethods.cpp', '7zip/7zip/Archive/Common/CodecsPath.cpp', 
    '7zip/7zip/Archive/7z/7zUpdate.cpp', '7zip/7zip/Archive/Common/InStreamWithCRC.cpp', 
    '7zip/7zip/Archive/Common/OutStreamWithCRC.cpp', '7zip/7zip/Archive/7z/7zProperties.cpp',
    '7zip/7zip/Archive/7z/7zFolderInStream.cpp', '7zip/7zip/Archive/7z/7zFolderOutStream.cpp', 
    '7zip/7zip/Archive/7z/7zOut.cpp', '7zip/7zip/Archive/7z/7zHandlerOut.cpp', 
    '7zip/Common/StringToInt.cpp', '7zip/7zip/Compress/Copy/CopyCoder.cpp', 
    '7zip/7zip/Archive/7z/7zExtract.cpp', '7zip/7zip/Common/ProgressUtils.cpp',
    )
join = os.path.join
normalize = os.path.normpath
c_files += map(lambda x: normalize(join('.', x)), lzma_files)
c_files += map(lambda x: normalize(join('.', x)), archive_7z_files)
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
