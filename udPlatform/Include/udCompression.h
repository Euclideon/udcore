#ifndef UDCOMPRESSION_H
#define UDCOMPRESSION_H
#include "udPlatform.h"

enum udCompressionType
{
  udCT_None,    // No compression (passthru)
  udCT_MiniZ,   // MiniZ

  udCT_Count
};

// Compress a buffer, providing an allocate buffer of the compressed data
udResult udCompression_Deflate(void **ppDest, size_t *pDestSize, const void *pSource, size_t sourceSize, udCompressionType type = udCT_MiniZ);

// Decompress a buffer. If pInflatedSize is null, an error is returned if inflated size doesn't equal destSize exactly.
udResult udCompression_Inflate(void *pDest, size_t destSize, const void *pSource, size_t sourceSize, size_t *pInflatedSize = nullptr, udCompressionType type = udCT_MiniZ);


// *************************** DEPRECATED API *****************************

struct udMiniZCompressor;

udResult udMiniZCompressor_Create(udMiniZCompressor **ppCompressor);
udResult udMiniZCompressor_Destroy(udMiniZCompressor **ppCompressor);
udResult udMiniZCompressor_Deflate(udMiniZCompressor *pCompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pCompressedSize);
size_t udMiniZCompressor_DeflateBounds(udMiniZCompressor *pCompressor, size_t sourceLength);

udResult udMiniZCompressor_InitStream(udMiniZCompressor *pCompressor, void *pDestBuffer, size_t size);
udResult udMiniZCompressor_DeflateStream(udMiniZCompressor *pCompressor, void *pStream, size_t size, size_t *pCompressedSize);



struct udMiniZDecompressor;

udResult udMiniZDecompressor_Create(udMiniZDecompressor **ppDecompressor);
udResult udMiniZDecompressor_Destroy(udMiniZDecompressor **ppDecompressor);
udResult udMiniZDecompressor_Inflate(udMiniZDecompressor *pDecompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pInflatedSize);

size_t udMiniZDecompressor_GetStructureSize();


// Register a file handler to load files from an in-memory zip
udResult udMiniZ_RegisterMemoryFileHandler(void *pMem, size_t size);

// Remove registered handler (only one handler can be registered at a time)
udResult udMiniZ_DegisterFileHandler();

#endif // UDCOMPRESSION_H
