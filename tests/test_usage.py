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
# $Id: testall.py 61 2005-10-13 20:48:53Z jojo $
#
import os, sys
import unittest
import doctest

USAGE_FILE = os.path.join('..', 'doc', 'usage.txt')

ROOT = os.path.abspath(os.path.split(__file__)[0])
sys.path.insert(0, ROOT)

def suite():
    suite = unittest.TestSuite()
    if sys.version_info[:2] < (2,4):
        import warnings
        warnings.warn('Python 2.4 or above required to run doctests')
    elif sys.version_info[:2] < (3, 0):
        suite.addTest(doctest.DocFileSuite(USAGE_FILE))
    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='suite')
