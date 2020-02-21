#ifndef UDIMAGE_H
#define UDIMAGE_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, February 2019
//
// Module for loading and sampling images, wrapping stb_image internally
//

#include "udPlatform.h"
struct udFile;

struct udImage
{
  enum ImageFlags { IF_Streaming = 1 };
  uint32_t width, height;
  uint16_t sourceChannels;
  uint16_t mipCount;
  uint16_t flags;
  uint32_t *apImageData[1]; // Array of pointers, length mipCount, followed by OnDisk structure if streaming

  void GetMipLevelDimensions(uint16_t mipLevel, uint32_t *pWidth, uint32_t *pHeight) const
  {
    if (pWidth)
      *pWidth = width;
    if (pHeight)
      *pHeight = height;
    for (uint16_t i = 0; i < mipLevel; ++i)
    {
      if (pWidth && *pWidth > 1)
        *pWidth >>= 1;
      if (pHeight && *pHeight > 1)
        *pHeight >>= 1;
    }
  }
};

enum udImageSampleFlags
{
  udISF_None = 0,
  udISF_Filter = 1,
  udISF_Clamp = 2,
  udISG_ABGR = 4,       // By default, colors are returned in ARGB (blue least significant byte), this option returns in ABGR (red least significant byte)
  udISG_TopLeft = 8,    // Sample with top left point as 0,0 rather than openGL's curious bottom left 0,0 scheme
};
inline udImageSampleFlags operator|(udImageSampleFlags a, udImageSampleFlags b) { return (udImageSampleFlags)(int(a) | int(b)); }

// Load an image from a filename
udResult udImage_Load(udImage **ppImage, const char *pFilename, bool generateMips = false);

// Load an image from a file already in memory
udResult udImage_LoadFromMemory(udImage **ppImage, const void *pMemory, size_t length, bool generateMips = false);

// Generate mips and save a streamable image to existing file handle and offset (pass null for pFile to just generate size required)
udResult udImage_SaveStreamable(const udImage *pImage, udFile *pFile, int64_t offset, uint32_t *pSaveSize);

// Load a streamable image from an existing file handle and offset (normal case to have all images in a single file)
udResult udImage_LoadStreamable(udImage **ppImage, udFile *pFile, int64_t offset);

// Load a streamable image from an existing file handle and offset (normal case to have all images in a single file)
udResult udImage_LoadMip(udImage *pImage, uint16_t mipLevel, udFile *pFile, int64_t offset);

// Sample a pixel (optionally with bilinear filtering) with openGL-style UV coordinates (0,0 bottom left)
uint32_t udImage_Sample(udImage *pImage, float u, float v, udImageSampleFlags flags = udISF_None, uint16_t mipLevel = 0);

// Destroy the image and free resources
void udImage_Destroy(udImage **ppImage);

#endif // UDIMAGE_H
