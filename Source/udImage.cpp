#include "udImage.h"
#include "udFile.h"
#include "udCompression.h"
#include "udStringUtil.h"
#include "udThread.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <algorithm>

// Helpers to ensure encoding scheme for cell indices stays in one place
static inline uint32_t PackCellIndex(uint32_t cellX, uint32_t cellY, uint32_t mip) { return mip | (cellX << 8) | (cellY << 16); }
static inline void UnpackCellIndex(uint32_t cellIndex, uint32_t *pCellX, uint32_t *pCellY, uint32_t *pMip = nullptr)
{
  if (pCellX)
    *pCellX = (cellIndex >> 8) & 0xff;
  if (pCellY)
    *pCellY = (cellIndex >> 16) & 0xff;
  if (pMip)
    *pMip = (cellIndex >> 0) & 0xff;
}
static inline void GetUVAsPixelIndex(float u, float v, udImageSampleFlags flags, uint32_t imageWidth, uint32_t imageHeight, uint32_t *pX, uint32_t *pY)
{
  if (flags & udISF_Clamp)
  {
    u = std::clamp(u, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);
  }

  u = u * imageWidth;
  if (flags & udISF_TopLeft)
    v = v * imageHeight;
  else
    v = -v * imageHeight;

  while (u < 0.0f || u >= imageWidth)
    u = u - imageWidth * floorf((u / imageWidth));
  while (v < 0.0f || v >= imageHeight)
    v = v - imageHeight * floorf((v / imageHeight));

  *pX = (uint32_t)u;
  *pY = (uint32_t)v;
}




// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_Load(udImage **ppImage, const char *pFilename, bool infoOnly)
{
  udResult result;
  udFile *pFile = nullptr;
  void *pMem = nullptr;
  int64_t fileLen;

  if (infoOnly)
  {
    const int headerMaxLen = 512; // Let's hope the dimensions are in the first half a KB
    pMem = udAlloc(headerMaxLen);
    UD_ERROR_NULL(pMem, udR_MemoryAllocationFailure);
    UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_FastOpen | udFOF_Read));
    size_t actRead;
    UD_ERROR_CHECK(udFile_Read(pFile, pMem, headerMaxLen, 0, udFSW_SeekSet, &actRead));
    fileLen = (int64_t)actRead;
  }
  else
  {
    UD_ERROR_CHECK(udFile_Load(pFilename, &pMem, &fileLen));
  }
  UD_ERROR_CHECK(udImage_LoadFromMemory(ppImage, pMem, (size_t)fileLen, infoOnly));
  // Set to true for debugging
  if constexpr (false)
    udDebugPrintf("%s %d x %d to %p (%s)\n", infoOnly ? "Read header" : "Loaded", (*ppImage)->width, (*ppImage)->height, (void*)*ppImage, pFilename);

