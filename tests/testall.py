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
import md5, random
import pylzma
import unittest
from binascii import unhexlify

class TestPyLZMA(unittest.TestCase):
    
    def testcompression(self):
        original = 'hello, this is a test string'
        compressed = pylzma.compress(original)
        self.assertEqual(md5.new(compressed).hexdigest(), '790a7c03afa72b2fbc93f46b39c3cd73')

    def testdecompression(self):
        original = '5d0000800000341949ee8def8c6b64909b1386e370bebeb1b656f5736d653c127731a214ff7031c000'
        original = unhexlify(original)
        decompressed = pylzma.decompress(original)
        self.assertEqual(decompressed, 'hello, this is a test string')

    def testboth(self):
        data = map(lambda x: chr(x), xrange(256))
        for i in xrange(18):
            size = 1 << i
            original = map(lambda x: data[random.randrange(0, 255)], xrange(size))
            original = ''.join(original)
            result = pylzma.decompress(pylzma.compress(original))
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

if __name__ == "__main__":
    unittest.main()
