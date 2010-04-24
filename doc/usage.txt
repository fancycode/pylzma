$Id$

In this document, some samples of the PyLZMA library will be given.


First, we need to import the module::

  >>> import pylzma


The easiest usage is compression and decompression in one step::
  
  >>> compressed = pylzma.compress('Hello world!')
  >>> pylzma.decompress(compressed)
  'Hello world!'


For compression, additional parameters can be specified::

  >>> compressed = pylzma.compress('Hello world!', dictionary=10)
  >>> pylzma.decompress(compressed)
  'Hello world!'


Other available parameters are:

dictionary
  Dictionary size (Range 0-28, Default: 23 (8MB))

  The maximum value for dictionary size is 256 MB = 2^28 bytes.
  Dictionary size is calculated as DictionarySize = 2^N bytes. 
  For decompressing file compressed by LZMA method with dictionary 
  size D = 2^N you need about D bytes of memory (RAM).
  
fastBytes
  Range 5-255, default 128

  Usually big number gives a little bit better compression ratio and slower
  compression process.

literalContextBits
  Range 0-8, default 3

  Sometimes literalContextBits=4 gives gain for big files.

literalPosBits
  Range 0-4, default 0

  This switch is intended for periodical data when period is equal 2^N.
  For example, for 32-bit (4 bytes) periodical data you can use
  literalPosBits=2. Often it's better to set literalContextBits=0, if you
  change the literalPosBits switch.

posBits
  Range 0-4, default 2

  This switch is intended for periodical data when period is equal 2^N.

algorithm
  Compression mode 0 = fast, 1 = normal, 2 = max (Default: 2)

  The lower the number specified for algorithm, the faster compression will
  perform.

multithreading
  Use multithreading if available? (Default yes)

  Currently, multithreading is only available on Windows platforms.

eos
  Should the `End Of Stream` marker be written? (Default yes)
  You can save some bytes if the marker is omitted, but the total uncompressed
  size must be stored by the application and used when decompressing:
  
  >>> compressed1 = pylzma.compress('Hello world!', eos=1)
  >>> compressed2 = pylzma.compress('Hello world!', eos=0)
  >>> len(compressed1) > len(compressed2)
  True
  
  >>> pylzma.decompress(compressed2)
  Traceback (most recent call last):
  ...
  ValueError: data error during decompression

  >>> pylzma.decompress(compressed2, maxlength=12)
  'Hello world!'
  
  If you don't know the total uncompressed size, you can use the compatibility
  decompression function from pylzma version 0.0.3.  Be aware that this old
  method is slower than the new decompression function, so you should use
  `pylzma.decompress` whenever possible.
  
  >>> pylzma.decompress_compat(compressed2)
  'Hello world!'


If you need to compress larger amounts of data, you should use the streaming
version of the library.  If supports compressing any file-like objects::

  >>> from cStringIO import StringIO
  >>> fp = StringIO('Hello world!')
  >>> c_fp = pylzma.compressfile(fp, eos=1)
  >>> compressed = ''
  >>> while True:
  ...   tmp = c_fp.read(1)
  ...   if not tmp: break
  ...   compressed += tmp
  ...
  >>> pylzma.decompress(compressed)
  'Hello world!'
  

Using a similar technique, you can decompress large amounts of data without
keeping everything in memory::
  
  >>> from cStringIO import StringIO
  >>> fp = StringIO(pylzma.compress('Hello world!'))
  >>> obj = pylzma.decompressobj()
  >>> plain = ''
  >>> while True:
  ...   tmp = fp.read(1)
  ...   if not tmp: break
  ...   plain += obj.decompress(tmp)
  ...
  >>> plain += obj.flush()
  >>> plain
  'Hello world!'


However this only works for streams that contain the `End Of Stream` marker.
You must provide the size of the decompressed data if you don't include the
EOS marker::

  >>> from cStringIO import StringIO
  >>> fp = StringIO(pylzma.compress('Hello world!', eos=0))
  >>> obj = pylzma.decompressobj(maxlength=13)
  >>> plain = ''
  >>> while True:
  ...   tmp = fp.read(1)
  ...   if not tmp: break
  ...   plain += obj.decompress(tmp)
  ...
  >>> plain += obj.flush()
  Traceback (most recent call last):
  ...
  ValueError: data error during decompression

  >>> obj.reset(maxlength=12)
  >>> fp.seek(0)
  >>> plain = ''
  >>> while True:
  ...   tmp = fp.read(1)
  ...   if not tmp: break
  ...   plain += obj.decompress(tmp)
  ...
  >>> plain += obj.flush()
  >>> plain
  'Hello world!'


Please note that the compressed data is not compatible to the lzma.exe command
line utility!  To get compatible data, you can use the following utility
function::

  >>> import struct
  >>> from cStringIO import StringIO
  
  >>> def compress_compatible(data):
  ...     c = pylzma.compressfile(StringIO(data))
  ...     # LZMA header
  ...     result = c.read(5)
  ...     # size of uncompressed data
  ...     result += struct.pack('<Q', len(data))
  ...     # compressed data
  ...     return result + c.read()