epilogue:
  udFile_Close(&pFile);
  udFree(pMem);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_LoadFromMemory(udImage **ppImage, const void *pMemory, size_t length, bool infoOnly)
{
  udResult result;
  udImage *pImage = nullptr;
  const stbi_uc *pSTBIImage = nullptr;
  int w, h, sc; // Some plain integers for call to 3rd party API

  UD_ERROR_NULL(ppImage, udR_InvalidParameter);
  UD_ERROR_NULL(pMemory, udR_InvalidParameter);
  UD_ERROR_IF(length == 0, udR_InvalidParameter);

  if (infoOnly)
  {
    UD_ERROR_IF(stbi_info_from_memory((const stbi_uc *)pMemory, (int)length, &w, &h, &sc) != 1, udR_ImageLoadFailure);
  }
  else
  {
    pSTBIImage = stbi_load_from_memory((const stbi_uc *)pMemory, (int)length, &w, &h, &sc, 4);
    UD_ERROR_NULL(pSTBIImage, udR_ImageLoadFailure);
  }

  pImage = (udImage *)udAllocFlags(sizeof(udImage), udAF_Zero);
  UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
  pImage->width = (uint32_t)w;
  pImage->height = (uint32_t)h;
  pImage->sourceChannels = (uint16_t)sc;

  if (!infoOnly)
  {
    // Duplicate stbi image data
    pImage->pImageData = (uint32_t *)udMemDup(pSTBIImage, w * h * 4, 0, udAF_None);
    UD_ERROR_NULL(pImage->pImageData, udR_MemoryAllocationFailure);
    stbi_image_free((void *)pSTBIImage);
    pSTBIImage = nullptr;
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

struct stbiWriteContext
{
  udFile *pFile;
  udResult *pResult;
  uint32_t *pBytesWritten;
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2019
static void stbiWriteCallback(void *_pContext, void *pData, int size)
{
  stbiWriteContext *pContext = (stbiWriteContext *)_pContext;
  // Only attempt to write if there isn't a previous error
  if (*pContext->pResult == udR_Success)
    *pContext->pResult = udFile_Write(pContext->pFile, pData, size);
  if (pContext->pBytesWritten && *pContext->pResult == udR_Success)
    *pContext->pBytesWritten += size;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, February 2019
static void stbiWriteWrapper(stbiWriteContext *pContext, uint32_t width, uint32_t height, const void *pRGB, udImageSaveType saveType)
{
  if (pContext->pResult)
    *pContext->pResult = udR_Success;

  if (pContext->pBytesWritten)
    *pContext->pBytesWritten = 0;

  switch (saveType)
  {
    case udIST_PNG:
      if (stbi_write_png_to_func(stbiWriteCallback, pContext, width, height, 3, pRGB, 0) == 0)
        *pContext->pResult = udR_Failure;
      break;

    case udIST_JPG:
      if (stbi_write_jpg_to_func(stbiWriteCallback, pContext, width, height, 3, pRGB, 80) == 0) // quality value of 80
        *pContext->pResult = udR_Failure;
      break;

    case udIST_Max:
      *pContext->pResult = udR_InvalidParameter;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_Save(const udImage *pImage, const char *pFilename, uint32_t *pSaveSize, udImageSaveType saveType)
{
  udResult result;
  void *pRGB = nullptr; // Sadly, stbi has no support for writing 24-bit from a 32-bit source, so we must shrink
  uint8_t *pSource, *pDest; // For shrinking
  uint32_t count;
  stbiWriteContext writeContext;

  UD_ERROR_NULL(pImage, udR_InvalidParameter);
  UD_ERROR_NULL(pFilename, udR_InvalidParameter);
  UD_ERROR_IF(saveType >= udIST_Max, udR_InvalidParameter);
  count = pImage->width * pImage->height;
  pRGB = udAlloc(count * 3);
  UD_ERROR_NULL(pRGB, udR_MemoryAllocationFailure);
  pSource = (uint8_t *)pImage->pImageData;
  pDest = (uint8_t*)pRGB;
  while (count--)
  {
    *pDest++ = pSource[0];
    *pDest++ = pSource[1];
    *pDest++ = pSource[2];
    pSource += 4;
  }

  writeContext.pResult = &result;
  writeContext.pBytesWritten = pSaveSize;
  UD_ERROR_CHECK(udFile_Open(&writeContext.pFile, pFilename, udFOF_Create | udFOF_Write));
  stbiWriteWrapper(&writeContext, pImage->width, pImage->height, pRGB, saveType);
  udFile_Close(&writeContext.pFile);

epilogue:
  udFree(pRGB);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
uint32_t udImage_Sample(udImage *pImage, float u, float v, udImageSampleFlags flags)
{
  if (pImage->pImageData == nullptr)
    return 0;
  uint32_t x, y;

  GetUVAsPixelIndex(u, v, flags, pImage->width, pImage->height, &x, &y);

  if (flags & udISF_Filter)
  {
    uint32_t u1 = (uint32_t)(u * 256) & 0xff; // Get most most significant bits of PRECISION
    uint32_t v1 = (uint32_t)(v * 256) & 0xff; // Get most most significant bits of PRECISION
    uint32_t u0 = 256 - u1;
    uint32_t v0 = 256 - v1;

    uint32_t x0 = std::clamp(x + 0, 0U, (uint32_t)pImage->width - 1);
    uint32_t x1 = std::clamp(x + 1, 0U, (uint32_t)pImage->width - 1);
    uint32_t y0 = std::clamp(y + 0, 0U, (uint32_t)pImage->height - 1);
    uint32_t y1 = std::clamp(y + 1, 0U, (uint32_t)pImage->height - 1);

    uint32_t a = u0 * v0;
    uint32_t b = u1 * v0;
    uint32_t c = u0 * v1;
    uint32_t d = u1 * v1;

    uint32_t c0 = (pImage->pImageData[x0 + y0 * pImage->width]);
    uint32_t c1 = (pImage->pImageData[x1 + y0 * pImage->width]);
    uint32_t c2 = (pImage->pImageData[x0 + y1 * pImage->width]);
    uint32_t c3 = (pImage->pImageData[x1 + y1 * pImage->width]);

    uint32_t bfB = 0x00ff0000 & (((c0 >> 16)      * a) + ((c1 >> 16)      * b) + ((c2 >> 16)      * c) + ((c3 >> 16)      * d));
    uint32_t bfG = 0xff000000 & (((c0 & 0x00ff00) * a) + ((c1 & 0x00ff00) * b) + ((c2 & 0x00ff00) * c) + ((c3 & 0x00ff00) * d));
    uint32_t bfR = 0x00ff0000 & (((c0 & 0x0000ff) * a) + ((c1 & 0x0000ff) * b) + ((c2 & 0x0000ff) * c) + ((c3 & 0x0000ff) * d));

    if (flags & udISF_ABGR)
      return 0xff000000 | bfB | ((bfG | bfR) >> 16);
    else
      return 0xff000000 | bfR | ((bfG | bfB) >> 16);
  }
  else
  {
    uint32_t c = (pImage->pImageData[x + y * pImage->width]); // STBI returns colors as ABGR

    if (flags & udISF_ABGR)
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
    // Uncomment for debugging
    //udDebugPrintf("Freeing %d x %d at %p\n", (*ppImage)->width, (*ppImage)->height, (void*)*ppImage);
    udImage *pImage = *ppImage;
    *ppImage = nullptr;
    udFree(pImage->pImageData);
    udFree(pImage);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_Save(const udImage *pImage, udImageStreamingOnDisk **ppOnDisk, uint32_t *pSaveSize, const char *pNameDesc)
{
  udResult result;
  udImageStreamingOnDisk *pOnDisk = nullptr;
  uint32_t saveSize = (uint32_t)sizeof(udImageStreamingOnDisk);
  uint16_t mipCount = 0;
  uint32_t mipW, mipH;
  uint8_t *p24BitData = nullptr;
  uint8_t *pOut, *pIn;

  UD_ERROR_NULL(pImage, udR_InvalidParameter);
  UD_ERROR_NULL(pSaveSize, udR_InvalidParameter);

  mipW = pImage->width;
  mipH = pImage->height;
  do
  {
    saveSize += mipW * mipH * 3;
    mipW = std::max(1U, mipW >> 1);
    mipH = std::max(1U, mipH >> 1);
    ++mipCount;
  } while (mipW > 1 && mipH > 1);

  if (ppOnDisk)
  {
    pOnDisk = (udImageStreamingOnDisk *)udAlloc(saveSize);
    UD_ERROR_NULL(pOnDisk, udR_MemoryAllocationFailure);
    memset(pOnDisk, 0, sizeof(udImageStreamingOnDisk));
    pOnDisk->fourcc = udImageStreaming::Fourcc;
    pOnDisk->width = pImage->width;
    pOnDisk->height = pImage->height;
    pOnDisk->mipCount = mipCount;
    udStrcpy(pOnDisk->name, pNameDesc);
    pOnDisk->offsetToMip0 = (uint16_t)sizeof(udImageStreamingOnDisk);

    // Make a 24-bit copy of the source image
    p24BitData = udAllocType(uint8_t, pImage->width * pImage->height * 3, udAF_None);
    UD_ERROR_NULL(p24BitData, udR_MemoryAllocationFailure);
    pOut = p24BitData;
    pIn = (uint8_t*)(pImage->pImageData);
    for (uint32_t y = 0; y < pImage->height; ++y)
    {
      for (uint32_t x = 0; x < pImage->width; ++x)
      {
        *pOut++ = *pIn++;
        *pOut++ = *pIn++;
        *pOut++ = *pIn++;
        ++pIn; // Skip alpha
      }
    }

    pOut = ((uint8_t *)pOnDisk) + pOnDisk->offsetToMip0;
    mipW = pImage->width;
    mipH = pImage->height;
    for (uint16_t mip = 0; mip < mipCount; ++mip)
    {
      uint32_t cellCountX = (mipW + udImageStreamingOnDisk::TileSize - 1) / udImageStreamingOnDisk::TileSize;
      uint32_t cellCountY = (mipH + udImageStreamingOnDisk::TileSize - 1) / udImageStreamingOnDisk::TileSize;
      for (uint32_t cellY = 0; cellY < cellCountY; ++cellY)
      {
        for (uint32_t cellX = 0; cellX < cellCountX; ++cellX)
        {
          //udDebugPrintf("Writing cell %d,%d at offset %llx\n", cellX, cellY, pOut - ((uint8_t *)pOnDisk));
          uint32_t cellW = std::min((uint32_t)udImageStreamingOnDisk::TileSize, mipW - (cellX * udImageStreamingOnDisk::TileSize));
          uint32_t cellH = std::min((uint32_t)udImageStreamingOnDisk::TileSize, mipH - (cellY * udImageStreamingOnDisk::TileSize));
          for (uint32_t y = 0; y < cellH; ++y)
          {
            pIn = p24BitData + ((cellY * udImageStreamingOnDisk::TileSize + y) * (mipW * 3)) + (cellX * udImageStreamingOnDisk::TileSize * 3);
            for (uint32_t x = 0; x < cellW; ++x)
            {
              *pOut++ = *pIn++;
              *pOut++ = *pIn++;
              *pOut++ = *pIn++;
            }
          }
        }
      }

      // Now generate the next mip down
      uint8_t *pO = p24BitData; // Overwrite the data as we go by processing left to right, top to bottom order
      for (uint32_t y = 0; y < mipH; y += 2)
      {
        uint8_t *pA = p24BitData + (y * mipW * 3);
        uint8_t *pB = pA + (mipW * 3); // Line immediately under pA's line

        if (y + 1 == mipH)
          pB = pA; // Just use the same line again

        for (uint32_t x = 0; x < mipW; x += 2)
        {
          if (x + 1 == mipW) // Last line on an "odd" sized image
          {
            // Loop for each of the components r,g,b
            for (uint32_t c = 0; c < 3; ++c)
              *pO++ = (unsigned(pA[c]) + unsigned(pB[c])) >> 1;
          }
          else
          {
            // Loop for each of the components r,g,b
            for (uint32_t c = 0; c < 3; ++c)
              *pO++ = (unsigned(pA[c + 0]) + unsigned(pA[c + 3]) + unsigned(pB[c + 0]) + unsigned(pB[c + 3])) >> 2;
          }

          // Source A and B pointers advance by 2 pixels (6 bytes)
          pA += 3 * 2;
          pB += 3 * 2;
        }
      }
      mipW = std::max(1U, mipW >> 1);
      mipH = std::max(1U, mipH >> 1);
    }

    *ppOnDisk = pOnDisk;
  }

  *pSaveSize = saveSize;
  *ppOnDisk = pOnDisk;
  pOnDisk = nullptr;
  result = udR_Success;

epilogue:
  udFree(p24BitData);
  udFree(pOnDisk);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_Load(udImageStreaming **ppImage, udFile *pFile, int64_t offset)
{
  udResult result;
  udImageStreaming *pImage = nullptr;

  UD_ERROR_NULL(ppImage, udR_InvalidParameter);
  UD_ERROR_CHECK(udImageStreaming_Reserve(&pImage, pFile, offset));
  UD_ERROR_CHECK(udImageStreaming_LoadCell(pImage, (uint32_t)-1));
  *ppImage = pImage;
  pImage = nullptr;
  result = udR_Success;

epilogue:
  if (pImage)
    udImageStreaming_Destroy(&pImage);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_Reserve(udImageStreaming **ppImage, udFile *pFile, int64_t offset)
{
  udResult result;
  udImageStreaming *pImage = nullptr;

  UD_ERROR_NULL(ppImage, udR_InvalidParameter);
  UD_ERROR_NULL(pFile, udR_InvalidParameter);

  pImage = udAllocType(udImageStreaming, 1, udAF_Zero);
  UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
  pImage->pFile = pFile;
  pImage->baseOffset = offset;
  pImage->pLock = udCreateMutex();

  *ppImage = pImage;
  pImage = nullptr;
  result = udR_Success;

epilogue:
  if (pImage)
    udImageStreaming_Destroy(&pImage);
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2020
uint32_t udImageStreaming_Sample(udImageStreaming *pImage, float u, float v, udImageSampleFlags flags, uint16_t mipLevel)
{
  udResult result;
  uint32_t x, y; // Coordinates on texture of u,v
  uint32_t texel = 0; // Default value returned if no data available
  udImageStreaming::Mip *pMip = nullptr;
  uint8_t *p;

  // First sample may trigger the initial load, but can't do error checking here so an error returns zero color
  if (pImage->fourcc == 0 && udImageStreaming_LoadCell(pImage, (uint32_t)-1) != udR_Success)
  {
    udDebugPrintf("Error loading texture header\n");
    return texel;
  }

  pMip = &pImage->mips[std::clamp((int)mipLevel, 0, pImage->mipCount - 1)];
  GetUVAsPixelIndex(u, v, flags, pMip->width, pMip->height, &x, &y);

  uint32_t cellX = x / udImageStreaming::TileSize;
  uint32_t cellY = y / udImageStreaming::TileSize;
  uint32_t cellIndex = cellY * pMip->gridW + cellX;
  uint32_t cellWidth = std::min((uint32_t)udImageStreaming::TileSize, pMip->width - (cellX * udImageStreaming::TileSize));
  if (!pMip->ppCellImage || !pMip->ppCellImage[cellIndex])
  {
    if (flags & udISF_NoStream)
      UD_ERROR_SET(udR_Success);
    else
      UD_ERROR_CHECK(udImageStreaming_LoadCell(pImage, PackCellIndex(cellX, cellY, mipLevel)));
  }
  x &= (udImageStreaming::TileSize - 1);
  y &= (udImageStreaming::TileSize - 1);
  p = pMip->ppCellImage[cellIndex] + (y * cellWidth + x) * 3;
  if (flags & udISF_ABGR)
    texel = p[0] | (p[1] << 8) | (p[2] << 16) | 0xff000000;
  else
    texel = p[2] | (p[1] << 8) | (p[0] << 16) | 0xff000000;

epilogue:
  return texel;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_LoadCell(udImageStreaming *pImage, uint32_t cellIndexData)
{
  udResult result;
  udMutex *pLocked = nullptr;
  uint8_t *pCellMem = nullptr;

  UD_ERROR_NULL(pImage, udR_InvalidParameter);

  if (pImage->fourcc == 0)
  {
    pLocked = udLockMutex(pImage->pLock);
    if (pImage->fourcc == 0)
    {
      UD_ERROR_CHECK(udFile_Read(pImage->pFile, pImage, sizeof(udImageStreamingOnDisk), pImage->baseOffset, udFSW_SeekSet));
      UD_ERROR_IF(pImage->fourcc != udImageStreamingOnDisk::Fourcc, udR_ObjectTypeMismatch);
      UD_ERROR_IF(pImage->mipCount > udImageStreamingOnDisk::MaxMipLevels, udR_CorruptData);

      for (uint16_t mip = 0; mip < pImage->mipCount; ++mip)
      {
        if (!mip)
        {
          pImage->mips[mip].offset = pImage->baseOffset + pImage->offsetToMip0;
          pImage->mips[mip].width = pImage->width;
          pImage->mips[mip].height = pImage->height;
        }
        else
        {
          pImage->mips[mip].offset = pImage->mips[mip - 1].offset + (pImage->mips[mip - 1].width * pImage->mips[mip - 1].height * 3);
          pImage->mips[mip].width = std::max(1U, pImage->mips[mip - 1].width >> 1);
          pImage->mips[mip].height = std::max(1U, pImage->mips[mip - 1].height >> 1);
        }

        pImage->mips[mip].gridW = (uint16_t)(pImage->mips[mip].width + udImageStreamingOnDisk::TileSize - 1) / udImageStreamingOnDisk::TileSize;
        pImage->mips[mip].gridH = (uint16_t)(pImage->mips[mip].height + udImageStreamingOnDisk::TileSize - 1) / udImageStreamingOnDisk::TileSize;
      }
    }
  }

  if (cellIndexData != (uint32_t)-1)
  {
    uint32_t mipLevel = (cellIndexData >> 0) & 0xff;
    uint32_t cellX = (cellIndexData >> 8) & 0xff;
    uint32_t cellY = (cellIndexData >> 16) & 0xff;

    UD_ERROR_IF(mipLevel >= pImage->mipCount, udR_InvalidParameter);
    udImageStreaming::Mip *pMip = &pImage->mips[std::clamp((int)mipLevel, 0, pImage->mipCount - 1)];
    UD_ERROR_IF(cellX >= pMip->gridW, udR_InvalidParameter);
    UD_ERROR_IF(cellY >= pMip->gridH, udR_InvalidParameter);

    uint32_t cellIndex = cellY * pMip->gridW + cellX;
    uint32_t cellWidth = std::min((uint32_t)udImageStreaming::TileSize, pMip->width - (cellX * udImageStreaming::TileSize));
    if (!pMip->ppCellImage || !pMip->ppCellImage[cellIndex])
    {
      if (!pLocked)
        pLocked = udLockMutex(pImage->pLock);
      if (!pMip->ppCellImage || !pMip->ppCellImage[cellIndex]) // Second check after the lock
      {
        if (!pMip->ppCellImage)
        {
          pMip->ppCellImage = udAllocType(uint8_t *, pMip->gridW * pMip->gridH, udAF_Zero);
          UD_ERROR_NULL(pMip->ppCellImage, udR_MemoryAllocationFailure);
        }
        uint32_t cellHeight = std::min((uint32_t)udImageStreaming::TileSize, pMip->height - (cellY * udImageStreaming::TileSize));
        uint32_t cellSizeBytes = cellWidth * cellHeight * 3;
        uint32_t cellOffset = (cellY * udImageStreaming::TileSize * pMip->width * 3) + (cellX * udImageStreaming::TileSize * udImageStreaming::TileSize * 3);
        // Read into locally allocated block
        pCellMem = udAllocType(uint8_t, cellSizeBytes, udAF_None);
        UD_ERROR_NULL(pCellMem, udR_MemoryAllocationFailure);
        UD_ERROR_CHECK(udFile_Read(pImage->pFile, pCellMem, cellSizeBytes, pMip->offset + cellOffset, udFSW_SeekSet));
        // Assign the pointer after reading to ensure another thread doesn't access the memory before the read is complete
        udInterlockedExchangePointer(&pMip->ppCellImage[cellIndex], pCellMem);
        pCellMem = nullptr;
      }
      udReleaseMutex(pLocked);
      pLocked = nullptr;
    }
  }

  result = udR_Success;

epilogue:
  udFree(pCellMem);
  if (pLocked)
    udReleaseMutex(pLocked);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2023
udResult udImageStreaming_GetSubsectionCode(const udImageStreaming *pImage, uint64_t *pSubsectionCode, float minU, float minV, float maxU, float maxV, uint16_t mipLevel, udImageSampleFlags flags, uint64_t existingSubsection, float uvOffsetScale[4])
{
  udResult result;
  uint32_t minCellX, minCellY, maxCellX, maxCellY, x, y;
  const udImageStreaming::Mip *pMip = nullptr;

  UD_ERROR_NULL(pImage, udR_InvalidParameter);
  UD_ERROR_NULL(pSubsectionCode, udR_InvalidParameter);

  // Force load the header
  if (pImage->fourcc == 0)
    UD_ERROR_CHECK(udImageStreaming_LoadCell(const_cast<udImageStreaming *>(pImage), (uint32_t)-1));

  if (existingSubsection)
  {
    UnpackCellIndex((uint32_t)(existingSubsection >> 0), &minCellX, &minCellY);
    UnpackCellIndex((uint32_t)(existingSubsection >> 32), &maxCellX, &maxCellY);
    UD_ERROR_IF(minCellX > maxCellX, udR_InvalidConfiguration);
    UD_ERROR_IF(minCellY > maxCellY, udR_InvalidConfiguration);
  }
  else
  {
    minCellX = minCellY = (uint32_t)-1;
    maxCellX = maxCellY = 0;
  }
  pMip = &pImage->mips[std::clamp((int)mipLevel, 0, pImage->mipCount - 1)];

  GetUVAsPixelIndex(minU, minV, flags, pMip->width, pMip->height, &x, &y);
  minCellX = std::min(minCellX, x);
  maxCellX = std::max(maxCellX, x);
  minCellY = std::min(minCellY, y);
  maxCellY = std::max(maxCellY, y);

  GetUVAsPixelIndex(maxU, maxV, flags, pMip->width, pMip->height, &x, &y);
  // The max UV could be can be 1.0 which will map back to pixel offset 0, but so could a UV of 0.0, so decrement accordingly
  if (maxU != 0.f && x == 0)
    x = pMip->width - 1;
  if (maxV != 0.f && y == 0)
    y = pMip->height - 1;
  minCellX = std::min(minCellX, x);
  maxCellX = std::max(maxCellX, x);
  minCellY = std::min(minCellY, y);
  maxCellY = std::max(maxCellY, y);

  minCellX /= udImageStreaming::TileSize;
  maxCellX /= udImageStreaming::TileSize;
  minCellY /= udImageStreaming::TileSize;
  maxCellY /= udImageStreaming::TileSize;
  UD_ERROR_IF(minCellX >= pImage->mips[mipLevel].gridW, udR_InternalError);
  UD_ERROR_IF(maxCellX >= pImage->mips[mipLevel].gridW, udR_InternalError);
  UD_ERROR_IF(minCellY >= pImage->mips[mipLevel].gridH, udR_InternalError);
  UD_ERROR_IF(maxCellY >= pImage->mips[mipLevel].gridH, udR_InternalError);

  *pSubsectionCode = PackCellIndex(minCellX, minCellY, mipLevel) | (uint64_t(PackCellIndex(maxCellX, maxCellY, mipLevel)) << 32);
  if (uvOffsetScale)
  {
    // Scale factor for UV's is always just how many tiles are in the subsection divided by the max tiles in that axis
    uvOffsetScale[2] = pImage->mips[mipLevel].gridW / (maxCellX - minCellX + 1.f);
    uvOffsetScale[3] = pImage->mips[mipLevel].gridH / (maxCellY - minCellY + 1.f);

    // Offset values are applied before the scale, so they are just the UV coordinate for the minimum tile
    // Slightly complicated by the fact that GL uses upside down texturing in the V axis
    uvOffsetScale[0] = minCellX / (float)pImage->mips[mipLevel].gridW;
    if (flags & udISF_TopLeft)
      uvOffsetScale[1] = minCellY / (float)pImage->mips[mipLevel].gridH;
    else
      uvOffsetScale[1] = (pImage->mips[mipLevel].gridH - (maxCellY + 1)) / (float)pImage->mips[mipLevel].gridH;
  }
  //udDebugPrintf("Total cells: %d/%d\n", (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1), pImage->mips[mipLevel].gridW * pImage->mips[mipLevel].gridH);
  result = udR_Success;

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_GetImage24(udImageStreaming *pImage, uint8_t **ppImage24, udImageSampleFlags flags, uint32_t *pWidth, uint32_t *pHeight, uint64_t subsectionCode)
{
  udResult result;
  uint8_t *pRGB = nullptr;
  const udImageStreaming::Mip *pMip = nullptr;
  uint32_t mipLevel;
  uint32_t minX, minY, maxX, maxY; // min/max CELL indices (not pixels, and mip dependent)
  uint32_t baseX, baseY, outW, outH; // The output rectangle in pixel coordinates

  UD_ERROR_NULL(ppImage24, udR_InvalidParameter);
  UD_ERROR_NULL(pImage, udR_InvalidParameter);

  // Ensure the header has been loaded by doing an initial dummy sample
  udImageStreaming_Sample(pImage, 0.f, 0.f);
  if (subsectionCode)
  {
    uint32_t verifyMipLevel;
    UnpackCellIndex((uint32_t)(subsectionCode >> 0), &minX, &minY, &mipLevel);
    UnpackCellIndex((uint32_t)(subsectionCode >> 32), &maxX, &maxY, &verifyMipLevel);
    UD_ERROR_IF(mipLevel != verifyMipLevel, udR_InvalidConfiguration);
    pMip = &pImage->mips[std::clamp((int)mipLevel, 0, pImage->mipCount - 1)];
  }
  else
  {
    mipLevel = 0;
    pMip = &pImage->mips[0];
    minX = minY = 0;
    maxX = pMip->gridW - 1;
    maxY = pMip->gridH - 1;
  }
  baseX = minX * udImageStreaming::TileSize;
  baseY = minY * udImageStreaming::TileSize;
  outW = (maxX - minX + 1) * udImageStreaming::TileSize;
  outH = (maxY - minY + 1) * udImageStreaming::TileSize;

  // Make a 24-bit copy to avoid PNG saving 32-bit when all alpha values are 0xff (currently anyway)
  // This isn't optimal, but for simplicity we just use the Sample function to handle streaming
  pRGB = (uint8_t *)udAlloc(outW * outH * 3);
  UD_ERROR_NULL(pRGB, udR_MemoryAllocationFailure);
  for (uint32_t y = 0; y < outH; ++y)
  {
    for (uint32_t x = 0; x < outW; ++x)
    {
      uint32_t c = udImageStreaming_Sample(pImage, (baseX + x) / (float)pMip->width, (baseY + y) / (float)pMip->height, flags, (uint16_t)mipLevel);
      memcpy(&pRGB[(y * outW + x) * 3], &c, 3);
    }
  }

  if constexpr (0)
  {
    stbiWriteContext writeContext;
    uint32_t bw;
    writeContext.pResult = &result;
    writeContext.pBytesWritten = &bw;
    udFile_Open(&writeContext.pFile, "c:\\temp\\img24.png", udFOF_Create | udFOF_Write);
    stbiWriteWrapper(&writeContext, outW, outH, pRGB, udIST_PNG);
    udFile_Close(&writeContext.pFile);
  }


  *ppImage24 = pRGB;
  pRGB = nullptr;
  if (pWidth)
    *pWidth = outW;
  if (pHeight)
    *pHeight = outH;
  result = udR_Success;

epilogue:
  udFree(pRGB);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
udResult udImageStreaming_SaveAs(udImageStreaming *pImage, const char *pFilename, uint32_t *pSaveSize, udImageSaveType saveType)
{
  udResult result;
  uint8_t *pRGB = nullptr; // Need to make a 24-bit copy for stbi
  stbiWriteContext writeContext;

  UD_ERROR_NULL(pImage, udR_InvalidParameter);
  UD_ERROR_NULL(pFilename, udR_InvalidParameter);
  UD_ERROR_IF(saveType >= udIST_Max, udR_InvalidParameter);

  udImageStreaming_GetImage24(pImage, &pRGB, udISF_ABGR | udISF_TopLeft);

  writeContext.pResult = &result;
  writeContext.pBytesWritten = pSaveSize;
  UD_ERROR_CHECK(udFile_Open(&writeContext.pFile, pFilename, udFOF_Create | udFOF_Write));
  stbiWriteWrapper(&writeContext, pImage->width, pImage->height, pRGB, saveType);
  udFile_Close(&writeContext.pFile);

epilogue:
  udFree(pRGB);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2020
void udImageStreaming_Destroy(udImageStreaming **ppImage)
{
  if (ppImage && *ppImage)
  {
    udImageStreaming *pImage = *ppImage;
    *ppImage = nullptr;
    for (uint16_t i = 0; i < pImage->mipCount; ++i)
    {
      if (pImage->mips[i].ppCellImage)
      {
        for (uint16_t j = 0; j < (pImage->mips[i].gridW * pImage->mips[i].gridH); ++j)
          udFree(const_cast<uint8_t*&>(pImage->mips[i].ppCellImage[j]));
        udFree(pImage->mips[i].ppCellImage);
      }
    }
    udDestroyMutex(&pImage->pLock);
    pImage->pFile = nullptr;
    udFree(pImage);
  }
}
