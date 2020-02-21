#include "udImage.h"
#include "udFile.h"
#include "udMath.h"
#include "udCompression.h"
#include "../3rdParty/stb/stb_image.h"

#define UDIMAGE_FOURCC MAKE_FOURCC('U', 'D', 'T', 'X')
#define MAX_MIP_LEVELS 24 // Maximum texture size of 16M x 16M

struct udImageOnDisk
{
  uint32_t fourcc;
  uint32_t width, height;
  uint16_t sourceChannels;
  uint16_t mipCount;
  enum Flags { F_None = 0, F_PNG = 1, F_Force32 = 0xffffffff } flags;
  uint64_t reserved;
  int64_t mipOffsets[MAX_MIP_LEVELS];
  uint32_t mipSizes[MAX_MIP_LEVELS];
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2020
uint16_t udImage_GetMipCount(uint32_t width, uint32_t height)
{
  uint16_t mipCount = 1;
  while (width > 1 && height > 1)
  {
    width >>= 1;
    height >>= 1;
    ++mipCount;
  }
  return mipCount;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_Load(udImage **ppImage, const char *pFilename, bool generateMips)
{
  udResult result;
  void *pMem = nullptr;
  int64_t fileLen;

  UD_ERROR_CHECK(udFile_Load(pFilename, &pMem, &fileLen));
  UD_ERROR_CHECK(udImage_LoadFromMemory(ppImage, pMem, (size_t)fileLen, generateMips));

epilogue:
  udFree(pMem);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2020
// Generate mip images from pImage->apImageData[0]
udResult udImage_GenerateMipChain(udImage *pImage)
{
  udResult result;
  uint32_t prevW, prevH;

  UD_ERROR_NULL(pImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pImage->apImageData[0], udR_InvalidConfiguration);
  UD_ERROR_IF(pImage->mipCount <= 1, udR_InvalidConfiguration);

  prevW = pImage->width;
  prevH = pImage->height;
  for (uint16_t i = 1; i < pImage->mipCount; ++i)
  {
    // For now, each subsequent mip is just a 1/4 scale down of previous
    // TODO: This can be done much better, especially with polyphase filters for non power of two textures
    //       See http://download.nvidia.com/developer/Papers/2005/NP2_Mipmapping/NP2_Mipmap_Creation.pdf
    uint32_t mipW, mipH;
    pImage->GetMipLevelDimensions((uint16_t)i, &mipW, &mipH);
    udFree(pImage->apImageData[i]); // If for some reason memory is allocated
    pImage->apImageData[i] = udAllocType(uint32_t, mipW * mipH, udAF_None);
    UD_ERROR_NULL(pImage->apImageData[i], udR_MemoryAllocationFailure);
    for (uint32_t y = 0; y < mipH; ++y)
    {
      for (uint32_t x = 0; x < mipW; ++x)
      {
        // Do a filtered sample at the center of the 2x2 of the previous mip level
        pImage->apImageData[i][y * mipW + x] = udImage_Sample(pImage, (x * 2 + 1) / (float)prevW, (y * 2 + 1) / (float)prevH, udISF_Filter | udISG_ABGR | udISG_TopLeft, (uint16_t)(i - 1));
      }
    }
    prevW = mipW;
    prevH = mipH;
  }

  result = udR_Success;

epilogue:

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_LoadFromMemory(udImage **ppImage, const void *pMemory, size_t length, bool generateMips)
{
  udResult result;
  udImage *pImage = nullptr;
  const stbi_uc *pSTBIImage = nullptr;
  const udImageOnDisk *pOnDisk = static_cast<const udImageOnDisk *>(pMemory);

  UD_ERROR_NULL(ppImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pMemory, udR_InvalidParameter_);
  UD_ERROR_IF(length == 0, udR_InvalidParameter_);

  if (pOnDisk->fourcc == UDIMAGE_FOURCC)
  {
    pImage = (udImage *)udAllocFlags(sizeof(udImage) + (pOnDisk->mipCount - 1) * sizeof(pImage->apImageData[0]), udAF_Zero);
    UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
    UD_ERROR_IF(pOnDisk->mipCount >= MAX_MIP_LEVELS, udR_CorruptData);
    pImage->width = pOnDisk->width;
    pImage->height = pOnDisk->height;
    pImage->sourceChannels = pOnDisk->sourceChannels;
    pImage->mipCount = pOnDisk->mipCount;
    UD_ERROR_IF(pOnDisk->mipOffsets[0] + pOnDisk->mipSizes[0] > (int64_t)length, udR_CorruptData);
    uint32_t mipW, mipH;
    for (uint16_t i = 0; i < pOnDisk->mipCount; ++i)
    {
      const uint8_t *pMipSourceImage = ((const uint8_t *)pOnDisk) + pOnDisk->mipOffsets[i];
      pImage->GetMipLevelDimensions(i, &mipW, &mipH);
      pImage->apImageData[i] = udAllocType(uint32_t, mipW * mipH, udAF_None);
      UD_ERROR_NULL(pImage->apImageData[i], udR_MemoryAllocationFailure);
      if (pOnDisk->flags & udImageOnDisk::F_PNG)
      {
        int w, h, sc; // Some plain integers for call to 3rd party API
        pSTBIImage = stbi_load_from_memory((const stbi_uc *)pMipSourceImage, (int)pOnDisk->mipSizes[i], &w, &h, &sc, 4);
        UD_ERROR_NULL(pSTBIImage, udR_ImageLoadFailure);
        UD_ERROR_IF(w != (int)mipW || h != (int)mipH, udR_CorruptData);
        memcpy(pImage->apImageData[i], pSTBIImage, (mipW * mipH * 4));
        stbi_image_free((void *)pSTBIImage);
        pSTBIImage = nullptr;
      }
      else
      {
        UD_ERROR_IF(pOnDisk->mipSizes[i] != (mipW * mipH * 4), udR_CorruptData);
        memcpy(pImage->apImageData[i], pMipSourceImage, pOnDisk->mipSizes[i]);
      }
      if (!generateMips)
        break;
    }
  }
  else
  {
    int w, h, sc; // Some plain integers for call to 3rd party API
    pSTBIImage = stbi_load_from_memory((const stbi_uc *)pMemory, (int)length, &w, &h, &sc, 4);
    UD_ERROR_NULL(pSTBIImage, udR_ImageLoadFailure);
    uint16_t mipCount = generateMips ? udImage_GetMipCount((uint32_t)w, (uint32_t)h) : 1;

    pImage = (udImage *)udAllocFlags(sizeof(udImage) + (mipCount - 1) * sizeof(pImage->apImageData[0]), udAF_Zero);
    UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
    pImage->width = (uint32_t)w;
    pImage->height = (uint32_t)h;
    pImage->sourceChannels = (uint16_t)sc;
    pImage->mipCount = mipCount;

    // Duplicate stbi image data for the top mip
    pImage->apImageData[0] = (uint32_t *)udMemDup(pSTBIImage, w * h * 4, 0, udAF_None);
    UD_ERROR_NULL(pImage->apImageData[0], udR_MemoryAllocationFailure);
    stbi_image_free((void *)pSTBIImage);
    pSTBIImage = nullptr;

    if (generateMips)
      udImage_GenerateMipChain(pImage);
  }

  *ppImage = pImage;
  pImage = nullptr;
  result = udR_Success;

epilogue:
  if (pSTBIImage)
  {
    stbi_image_free((void *)pSTBIImage);
    pSTBIImage = nullptr;
  }
  if (pImage)
    udImage_Destroy(&pImage);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2020
udResult udImage_SaveStreamable(const udImage *pImage, udFile *pFile, int64_t offset, uint32_t *pSaveSize)
{
  udResult result;
  udImageOnDisk onDisk;
  void *apCompressedMipImages[MAX_MIP_LEVELS];
  udImage *pTempImage = nullptr; // Used to generate mips if necessary
  uint16_t mipCount;

  memset(apCompressedMipImages, 0, sizeof(apCompressedMipImages));
  UD_ERROR_NULL(pImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pImage->apImageData[0], udR_InvalidConfiguration);
  UD_ERROR_NULL(pFile, udR_InvalidParameter_);

  mipCount = udImage_GetMipCount(pImage->width, pImage->height);
  if (pImage->mipCount != mipCount)
  {
    // If we need to generate mips make a temporary image to write out, and destroy it at the end
    pTempImage = (udImage *)udAllocFlags(sizeof(udImage) + (mipCount - 1) * sizeof(pImage->apImageData[0]), udAF_Zero);
    UD_ERROR_NULL(pTempImage, udR_MemoryAllocationFailure);
    pTempImage->width = pImage->width;
    pTempImage->height = pImage->height;
    pTempImage->sourceChannels = pImage->sourceChannels;
    pTempImage->mipCount = mipCount;
    pTempImage->apImageData[0] = (uint32_t*)udMemDup(pImage->apImageData[0], pImage->width * pImage->height * 4, 0, udAF_None);
    UD_ERROR_NULL(pTempImage->apImageData[0], udR_InvalidConfiguration);
    UD_ERROR_CHECK(udImage_GenerateMipChain(pTempImage));
    pImage = pTempImage;
  }

  memset(&onDisk, 0, sizeof(onDisk));
  onDisk.fourcc = UDIMAGE_FOURCC;
  onDisk.width = pImage->width;
  onDisk.height = pImage->height;
  onDisk.sourceChannels = pImage->sourceChannels;
  onDisk.mipCount = mipCount;
  onDisk.flags = udImageOnDisk::F_PNG;

  for (uint16_t i = 0; i < mipCount; ++i)
  {
    uint32_t mipW, mipH;
    pImage->GetMipLevelDimensions(i, &mipW, &mipH);
    if (onDisk.flags & udImageOnDisk::F_PNG)
    {
      size_t temp;
      UD_ERROR_CHECK(udCompression_CreatePNG(&apCompressedMipImages[i], &temp, (const uint8_t *)pImage->apImageData[i], mipW, mipH, 4)); // TODO: Make a 24-bit version first
      onDisk.mipSizes[i] = (uint32_t)temp;
    }
    else
    {
      onDisk.mipSizes[i] = mipW * mipH;
    }
  }

  // Calculate the offsets so that the largest mip appears last on the disk
  for (int i = mipCount - 1; i >= 0; --i)
    onDisk.mipOffsets[i] = (i == mipCount - 1) ? sizeof(onDisk) : onDisk.mipOffsets[i + 1] + onDisk.mipSizes[i + 1];

  // Now write the data in order
  UD_ERROR_CHECK(udFile_Write(pFile, &onDisk, sizeof(onDisk), offset, udFSW_SeekSet));
  for (int i = mipCount - 1; i >= 0; --i)
    UD_ERROR_CHECK(udFile_Write(pFile, (onDisk.flags & udImageOnDisk::F_PNG) ? apCompressedMipImages[i] : pImage->apImageData[i], onDisk.mipSizes[i], offset + onDisk.mipOffsets[i], udFSW_SeekSet));

  if (pSaveSize)
    *pSaveSize = (uint32_t)(onDisk.mipOffsets[0] + onDisk.mipSizes[0]); // Since mip 0 is always last, just taking it's offset and size is the size of the entire save
  result = udR_Success;

epilogue:
  if (pTempImage)
    udImage_Destroy(&pTempImage);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2020
udResult udImage_LoadStreamable(udImage **ppImage, udFile *pFile, int64_t offset)
{
  udResult result;
  udImageOnDisk onDisk;
  udImage *pImage = nullptr;

  UD_ERROR_NULL(ppImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pFile, udR_InvalidParameter_);

  UD_ERROR_CHECK(udFile_Read(pFile, &onDisk, sizeof(onDisk), offset, udFSW_SeekSet));
  UD_ERROR_IF(onDisk.fourcc != UDIMAGE_FOURCC, udR_ObjectTypeMismatch);
  UD_ERROR_IF(onDisk.mipCount > MAX_MIP_LEVELS, udR_CorruptData);
  pImage = (udImage *)udAllocFlags(sizeof(udImage) + (onDisk.mipCount - 1) * sizeof(pImage->apImageData[0]) + sizeof(onDisk), udAF_Zero);
  UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
  pImage->width = onDisk.width;
  pImage->height = onDisk.height;
  pImage->sourceChannels = onDisk.sourceChannels;
  pImage->mipCount = onDisk.mipCount;
  pImage->flags = udImage::IF_Streaming;
  memcpy(pImage->apImageData + onDisk.mipCount, &onDisk, sizeof(onDisk));

  *ppImage = pImage;
  pImage = nullptr;
  result = udR_Success;

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2020
udResult udImage_LoadMip(udImage *pImage, uint16_t mipLevel, udFile *pFile, int64_t offset)
{
  udResult result;
  uint32_t mipW, mipH;
  const stbi_uc *pSTBIImage = nullptr;
  void *pMipSourceImage = nullptr;

  UD_ERROR_NULL(pImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pFile, udR_InvalidParameter_);
  UD_ERROR_IF(mipLevel >= pImage->mipCount, udR_InvalidParameter_);
  UD_ERROR_IF((pImage->flags & udImage::IF_Streaming) == 0, udR_InvalidConfiguration);

  if (pImage->apImageData[mipLevel] == nullptr)
  {
    udImageOnDisk *pOnDisk = (udImageOnDisk *)(pImage->apImageData + pImage->mipCount);
    pMipSourceImage = udAlloc(pOnDisk->mipSizes[mipLevel]);
    UD_ERROR_NULL(pMipSourceImage, udR_MemoryAllocationFailure);
    UD_ERROR_CHECK(udFile_Read(pFile, pMipSourceImage, pOnDisk->mipSizes[mipLevel], offset + pOnDisk->mipOffsets[mipLevel], udFSW_SeekSet));

    pImage->GetMipLevelDimensions(mipLevel, &mipW, &mipH);
    if (pOnDisk->flags & udImageOnDisk::F_PNG)
    {
      int w, h, sc; // Some plain integers for call to 3rd party API
      pSTBIImage = stbi_load_from_memory((const stbi_uc *)pMipSourceImage, (int)pOnDisk->mipSizes[mipLevel], &w, &h, &sc, 4);
      UD_ERROR_NULL(pSTBIImage, udR_ImageLoadFailure);
      UD_ERROR_IF(w != (int)mipW || h != (int)mipH, udR_CorruptData);
      pImage->apImageData[mipLevel] = udAllocType(uint32_t, mipW * mipH, udAF_None);
      UD_ERROR_NULL(pImage->apImageData[mipLevel], udR_MemoryAllocationFailure);
      memcpy(pImage->apImageData[mipLevel], pSTBIImage, (mipW * mipH * 4));
    }
    else
    {
      UD_ERROR_IF(pOnDisk->mipSizes[mipLevel] != (mipW * mipH * 4), udR_CorruptData);
      pImage->apImageData[mipLevel] = (uint32_t*)pMipSourceImage;
      pMipSourceImage = nullptr;
    }
  }
  result = udR_Success;

epilogue:
  if (pSTBIImage)
  {
    stbi_image_free((void *)pSTBIImage);
    pSTBIImage = nullptr;
  }
  udFree(pMipSourceImage);

  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, February 2019
uint32_t udImage_Sample(udImage *pImage, float u, float v, udImageSampleFlags flags, uint16_t mipLevel)
{
  mipLevel = udMin(mipLevel, uint16_t(pImage->mipCount - 1));
  // If requesting a lower mip that isn't present, go higher (for now)
  while (mipLevel < (pImage->mipCount - 1) && pImage->apImageData[mipLevel] == nullptr)
    ++mipLevel;
  if (pImage->apImageData[mipLevel] == nullptr)
    return 0; // TODO

  uint32_t width, height;
  pImage->GetMipLevelDimensions(mipLevel, &width, &height);

  if (flags & udISF_Clamp)
  {
    u = udClamp(u, 0.f, 1.f);
    v = udClamp(v, 0.f, 1.f);
  }

  u =  u * width;
  if (flags & udISG_TopLeft)
    v = v * height;
  else
    v = -v * height;

  while (u < 0.0f || u >= width)
    u = u - width * udFloor((u / width));
  while (v < 0.0f || v >= height)
    v = v - height * udFloor((v / height));

  int x = (int)u;
  int y = (int)v;
  if (flags & udISF_Filter)
  {
    int u1 = (int)(u * 256) & 0xff; // Get most most significant bits of PRECISION
    int v1 = (int)(v * 256) & 0xff; // Get most most significant bits of PRECISION
    int u0 = 256 - u1;
    int v0 = 256 - v1;

    int x0 = udClamp(x + 0, 0, (int)width - 1);
    int x1 = udClamp(x + 1, 0, (int)width - 1);
    int y0 = udClamp(y + 0, 0, (int)height - 1);
    int y1 = udClamp(y + 1, 0, (int)height - 1);

    int a = u0 * v0;
    int b = u1 * v0;
    int c = u0 * v1;
    int d = u1 * v1;

    uint32_t c0 = (pImage->apImageData[mipLevel][x0 + y0 * width]);
    uint32_t c1 = (pImage->apImageData[mipLevel][x1 + y0 * width]);
    uint32_t c2 = (pImage->apImageData[mipLevel][x0 + y1 * width]);
    uint32_t c3 = (pImage->apImageData[mipLevel][x1 + y1 * width]);

    uint32_t bfB = 0x00ff0000 & (((c0 >> 16)      * a) + ((c1 >> 16)      * b) + ((c2 >> 16)      * c) + ((c3 >> 16)      * d));
    uint32_t bfG = 0xff000000 & (((c0 & 0x00ff00) * a) + ((c1 & 0x00ff00) * b) + ((c2 & 0x00ff00) * c) + ((c3 & 0x00ff00) * d));
    uint32_t bfR = 0x00ff0000 & (((c0 & 0x0000ff) * a) + ((c1 & 0x0000ff) * b) + ((c2 & 0x0000ff) * c) + ((c3 & 0x0000ff) * d));

    if (flags & udISG_ABGR)
      return 0xff000000 | bfB | ((bfG | bfR) >> 16);
    else
      return 0xff000000 | bfR | ((bfG | bfB) >> 16);
  }
  else
  {
    uint32_t c = (pImage->apImageData[mipLevel][x + y * width]); // STBI returns colors as ABGR

    if (flags & udISG_ABGR)
      return c;
    else
      return (c & 0xff00ff00) | ((c & 0xff) << 16) | ((c >> 16) & 0xff);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
void udImage_Destroy(udImage **ppImage)
{
  if (ppImage && *ppImage)
  {
    udImage *pImage = *ppImage;
    *ppImage = nullptr;
    for (uint16_t i = 0; i < pImage->mipCount; ++i)
      udFree(pImage->apImageData[i]);
    udFree(pImage);
  }
}
