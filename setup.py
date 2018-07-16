#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2015 by Joachim Bauch, mail@joachim-bauch.de
# 7-Zip Copyright (C) 1999-2010 Igor Pavlov
# LZMA SDK Copyright (C) 1999-2010 Igor Pavlov
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
from distutils import log
from distutils.command.build_ext import build_ext as _build_ext
from version import get_git_version

try:
    from setuptools import setup, Extension
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()

    from setuptools import setup, Extension

class UnsupportedPlatformWarning(Warning):
    pass

try:
    unicode
except NameError:
    # Python 3.x
    class unicode(object):
        def __new__(cls, s):
            if isinstance(s, str):
                return s
            return s and s.decode('utf-8') or None

version = get_git_version()
if version and not isinstance(version, unicode):
    version = unicode(version)

# set this to any true value to enable multithreaded compression
ENABLE_MULTITHREADING = True

# set this to any true value to add the compatibility decoder
# from version 0.0.3 to be able to decompress strings without
# the end of stream mark and you don't know their lengths
ENABLE_COMPATIBILITY = True

# compile including debug symbols on Windows?
COMPILE_DEBUG = False

# are we running on Windows?
IS_WINDOWS = sys.platform in ('win32', )

libraries = []
if IS_WINDOWS:
    libraries += ['user32', 'oleaut32']

library_dirs = []

# platforms that multithreaded compression is supported on
mt_platforms = (
    'win32',
)

if IS_WINDOWS:
    # don't try to import MSVC compiler on non-windows platforms
    # as this triggers unnecessary warnings
    from distutils.msvccompiler import MSVCCompiler
else:
    class MSVCCompiler(object):
        # dummy marker class
        pass

class build_ext(_build_ext):

    def build_extension(self, ext):
        self.with_mt = ENABLE_MULTITHREADING
        if self.with_mt and not sys.platform in mt_platforms:
            warn("""\
Multithreading is not supported on the platform "%s",
please contact mail@joachim-bauch.de for more informations.""" % (sys.platform), UnsupportedPlatformWarning)
            self.with_mt = False

        if self.with_mt:
            log.info('adding support for multithreaded compression')
            ext.define_macros.append(('COMPRESS_MF_MT', 1))
            ext.sources += ('src/sdk/C/LzFindMt.c', 'src/sdk/C/Threads.c', )
        else:
            ext.define_macros.append(('_7ZIP_ST', 1))

        if isinstance(self.compiler, MSVCCompiler):
            # set flags only available when using MSVC
            ext.extra_link_args.append('/MANIFEST')
            if COMPILE_DEBUG:
                ext.extra_compile_args.append('/Zi')
                ext.extra_compile_args.append('/MTd')
                ext.extra_link_args.append('/DEBUG')
            else:
                ext.extra_compile_args.append('/MT')

        _build_ext.build_extension(self, ext)

descr = "Python bindings for the LZMA library by Igor Pavlov."
long_descr = """PyLZMA provides a platform independent way to read and write data
that has been compressed or can be decompressed by the LZMA library by Igor Pavlov."""
modules = ['py7zlib']
c_files = ['src/pylzma/pylzma.c', 'src/pylzma/pylzma_decompressobj.c', 'src/pylzma/pylzma_compressfile.c',
           'src/pylzma/pylzma_decompress.c', 'src/pylzma/pylzma_compress.c', 'src/pylzma/pylzma_streams.c', \
           'src/pylzma/pylzma_aes.c']
compile_args = []
link_args = []
macros = [
    ('PY_SSIZE_T_CLEAN', 1),
]
lzma_files = ('src/sdk/C/LzFind.c', 'src/sdk/C/LzmaDec.c', 'src/sdk/C/LzmaEnc.c', \
    'src/sdk/C/CpuArch.c', 'src/sdk/C/Aes.c', 'src/sdk/C/AesOpt.c', \
    'src/sdk/C/Sha256.c', 'src/sdk/C/Bra.c', 'src/sdk/C/Bra86.c', \
    'src/sdk/C/BraIA64.c')
if ENABLE_COMPATIBILITY:
    c_files += ('src/pylzma/pylzma_decompress_compat.c', 'src/pylzma/pylzma_decompressobj_compat.c', )
    lzma_files += ('src/compat/LzmaCompatDecode.c', )
    macros.append(('WITH_COMPAT', 1))

if version:
    macros.append(('PYLZMA_VERSION', version))

c_files += [os.path.normpath(os.path.join('.', x)) for x in lzma_files]
extens = [
    Extension('pylzma', c_files, libraries=libraries,
              library_dirs=library_dirs, define_macros=macros, extra_compile_args=compile_args,
              extra_link_args=link_args),
]

install_requires = []
tests_require = []

setup(
    name = "pylzma",
    version = version,
    description = descr,
    author = "Joachim Bauch",
    author_email = "mail@joachim-bauch.de",
    url = "http://www.joachim-bauch.de/projects/pylzma/",
    download_url = "http://pypi.python.org/pypi/pylzma/",
    license = 'LGPL',
    keywords = "lzma compression",
    long_description = long_descr,
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
        'Operating System :: OS Independent',
    ],
    py_modules = modules,
    ext_modules = extens,
    cmdclass = {
        'build_ext': build_ext,
    },
    install_requires = install_requires,
    extras_require = {
        'decrypt': [],
    },
    tests_require = tests_require,
    test_suite = 'tests.suite',
    zip_safe = False,
)
