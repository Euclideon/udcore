#include "udFile.h"
#include "udImage.h"
#include "gtest/gtest.h"

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
  const char *pRawOutput = nullptr;
  uint32_t sz;

  for (int i = 0; i < (dim * dim); ++i)
    imageData[i] = rand() | 0xff000000;

  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawOutput, nullptr, 0, udCT_None, "testoutput", rawBufferSize));

  for (udImageSaveType saveType = (udImageSaveType)0; saveType < udIST_Max; saveType = (udImageSaveType)(saveType + 1))
  {
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(nullptr, pRawOutput, nullptr, saveType));
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(&testImage, nullptr, nullptr, saveType));
    EXPECT_EQ(udR_InvalidParameter, udImage_Save(&testImage, pRawOutput, nullptr, (udImageSaveType)(udIST_Max + saveType)));
    // Test with a null for the size for just the first type
    ASSERT_EQ(udR_Success, udImage_Save(&testImage, pRawOutput, &sz, saveType));
    EXPECT_EQ(udR_Success, udImage_Load(&pReload, pRawOutput));
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
  udFree(pRawOutput);
}
