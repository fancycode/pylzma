#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2010 by Joachim Bauch, mail@joachim-bauch.de
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
from datetime import datetime
import os
import pylzma
from py7zlib import Archive7z, NoPasswordGivenError, WrongPasswordError
import sys
import unittest

try:
    sorted
except NameError:
    # Python 2.3 and older
    def sorted(l):
        l = list(l)
        l.sort()
        return l

if sys.version_info[:2] < (3, 0):
    def bytes(s, encoding):
        return s
    
    def unicode_string(s):
        return s.decode('latin-1')
else:
    def unicode_string(s):
        return s

ROOT = os.path.abspath(os.path.split(__file__)[0])

class Test7ZipFiles(unittest.TestCase):
    
    def _test_archive(self, filename):
        fp = open(os.path.join(ROOT, 'data', filename), 'rb')
        archive = Archive7z(fp)
        self.failUnlessEqual(sorted(archive.getnames()), ['test/test2.txt', 'test1.txt'])
        self.failUnlessEqual(archive.getmember('test2.txt'), None)
        cf = archive.getmember('test1.txt')
        self.failUnless(cf.checkcrc())
        self.failUnlessEqual(datetime.fromtimestamp(cf.lastwritetime).replace(microsecond=0), \
            datetime(2006, 3, 15, 21, 43, 48))
        self.failUnlessEqual(cf.read(), bytes('This file is located in the root.', 'ascii'))
        cf.reset()
        self.failUnlessEqual(cf.read(), bytes('This file is located in the root.', 'ascii'))

        cf = archive.getmember('test/test2.txt')
        self.failUnless(cf.checkcrc())
        self.failUnlessEqual(datetime.fromtimestamp(cf.lastwritetime).replace(microsecond=0), \
            datetime(2006, 3, 15, 21, 43, 36))
        self.failUnlessEqual(cf.read(), bytes('This file is located in a folder.', 'ascii'))
        cf.reset()
        self.failUnlessEqual(cf.read(), bytes('This file is located in a folder.', 'ascii'))

    def test_non_solid(self):
        # test loading of a non-solid archive
        self._test_archive('non_solid.7z')

    def test_solid(self):
        # test loading of a solid archive
        self._test_archive('solid.7z')

    def _test_umlaut_archive(self, filename):
        fp = open(os.path.join(ROOT, 'data', filename), 'rb')
        archive = Archive7z(fp)
        self.failUnlessEqual(sorted(archive.getnames()), [unicode_string('t\xe4st.txt')])
        self.failUnlessEqual(archive.getmember('test.txt'), None)
        cf = archive.getmember(unicode_string('t\xe4st.txt'))
        self.failUnlessEqual(cf.read(), bytes('This file contains a german umlaut in the filename.', 'ascii'))
        cf.reset()
        self.failUnlessEqual(cf.read(), bytes('This file contains a german umlaut in the filename.', 'ascii'))
        
    def test_non_solid_umlaut(self):
        # test loading of a non-solid archive containing files with umlauts
        self._test_umlaut_archive('umlaut-non_solid.7z')

    def test_solid_umlaut(self):
        # test loading of a solid archive containing files with umlauts
        self._test_umlaut_archive('umlaut-solid.7z')

    def test_bugzilla_4(self):
        # sample file for bugzilla #4
        fp = open(os.path.join(ROOT, 'data', 'bugzilla_4.7z'), 'rb')
        archive = Archive7z(fp)
        filenames = archive.getnames()
        for filename in filenames:
            cf = archive.getmember(filename)
            self.failUnlessEqual(len(cf.read()), cf.uncompressed)

    if sys.version_info[:2] < (3, 0):

        def test_encrypted_no_password(self):
            # test loading of encrypted files without password (can read archived filenames)
            fp = open(os.path.join(ROOT, 'data', 'encrypted.7z'), 'rb')
            archive = Archive7z(fp)
            filenames = archive.getnames()
            self.failUnlessEqual(sorted(archive.getnames()), sorted(['test1.txt', 'test/test2.txt']))
            self.failUnlessRaises(NoPasswordGivenError, archive.getmember('test1.txt').read)
            self.failUnlessRaises(NoPasswordGivenError, archive.getmember('test/test2.txt').read)

        def test_encrypted_password(self):
            # test loading of encrypted files with correct password
            fp = open(os.path.join(ROOT, 'data', 'encrypted.7z'), 'rb')
            archive = Archive7z(fp, password='secret')
            filenames = archive.getnames()
            for filename in filenames:
                cf = archive.getmember(filename)
                self.failUnlessEqual(len(cf.read()), cf.uncompressed)

        def test_encrypted_wong_password(self):
            # test loading of encrypted files with wrong password
            fp = open(os.path.join(ROOT, 'data', 'encrypted.7z'), 'rb')
            archive = Archive7z(fp, password='password')
            filenames = archive.getnames()
            for filename in filenames:
                cf = archive.getmember(filename)
                self.failUnlessRaises(WrongPasswordError, cf.read)

    def test_deflate(self):
        # test loading of deflate compressed files
        self._test_archive('deflate.7z')

    def test_deflate64(self):
        # test loading of deflate64 compressed files
        self._test_archive('deflate64.7z')

    def test_bzip2(self):
        # test loading of bzip2 compressed files
        self._test_archive('bzip2.7z')

    def test_copy(self):
        # test loading of copy compressed files
        self._test_archive('copy.7z')

    def test_regress_1(self):
        # prevent regression bug #1 reported by mail
        fp = open(os.path.join(ROOT, 'data', 'regress_1.7z'), 'rb')
        archive = Archive7z(fp)
        filenames = list(archive.getnames())
        self.failUnlessEqual(len(filenames), 1)
        cf = archive.getmember(filenames[0])
        self.failIfEqual(cf, None)
        self.failUnless(cf.checkcrc())
        data = cf.read()
        self.failUnlessEqual(len(data), cf.size)

    def test_bugzilla_16(self):
        # sample file for bugzilla #16
        fp = open(os.path.join(ROOT, 'data', 'bugzilla_16.7z'), 'rb')
        archive = Archive7z(fp)
        filenames = archive.getnames()
        for filename in filenames:
            cf = archive.getmember(filename)
            self.failUnlessEqual(len(cf.read()), cf.uncompressed)


def suite():
    suite = unittest.TestSuite()

    test_cases = [
        Test7ZipFiles,
    ]

    for tc in test_cases:
        suite.addTest(unittest.makeSuite(tc))

    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='suite')
