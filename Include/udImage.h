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

struct udImage
{
  uint32_t width, height, sourceChannels;
  uint32_t allocationOwner; // Zero for udAlloc, 0x5781 if allocated by stbi library
  uint32_t *pImageData;
};

enum udImageSampleFlags
{
  udISF_None = 0,
  udISF_Filter = 1,
  udISF_Clamp = 2,
  udISG_ABGR = 4, // By default, colors are returned in ARGB (blue least significant byte), this option returns in ABGR (red least significant byte)
};
inline udImageSampleFlags operator|(udImageSampleFlags a, udImageSampleFlags b) { return (udImageSampleFlags)(int(a) | int(b)); }

// Load an image from a filename
udResult udImage_Load(udImage **ppImage, const char *pFilename);

// Load an image from a file already in memory
udResult udImage_LoadFromMemory(udImage **ppImage, const void *pMemory, size_t length);

// Sample a pixel (optionally with bilinear filtering) with openGL-style UV coordinates (0,0 bottom left)
uint32_t udImage_Sample(udImage *pImage, float u, float v, udImageSampleFlags flags = udISF_None);

// Destroy the image and free resources
void udImage_Destroy(udImage **ppImage);

#endif // UDIMAGE_H
