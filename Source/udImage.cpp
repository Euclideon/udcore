#include "udImage.h"
#include "udFile.h"
#include "udMath.h"
#include "../3rdParty/stb/stb_image.h"

#define STBI_ALLOCATION_OWNER_ID 0x5781

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_Load(udImage **ppImage, const char *pFilename)
{
  udResult result;
  void *pMem = nullptr;
  int64_t fileLen;

  UD_ERROR_CHECK(udFile_Load(pFilename, &pMem, &fileLen));
  UD_ERROR_CHECK(udImage_LoadFromMemory(ppImage, pMem, (size_t)fileLen));

epilogue:
  udFree(pMem);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
udResult udImage_LoadFromMemory(udImage **ppImage, const void *pMemory, size_t length)
{
  udResult result;
  udImage *pImage = nullptr;
  int w, h, sc; // Some plain integers for call to 3rd party API

  UD_ERROR_NULL(ppImage, udR_InvalidParameter_);
  UD_ERROR_NULL(pMemory, udR_InvalidParameter_);
  UD_ERROR_IF(length == 0, udR_InvalidParameter_);

  pImage = udAllocType(udImage, 1, udAF_Zero);
  UD_ERROR_NULL(pImage, udR_MemoryAllocationFailure);
  pImage->pImageData = (uint32_t*)stbi_load_from_memory((const stbi_uc*)pMemory, (int)length, &w, &h, &sc, 4);
  UD_ERROR_NULL(pImage->pImageData, udR_ImageLoadFailure);
  pImage->width = (uint32_t)w;
  pImage->height = (uint32_t)h;
  pImage->sourceChannels = (uint32_t)sc;

  pImage->allocationOwner = STBI_ALLOCATION_OWNER_ID;

  *ppImage = pImage;
  pImage = nullptr;
  result = udR_Success;

epilogue:
  udImage_Destroy(&pImage);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, February 2019
uint32_t udImage_Sample(udImage *pImage, float u, float v, udImageSampleFlags flags)
{
  if (flags & udISF_Clamp)
  {
    u = udClamp(u, 0.f, 1.f);
    v = udClamp(v, 0.f, 1.f);
  }

  u =  u * pImage->width;
  v = -v * pImage->height;

  if (u < 0.0f || u > pImage->width)
    u = u - pImage->width * udFloor((u / pImage->width));
  if (v < 0.0f || v > pImage->height)
    v = v - pImage->height * udFloor((v / pImage->height));

  int x = (int)u;
  int y = (int)v;
  if (flags & udISF_Filter)
  {
    int u1 = (int)(u * 256) & 0xff; // Get most most significant bits of PRECISION
    int v1 = (int)(v * 256) & 0xff; // Get most most significant bits of PRECISION
    int u0 = 256 - u1;
    int v0 = 256 - v1;

    int x0 = udClamp(x + 0, 0, (int)pImage->width - 1);
    int x1 = udClamp(x + 1, 0, (int)pImage->width - 1);
    int y0 = udClamp(y + 0, 0, (int)pImage->height - 1);
    int y1 = udClamp(y + 1, 0, (int)pImage->height - 1);

    int a = u0 * v0;
    int b = u1 * v0;
    int c = u0 * v1;
    int d = u1 * v1;

    uint32_t c0 = (pImage->pImageData[x0 + y0 * pImage->width]);
    uint32_t c1 = (pImage->pImageData[x1 + y0 * pImage->width]);
    uint32_t c2 = (pImage->pImageData[x0 + y1 * pImage->width]);
    uint32_t c3 = (pImage->pImageData[x1 + y1 * pImage->width]);

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
    uint32_t c = (pImage->pImageData[x + y * pImage->width]); // STBI returns colors as ABGR

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
    if (pImage->pImageData && pImage->allocationOwner == STBI_ALLOCATION_OWNER_ID)
    {
      stbi_image_free(pImage->pImageData);
      pImage->pImageData = nullptr;
    }
    udFree(pImage->pImageData);
    udFree(pImage);
  }
}
