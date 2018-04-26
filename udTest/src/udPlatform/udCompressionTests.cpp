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
  EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input), udCT_Deflate));
  EXPECT_NE(input, pDeflated);
  EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
  EXPECT_NE(0, memcmp(input, pDeflated, udMin(UDARRAYSIZE(input), deflatedSize)));

  // Decompress
  EXPECT_EQ(udR_Success, udCompression_Inflate(inflated, UDARRAYSIZE(input), pDeflated, deflatedSize, &inflatedSize, udCT_Deflate));
  EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
  EXPECT_EQ(0, memcmp(input, inflated, udMin(UDARRAYSIZE(input), inflatedSize)));

  udFree(pDeflated);
}

TEST(udCompressionTests, Complex)
{
  const char input[] = "This string is *much* better than the previous string I thought of.";
  uint8_t inflated[1024];
  size_t inflatedSize = 0;
  uint8_t deflated[1024];
  void *pDeflated = nullptr;
  size_t deflatedSize = 0;

  // Compress
  EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input)));
  // To save issues of leaking, copy to local array and free allocated memory
  if (deflatedSize < UDARRAYSIZE(deflated))
  {
    memcpy(deflated, pDeflated, deflatedSize);
    udFree(pDeflated);
  }
  EXPECT_LT(deflatedSize, UDARRAYSIZE(input)); // Expect some compression to have occured
  EXPECT_NE(0, memcmp(input, deflated, udMin(UDARRAYSIZE(input), deflatedSize)));

  // Decompress
  EXPECT_EQ(udR_Success, udCompression_Inflate(inflated, UDARRAYSIZE(inflated), deflated, deflatedSize, &inflatedSize));
  EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
  EXPECT_EQ(0, memcmp(input, inflated, inflatedSize));
}

