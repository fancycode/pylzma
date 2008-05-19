#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2008 by Joachim Bauch, mail@joachim-bauch.de
# 7-Zip Copyright (C) 1999-2005 Igor Pavlov
# LZMA SDK Copyright (C) 1999-2005 Igor Pavlov
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

# are we building an egg package?
BUILD_EGG = 'bdist_egg' in sys.argv or 'upload' in sys.argv

kw = {}
if BUILD_EGG:
    from setuptools import setup, Extension
    kw['test_suite'] = 'tests'
    kw['zip_safe'] = False
else:
    from distutils.core import setup, Extension

PYTHON_VERSION=sys.version[:3]
PYTHON_PREFIX=sys.prefix

class UnsupportedPlatformWarning(Warning):
    pass

# set this to any true value to enable multithreaded compression
ENABLE_MULTITHREADING = True

# set this to any true value to add the compatibility decoder
# from version 0.0.3 to be able to decompress strings without
# the end of stream mark and you don't know their lengths
ENABLE_COMPATIBILITY = True

# compile including debug symbols on Windows?
COMPILE_DEBUG = False

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
long_descr = """PyLZMA provides a platform independent way to read and write data
that has been compressed or can be decompressed by the LZMA library by Igor Pavlov."""
try: version = open('version.txt', 'rb').read().strip()
except: version = 'unknown'
modules = ['py7zlib']
c_files = ['pylzma.c', 'pylzma_decompressobj.c', 'pylzma_compressfile.cpp',
           'pylzma_decompress.c', 'pylzma_compress.cpp', 'pylzma_guids.cpp']
compile_args = []
link_args = []
macros = []
if 'win' in sys.platform:
    macros.append(('WIN32', 1))
    if COMPILE_DEBUG:
        compile_args.append('/Zi')
        compile_args.append('/MTd')
        link_args.append('/DEBUG')
    else:
        compile_args.append('/MT')
if not 'win' in sys.platform:
    # disable gcc warning about virtual functions with non-virtual destructors
    compile_args.append(('-Wno-non-virtual-dtor'))
if ENABLE_MULTITHREADING:
    macros.append(('COMPRESS_MF_MT', 1))
lzma_files = ('7zip/LzmaStateDecode.c', '7zip/7zip/Compress/LZMA/LZMAEncoder.cpp',
    '7zip/7zip/Compress/RangeCoder/RangeCoderBit.cpp', '7zip/Common/CRC.cpp',
    '7zip/7zip/Compress/LZ/LZInWindow.cpp', '7zip/7zip/Common/StreamUtils.cpp',
    '7zip/7zip/Common/OutBuffer.cpp', '7zip/Common/Alloc.cpp', '7zip/Common/NewHandler.cpp', )
if ENABLE_MULTITHREADING:
    lzma_files += ('7zip/7zip/Compress/LZ/MT/MT.cpp', '7zip/OS/Synchronization.cpp', )
if ENABLE_COMPATIBILITY:
    c_files += ('pylzma_decompress_compat.c', 'pylzma_decompressobj_compat.c', )
    lzma_files += ('7zip/LzmaCompatDecode.c', )
    macros.append(('WITH_COMPAT', 1))
join = os.path.join
normalize = os.path.normpath
c_files += map(lambda x: normalize(join('.', x)), lzma_files)
extens=[Extension('pylzma', c_files, include_dirs=include_dirs, libraries=libraries,
                  library_dirs=library_dirs, define_macros=macros, extra_compile_args=compile_args,
                  extra_link_args=link_args)] 

if sys.platform == 'win32':
    operating_system = 'Microsoft :: Windows'
else:
    operating_system = 'POSIX :: Linux'

setup(
    name = "pylzma",
    version = version,
    description = descr,
    author = "Joachim Bauch",
    author_email = "mail@joachim-bauch.de",
    url = "http://www.joachim-bauch.de",
    license = 'LGPL',
    keywords = "lzma compression",
    long_description = long_descr,
    platforms = sys.platform,
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Programming Language :: Python',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
        'Operating System :: %s' % operating_system,
    ],
    py_modules = modules,
    ext_modules = extens,
    **kw
)

sys.exit(0)
