#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2005 by Joachim Bauch, mail@joachim-bauch.de
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
import md5, random
import pylzma
import unittest
from binascii import unhexlify
from cStringIO import StringIO

if not hasattr(pylzma, 'decompress_compat'):
    raise ImportError, 'no compatibility support available'

ALL_CHARS = ''.join([chr(x) for x in xrange(256)])

# cache random strings to speed up tests
_random_strings = {}
def generate_random(size, choice=random.choice, ALL_CHARS=ALL_CHARS):
    global _random_strings
    if _random_strings.has_key(size):
        return _random_strings[size]
        
    s = ''.join([choice(ALL_CHARS) for x in xrange(size)])
    _random_strings[size] = s
    return s

class TestPyLZMACompability(unittest.TestCase):
    
    def setUp(self):
        self.plain = 'hello, this is a test string'
        self.plain_with_eos = unhexlify('5d0000800000341949ee8def8c6b64909b1386e370bebeb1b656f5736d653c127731a214ff7031c000')
        self.plain_without_eos = unhexlify('5d0000800000341949ee8def8c6b64909b1386e370bebeb1b656f5736d653c115edbe9')
        
    def test_decompression_noeos(self):
        # test decompression without the end of stream marker
        decompressed = pylzma.decompress_compat(self.plain_without_eos)
        self.assertEqual(decompressed, self.plain)

    def test_compression_decompression_noeos(self):
        # call compression and decompression on random data of various sizes
        for i in xrange(18):
            size = 1 << i
            original = generate_random(size)
            result = pylzma.decompress_compat(pylzma.compress(original, eos=0))
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

    def test_multi(self):
        # call compression and decompression multiple times to detect memory leaks...
        for x in xrange(4):
            self.test_compression_decompression_noeos()

def test_main():
    from test import test_support
    test_support.run_unittest(TestPyLZMACompability)

if __name__ == "__main__":
    unittest.main()
