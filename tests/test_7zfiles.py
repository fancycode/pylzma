#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2006 by Joachim Bauch, mail@joachim-bauch.de
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
import os
import pylzma
from py7zlib import Archive7z
import unittest

try:
    sorted
except NameError:
    # Python 2.3 and older
    def sorted(l):
        l = list(l)
        l.sort()
        return l

ROOT = os.path.abspath(os.path.split(__file__)[0])

class Test7ZipFiles(unittest.TestCase):
    
    def _test_archive(self, filename):
        fp = file(os.path.join(ROOT, 'data', filename), 'rb')
        archive = Archive7z(fp)
        self.failUnlessEqual(sorted(archive.getnames()), [u'test/test2.txt', u'test1.txt'])
        self.failUnlessEqual(archive.getmember(u'test2.txt'), None)
        cf = archive.getmember(u'test1.txt')
        self.failUnlessEqual(cf.read(), 'This file is located in the root.')
        cf.reset()
        self.failUnlessEqual(cf.read(), 'This file is located in the root.')

        cf = archive.getmember(u'test/test2.txt')
        self.failUnlessEqual(cf.read(), 'This file is located in a folder.')
        cf.reset()
        self.failUnlessEqual(cf.read(), 'This file is located in a folder.')

    def test_non_solid(self):
        # test loading of a non-solid archive
        self._test_archive('non_solid.7z')

    def test_solid(self):
        # test loading of a solid archive
        self._test_archive('solid.7z')

    def _test_umlaut_archive(self, filename):
        fp = file(os.path.join(ROOT, 'data', filename), 'rb')
        archive = Archive7z(fp)
        self.failUnlessEqual(sorted(archive.getnames()), [u't\xe4st.txt'])
        self.failUnlessEqual(archive.getmember(u'test.txt'), None)
        cf = archive.getmember(u't\xe4st.txt')
        self.failUnlessEqual(cf.read(), 'This file contains a german umlaut in the filename.')
        cf.reset()
        self.failUnlessEqual(cf.read(), 'This file contains a german umlaut in the filename.')
        
    def test_non_solid_umlaut(self):
        # test loading of a non-solid archive containing files with umlauts
        self._test_umlaut_archive('umlaut-non_solid.7z')

    def test_solid_umlaut(self):
        # test loading of a solid archive containing files with umlauts
        self._test_umlaut_archive('umlaut-solid.7z')

    def test_bugzilla_4(self):
        # sample file for bugzilla #4
        fp = file(os.path.join(ROOT, 'data', 'bugzilla_4.7z'), 'rb')
        archive = Archive7z(fp)
        filenames = archive.getnames()
        for filename in filenames:
            cf = archive.getmember(filename)
            self.failUnlessEqual(len(cf.read()), cf.uncompressed)


def test_main():
    from test import test_support
    test_support.run_unittest(Test7ZipFiles)

if __name__ == "__main__":
    unittest.main()
