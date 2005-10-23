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
import sys, md5, random
import pylzma
import unittest
from binascii import unhexlify
from cStringIO import StringIO

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
        decompressed = pylzma.decompress(self.plain_without_eos, maxlength=28)
        self.assertEqual(decompressed, self.plain)

    def test_compression_decompression_eos(self):
        # call compression and decompression on random data of various sizes
        for i in xrange(18):
            size = 1 << i
            original = generate_random(size)
            result = pylzma.decompress(pylzma.compress(original, eos=1))
            self.assertEqual(len(result), size)
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

    def test_compression_decompression_noeos(self):
        # call compression and decompression on random data of various sizes
        for i in xrange(18):
            size = 1 << i
            original = generate_random(size)
            result = pylzma.decompress(pylzma.compress(original, eos=0), maxlength=size)
            self.assertEqual(md5.new(original).hexdigest(), md5.new(result).hexdigest())

    def test_multi(self):
        # call compression and decompression multiple times to detect memory leaks...
        for x in xrange(4):
            self.test_compression_decompression_eos()
            self.test_compression_decompression_noeos()

    def test_matchfinders(self):
        # use different matchfinder algorithms for compression
        matchfinders = ['bt2', 'bt3', 'bt4', 'bt4b', 'pat2r', 'pat2', 'pat2h', 'pat3h', 'pat4h', 'hc3', 'hc4']
        if sys.platform == 'cygwin':
            # XXX: the matchfinder "pat4h" results in a core dump when running on cygwin
            matchfinders.remove('pat4h')
        original = 'hello world'
        for mf in matchfinders:
            result = pylzma.decompress(pylzma.compress(original, matchfinder=mf))
            self.assertEqual(original, result)
            
        self.failUnlessRaises(TypeError, pylzma.compress, original, matchfinder='1234')

    def test_decompression_stream(self):
        # test decompression object in one steps
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos)
        data += decompress.flush()
        self.assertEqual(data, self.plain)
    
    def test_decompression_stream_two(self):
        # test decompression in two steps
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos[:10])
        data += decompress.decompress(self.plain_with_eos[10:])
        data += decompress.flush()
        self.assertEqual(data, self.plain)

    def test_decompression_stream_props(self):
        # test decompression with properties in separate step
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos[:5])
        data += decompress.decompress(self.plain_with_eos[5:])
        data += decompress.flush()
        self.assertEqual(data, self.plain)

    def test_decompression_stream_reset(self):
        # test reset
        decompress = pylzma.decompressobj()
        data = decompress.decompress(self.plain_with_eos[:10])
        decompress.reset()
        data = decompress.decompress(self.plain_with_eos[:15])
        data += decompress.decompress(self.plain_with_eos[15:])
        data += decompress.flush()
        self.assertEqual(data, self.plain)

    def test_decompression_streaming(self):
        # test decompressing with one byte at a time...
        decompress = pylzma.decompressobj()
        infile = StringIO(self.plain_with_eos)
        outfile = StringIO()
        while 1:
            data = infile.read(1)
            if not data: break
            outfile.write(decompress.decompress(data, 1))
        outfile.write(decompress.flush())
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

    def test_compression_file(self):
        # test compressing from file-like object
        infile = StringIO(self.plain)
        outfile = StringIO()
        compress = pylzma.compressfile(infile, eos=1)
        while 1:
            data = compress.read(1)
            if not data: break
            outfile.write(data)
        check = pylzma.decompress(outfile.getvalue())
        self.assertEqual(check, self.plain)

def test_main():
    from test import test_support
    test_support.run_unittest(TestPyLZMA)

if __name__ == "__main__":
    unittest.main()
