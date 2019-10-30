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
struct udMutex;

struct udImage
{
  uint32_t width, height;
  uint16_t sourceChannels;
  uint16_t reserved;
  uint32_t *pImageData;
};

// A streaming tiled and mip mapped texture format
struct udImageStreamingOnDisk
{
  enum
  {
    Fourcc = MAKE_FOURCC('U', 'D', 'T', '0'),  // Fourcc also a version number
    TileSize = 64,
    MaxMipLevels = 24,
  };
  uint32_t fourcc;
  uint32_t width, height;
  uint16_t mipCount;
  char name[64]; // some descriptive text (such as a filename) to help identify
  uint16_t offsetToMip0; // To allow future extension
};

struct udImageStreaming : public udImageStreamingOnDisk
{
  udMutex *pLock;
  udFile *pFile;
  int64_t baseOffset;
  struct Mip
  {
    int64_t offset;             // Offset from beginning of on-disk image of first tile
    uint32_t width, height;     // Dimensions for this mip
    uint16_t gridW, gridH;      // Dimensions of cell grid
    uint8_t * volatile * ppCellImage;      // Pointers to each 64x64 tile
  } mips[MaxMipLevels];
};

enum udImageSampleFlags
{
  udISF_None = 0,
  udISF_Filter = 1,
  udISF_Clamp = 2,
  udISF_ABGR = 4,       // By default, colors are returned in ARGB (blue least significant byte), this option returns in ABGR (red least significant byte)
  udISF_TopLeft = 8,    // Sample with top left point as 0,0 rather than openGL's curious bottom left 0,0 scheme
  udISF_NoStream = 16,  // If a cell isn't loaded, don't load it, but rather return the index data in lower 24 bits, alpha is zero
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

// Generate mips and save a streamable image to existing file handle and offset (pass null for ppOnDisk to just generate size required)
udResult udImageStreaming_Save(const udImage *pImage, udImageStreamingOnDisk **ppOnDisk, uint32_t *pSaveSize, const char *pNameDesc = nullptr);

// Load a streamable image from an existing file handle and offset
udResult udImageStreaming_Load(udImageStreaming **ppImage, udFile *pFile, int64_t offset);

// Sample a pixel (optionally with bilinear filtering) with openGL-style UV coordinates (0,0 bottom left)
uint32_t udImageStreaming_Sample(udImageStreaming *pImage, float u, float v, udImageSampleFlags flags = udISF_None, uint16_t mipLevel = 0);

// Load a cell from index data returned by udImageStreaming_Sample using the udISF_NoStream flag
udResult udImageStreaming_LoadCell(udImageStreaming *pImage, uint32_t cellIndexData);

// Destroy the image and free resources
void udImageStreaming_Destroy(udImageStreaming **ppImage);

// Free least recently used tiles to meet a usage goal
void udImageStreaming_FreeTiles(uint32_t memoryUsageGoal);

#endif // UDIMAGE_H
