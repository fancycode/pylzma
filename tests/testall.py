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
from cStringIO import StringIO

class TestPyLZMA(unittest.TestCase):
    
    def setUp(self):
        self.plain = 'hello, this is a test string'
        self.plain_with_eos = unhexlify('5d0000800000341949ee8def8c6b64909b1386e370bebeb1b656f5736d653c127731a214ff7031c000')
        self.plain_without_eos = unhexlify('5d0000800000341949ee8def8c6b64909b1386e370bebeb1b656f5736d653c115edbe9')
        
    def test_compression_eos(self):
        # test compression with end of stream marker
        compressed = pylzma.compress(self.plain, eos=1)
        self.assertEqual(compressed, self.plain_with_eos)
    
    def test_compression_no_eos(self):
        # test compression without end of stream marker
        compressed = pylzma.compress(self.plain, eos=0)
        self.assertEqual(compressed, self.plain_without_eos)
        
    def test_decompression_eos(self):
        # test decompression with the end of stream marker
        decompressed = pylzma.decompress(self.plain_with_eos)
        self.assertEqual(decompressed, self.plain)
        
    def test_decompression_noeos(self):
        # test decompression without the end of stream marker
        decompressed = pylzma.decompress(self.plain_without_eos)
        decompressed = decompressed[:28]
        self.assertEqual(decompressed, self.plain)

    def test_compression_decompression_eos(self):
        # call compression and decompression on random data of various sizes
        data = map(lambda x: chr(x), xrange(256))
        for i in xrange(18):
            size = 1 << i
            original = map(lambda x: data[random.randrange(0, 255)], xrange(size))
            original = ''.join(original)
            result = pylzma.decompress(pylzma.compress(original, eos=1))
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

    def test_compression_decompression_noeos(self):
        # call compression and decompression on random data of various sizes
        data = map(lambda x: chr(x), xrange(256))
        for i in xrange(18):
            size = 1 << i
            original = map(lambda x: data[random.randrange(0, 255)], xrange(size))
            original = ''.join(original)
            result = pylzma.decompress(pylzma.compress(original, eos=0))
            # without the eos marker, the result may be padded with 0x00 bytes
            # when using this compression format, one must store the data length otherwise
            result = result[:size]
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

    def test_multi(self):
        # call compression and decompression multiple times to detect memory leaks...
        for x in xrange(4):
            self.test_compression_decompression_eos()
            self.test_compression_decompression_noeos()

    def test_decompression_stream(self):
        # test decompression in two steps
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos[:10])
        data += decompress.decompress(decompress.unconsumed_tail + self.plain_with_eos[10:])
        self.assertEqual(data, self.plain)

    def test_decompression_stream_reset(self):
        # test reset
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos[:10])
        decompress.reset()
        data = decompress.decompress(self.plain_with_eos[:15])
        data += decompress.decompress(decompress.unconsumed_tail + self.plain_with_eos[15:])
        self.assertEqual(data, self.plain)

    def test_decompression_streaming(self):
        # test decompressing with one byte at a time...
        decompress = pylzma.decompressobj()
        infile = StringIO(self.plain_with_eos)
        outfile = StringIO()
        while 1:
            data = decompress.unconsumed_tail + infile.read(1)
            if not data: break
            outfile.write(decompress.decompress(data, 1))
        self.assertEqual(outfile.getvalue(), self.plain)

    def _test_compression_streaming(self):
        # XXX: this doesn't work, yet
        # test compressing with one byte at a time...
        compress = pylzma.compressobj(eos=1)
        infile = StringIO(self.plain)
        outfile = StringIO()
        while 1:
            data = infile.read(1)
            if not data: break
            outfile.write(compress.compress(data, 1))
        outfile.write(compress.flush())
        check = pylzma.decompress(outfile.getvalue())
        self.assertEqual(check, self.plain)

if __name__ == "__main__":
    unittest.main()
