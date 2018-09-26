# pylzma

## 0.5.0

- Update to LZMA SDK 18.05.
- Support LZMA2 decompression.
- Fix decompressing empty members from 7z files.
- Fix parsing attributes from 7z members.
- Include test data in tarball.
- Fix AES decryption for solid 7z archives with LZMA2 compression.


## 0.4.9

- Fix for 7z files that contain padding data after uncompress / decrypt.
- Support dummy entries in 7z files.


## 0.4.8

- Don't include 7z files in releases.


## 0.4.7

- Fix input size when decompressing with multiple codecs.
- Reduce number of temporary objects for improved performance.
- Use long integers where possible to support large archives.
- Support archives with encrypted filenames.
- Fix switching of compressed streams that contain multiple files.


## 0.4.6

- Export `__version__` from module.
- Allow custom output for `Archive7z.list`.
- Fixes for running on Python 3.
- Include tests in tarball.


## 0.4.5

- Integrate with Travis CI and add more test cases.
- Fix potential double-free crash.
- Initial support for Python 3.
- Fix decoding of archive members without a filename.
- Add BCJ filters.
- Use own AES256-SHA256 implementation instead of `M2Crypto`.
- Support reading empty archives.
- Fix wrong offset when converting `FILETIME` to `datetime`.
- Cache data for better performance of solid 7z archives.
- Fix parsing of mixed solid/non-solid 7z files.


## 0.4.4

- Fix compiler error with gcc on i386.
- Fix compiler error with MSVC.


## 0.4.3

- Initial support for AES256-SHA256 using `M2Crypto`.
- Better support for Python versions before 2.5.
- Read bigger blocks when decompressing for improved performance.
- Fix parsing 7z files without CRC information.


## 0.4.2

- Update integration of multithreaded compression.
- Fix compiling with MinGW.
- Improve use of (re-)allocation for better performance.
- Include all header files in tarball.


## 0.4.1

- Update urls.


## 0.4.0

- First released version.
