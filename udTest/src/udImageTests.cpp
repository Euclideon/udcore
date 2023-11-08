#include "gtest/gtest.h"
#include "udImage.h"
#include "udFile.h"

TEST(udImageTests, SaveLoad)
{
  enum
  {
    dim = 32,
    rawBufferSize = 8192
  };
  uint32_t imageData[dim * dim];
  udImage testImage = { dim, dim, 3, 0, imageData };
  udImage *pReload = nullptr;
  const char *pRawOutputFilename = nullptr;
  uint32_t sz;

  for (int i = 0; i < (dim * dim); ++i)
    imageData[i] = rand() | 0xff000000;

  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawOutputFilename, nullptr, 0, udCT_None, "testoutput", rawBufferSize));

  for (udImageSaveType saveType = (udImageSaveType)0; saveType < udIST_Max; saveType = (udImageSaveType)(saveType + 1))
  {
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(nullptr, pRawOutputFilename, nullptr, saveType));
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(&testImage, nullptr, nullptr, saveType));
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(&testImage, pRawOutputFilename, nullptr, (udImageSaveType)(udIST_Max + saveType)));
    // Test with a null for the size for just the first type
    ASSERT_EQ(udR_Success, udImage_Save(&testImage, pRawOutputFilename, &sz, saveType));
    EXPECT_EQ(udR_Success, udImage_Load(&pReload, pRawOutputFilename));
    if (pReload)
    {
      EXPECT_EQ(testImage.width, pReload->width);
      EXPECT_EQ(testImage.height, pReload->height);
      bool isLossy = (saveType == udIST_JPG);
      // Don't test lossy output (yet)
      if (!isLossy)
      {
        EXPECT_EQ(0, memcmp(testImage.pImageData, pReload->pImageData, dim * dim * 4));
      }
      udImage_Destroy(&pReload);
    }
  }
  udFree(pRawOutputFilename);
}

TEST(udImageTests, SaveLoadImageStreaming)
{
  const uint32_t dim = 32;
  uint32_t imageData[dim * dim];
  udImage testImage = { dim, dim, 3, 0, imageData };
  udImageStreaming *pReload = nullptr;
  udImageStreamingOnDisk *pOnDisk = nullptr;
  const char *pRawOutputFilename = nullptr;
  uint32_t sz;
  udFile *pLoadFile = nullptr;
  uint8_t *pRGB = nullptr;

  for (int i = 0; i < (dim * dim); ++i)
    imageData[i] = rand() | 0xff000000;

  // Test invalid parameters
  EXPECT_EQ(udR_InvalidParameter, udImageStreaming_Save(nullptr, &pOnDisk, nullptr, nullptr));
  EXPECT_EQ(udR_InvalidParameter, udImageStreaming_Save(&testImage, nullptr, nullptr, nullptr));

  ASSERT_EQ(udR_Success, udImageStreaming_Save(&testImage, &pOnDisk, &sz, "testdesc"));
  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawOutputFilename, pOnDisk, sz, udCT_None, "testoutput"));
  udFree(pOnDisk);

  ASSERT_EQ(udR_Success, udFile_Open(&pLoadFile, pRawOutputFilename, udFOF_Read));
  EXPECT_EQ(udR_Success, udImageStreaming_Load(&pReload, pLoadFile, 0));
  if (pReload)
  {
    EXPECT_EQ(testImage.width, pReload->width);
    EXPECT_EQ(testImage.height, pReload->height);
    EXPECT_EQ(udR_Success, udImageStreaming_GetImage24(pReload, &pRGB, udISF_TopLeft | udISF_ABGR));
    // Test each pixel as 24-bit
    for (int i = dim * dim - 1; i >= 0; --i)
      EXPECT_EQ(0, memcmp(testImage.pImageData + i, pRGB + (i * 3), 3));
    udFree(pRGB);
    // Grab the first mip and ensure it's half the size of the full image
    uint32_t mw, mh;
    EXPECT_EQ(udR_Success, udImageStreaming_GetImage24(pReload, &pRGB, udISF_TopLeft | udISF_ABGR, &mw, &mh, 1));
    udFree(pRGB);
    EXPECT_EQ(dim / 2, mw);
    EXPECT_EQ(dim / 2, mh);
    udImageStreaming_Destroy(&pReload);
  }
  udFile_Close(&pLoadFile);
  udFree(pRawOutputFilename);
}
