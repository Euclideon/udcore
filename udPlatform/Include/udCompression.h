#ifndef UDCOMPRESSION_H
#define UDCOMPRESSION_H
#include "udPlatform.h"

struct udMiniZCompressor;

udResult udMiniZCompressor_Create(udMiniZCompressor **ppCompressor);
udResult udMiniZCompressor_Destroy(udMiniZCompressor **ppCompressor);
udResult udMiniZCompressor_Deflate(udMiniZCompressor *pCompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pCompressedSize);

udResult udMiniZCompressor_InitStream(udMiniZCompressor *pCompressor, void *pDestBuffer, size_t size);
udResult udMiniZCompressor_DeflateStream(udMiniZCompressor *pCompressor, void *pStream, size_t size, size_t *pCompressedSize);



struct udMiniZDecompressor;

udResult udMiniZDecompressor_Create(udMiniZDecompressor **ppDecompressor);
udResult udMiniZDecompressor_Destroy(udMiniZDecompressor **ppDecompressor);
udResult udMiniZDecompressor_Inflate(udMiniZDecompressor *pDecompressor, void *pDest, size_t destLength, const void *pSource, size_t sourceLength, size_t *pInflatedSize);

size_t udMiniZDecompressor_GetStructureSize();


#endif // UDCOMPRESSION_H
