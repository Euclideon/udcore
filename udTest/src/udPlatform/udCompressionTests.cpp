#include "gtest/gtest.h"
#include "udCompression.h"
#include "udMath.h"

TEST(udCompressionTests, Basic)
{
  const char input[] = "This is the best string I could think of.";
  void *pDeflated = nullptr;
  size_t deflatedSize = 0;
  char inflated[UDARRAYSIZE(input)] = {};
  size_t inflatedSize = 0;

  // Compress
  EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input), udCT_MiniZ));
  EXPECT_NE(input, pDeflated);
  EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
  EXPECT_NE(0, memcmp(input, pDeflated, udMin(UDARRAYSIZE(input), deflatedSize)));

  // Decompress
  EXPECT_EQ(udR_Success, udCompression_Inflate(inflated, UDARRAYSIZE(input), pDeflated, deflatedSize, &inflatedSize, udCT_MiniZ));
  EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
  EXPECT_EQ(0, memcmp(input, inflated, udMin(UDARRAYSIZE(input), inflatedSize)));

  udFree(pDeflated);
}

TEST(udCompressionTests, Complex)
{
  const char input[] = "This string is *much* better than the previous string I thought of.";
  udMiniZCompressor *pCompressor = nullptr;
  udMiniZDecompressor *pDecompressor = nullptr;
  uint8_t deflated[1024] = {};
  size_t deflatedSize = 0;
  uint8_t inflated[1024] = {};
  size_t inflatedSize = 0;

  // Compress
  EXPECT_EQ(udR_Success, udMiniZCompressor_Create(&pCompressor));
  EXPECT_NE(nullptr, pCompressor);
  EXPECT_LE(UDARRAYSIZE(input), udMiniZCompressor_DeflateBounds(pCompressor, UDARRAYSIZE(input)));
  EXPECT_EQ(udR_Success, udMiniZCompressor_Deflate(pCompressor, deflated, UDARRAYSIZE(deflated), input, UDARRAYSIZE(input), &deflatedSize));
  EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
  EXPECT_NE(0, memcmp(input, deflated, udMin(UDARRAYSIZE(input), deflatedSize)));
  EXPECT_EQ(udR_Success, udMiniZCompressor_Destroy(&pCompressor));
  EXPECT_EQ(nullptr, pCompressor);

  // Decompress
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Create(&pDecompressor));
  EXPECT_NE(nullptr, pDecompressor);
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Inflate(pDecompressor, inflated, UDARRAYSIZE(inflated), deflated, deflatedSize, &inflatedSize));
  EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
  EXPECT_EQ(0, memcmp(input, inflated, inflatedSize));
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Destroy(&pDecompressor));
  EXPECT_EQ(nullptr, pDecompressor);
}

TEST(udCompressionTests, ComplexStream)
{
  const char input[] = "This string is *much* better than the previous string I thought of.";
  udMiniZCompressor *pCompressor = nullptr;
  udMiniZDecompressor *pDecompressor = nullptr;
  uint8_t deflated[1024] = {};
  size_t deflatedSize = 0;
  uint8_t inflated[1024] = {};
  size_t inflatedSize = 0;

  // Compress
  EXPECT_EQ(udR_Success, udMiniZCompressor_Create(&pCompressor));
  EXPECT_NE(nullptr, pCompressor);
  EXPECT_LE(UDARRAYSIZE(input), udMiniZCompressor_DeflateBounds(pCompressor, UDARRAYSIZE(input)));
  EXPECT_EQ(udR_Success, udMiniZCompressor_InitStream(pCompressor, deflated, UDARRAYSIZE(deflated)));
  EXPECT_EQ(udR_Success, udMiniZCompressor_DeflateStream(pCompressor, (void*)input, UDARRAYSIZE(input), &deflatedSize));
  EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
  EXPECT_NE(0, memcmp(input, deflated, udMin(UDARRAYSIZE(input), deflatedSize)));
  EXPECT_EQ(udR_Success, udMiniZCompressor_Destroy(&pCompressor));
  EXPECT_EQ(nullptr, pCompressor);

  // Decompress
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Create(&pDecompressor));
  EXPECT_NE(nullptr, pDecompressor);
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Inflate(pDecompressor, inflated, UDARRAYSIZE(inflated), deflated, deflatedSize, &inflatedSize));
  EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
  EXPECT_EQ(0, memcmp(input, inflated, inflatedSize));
  EXPECT_EQ(udR_Success, udMiniZDecompressor_Destroy(&pDecompressor));
  EXPECT_EQ(nullptr, pDecompressor);
}
