#ifndef UDCOMPRESSION_H
#define UDCOMPRESSION_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, November 2017
//
// This module wraps zlib/miniz/gzip etc
//

#include "udPlatform.h"

enum udCompressionType
{
  udCT_None,        // No compression (performs a udMemDup)
  udCT_RawDeflate,  // Raw deflate compression
  udCT_ZlibDeflate, // Deflate compression with zlib header and footer
  udCT_GzipDeflate, // Deflate compression with gzip header and footer

  udCT_Count
};
const char *udCompressionTypeAsString(udCompressionType type); // Return a string of the enum (eg "RawDeflate"), or null if not defined

// Compress a buffer, providing an allocate buffer of the compressed data
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type = udCT_ZlibDeflate);

// Decompress a buffer. If pInflatedSize is null, an error is returned if inflated size doesn't equal destSize exactly.
// In-place decompression is supported, pDest must equal pSource exactly, ie, overlapping decompression is not supported
udResult udCompression_Inflate(void *pDest, size_t destSize, const void *pSource, size_t sourceSize, size_t *pInflatedSize = nullptr, udCompressionType type = udCT_ZlibDeflate);

// Generate a compressed PNG from a raw image, caller to udFree the memory
udResult udCompression_CreatePNG(void **ppPNG, size_t *pPNGLen, const uint8_t *pImage, int width, int height, int channels);

#endif // UDCOMPRESSION_H
