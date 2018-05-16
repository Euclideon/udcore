#ifndef UDCOMPRESSION_H
#define UDCOMPRESSION_H
#include "udPlatform.h"

enum udCompressionType
{
  udCT_None, // No compression (performs a udMemDup)
  udCT_RawDeflate, // Raw deflate compression
  udCT_ZlibDeflate, // Deflate compression with zlib header and footer
  udCT_GzipDeflate, // Deflate compression with gzip header and footer

  udCT_Count
};

// Compress a buffer, providing an allocate buffer of the compressed data
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type = udCT_ZlibDeflate);

// Decompress a buffer. If pInflatedSize is null, an error is returned if inflated size doesn't equal destSize exactly.
// In-place decompression is supported, pDest must equal pSource exactly, ie, overlapping decompression is not supported
udResult udCompression_Inflate(void *pDest, size_t destSize, const void *pSource, size_t sourceSize, size_t *pInflatedSize = nullptr, udCompressionType type = udCT_ZlibDeflate);

// Register a file handler to load files from an in-memory zip
udResult udCompression_RegisterMemoryZipHandler(void *pMem, size_t size);

// Remove registered handler (only one handler can be registered at a time)
udResult udMiniZ_DegisterMemoryZipHandler();

#endif // UDCOMPRESSION_H
