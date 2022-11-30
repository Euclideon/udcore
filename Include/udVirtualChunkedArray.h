#ifndef UDVIRTUALCHUNKEDARRAY_H
#define UDVIRTUALCHUNKEDARRAY_H

#include "udFile.h"
#include "udThread.h"
#include "udStringUtil.h"
#define VIRTUALCHUNKEDARRAY_DEBUGGING 0

// Minimal implementation currently, will only work for simple typed non-sparse arrays with elements read-only after being added
template <typename T>
class udVirtualChunkedArray
{
public:
  udResult Init(size_t chunkElementCount, const char *pTempFilename, uint32_t maxChunksInMem = 8);
  udResult Deinit();

  // Methods matching udChunkedArray functionality
  udResult PushBack(T **ppElement, bool zeroMemory = true);
  bool PopBack(T *pData = nullptr);
  const T &operator[](size_t index);
  inline udResult PushBack(const T &v) { udResult result;  T *pEl; result = PushBack(&pEl, false); if (result == udR_Success) *pEl = v; return result; }

  // Test if an element will require loading from the temp file
  bool IsElementInMemory(size_t index) const { return (index < array.length &&array.ppChunks[(index + array.inset) >> array.chunkElementCountShift] != nullptr); } // For unit testing
  size_t length;                // Mirror of internal array length

private:
  udChunkedArray<T> array;
  struct ChunkInfo
  {
    int64_t fileOffset;         // Offset into temp file for each chunk virtualised
    size_t lastReference;       // Every time the chunk is accessed this is updated
  } *pChunkInfo;
  size_t chunkInfoLength;       // Length of the info array (used to keep in sync with array.chunkCount)
  size_t currentReference;      // Rolling counter to determine LRU
  const char *pTempFilename;
  udFile *pTempFile;
  udMutex *pLock;
  uint32_t chunksInMem;
  uint32_t maxChunksInMem;

  udResult SaveLRUChunk();
};



// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
udResult udVirtualChunkedArray<T>::Init(size_t chunkElementCount, const char *_pTempFilename, uint32_t _maxChunksInMem)
{
  udResult result;

  pLock = udCreateMutex();
  UD_ERROR_CHECK(array.Init(chunkElementCount));
  pChunkInfo = nullptr;
  chunkInfoLength = 0;
  chunksInMem = 0;
  maxChunksInMem = _maxChunksInMem;
  currentReference = 1;
  pTempFilename = udStrdup(_pTempFilename);
  pTempFile = nullptr;
  if (pTempFilename)
    UD_ERROR_CHECK(udFile_Open(&pTempFile, pTempFilename, udFOF_Create | udFOF_Read | udFOF_Write));
  result = udR_Success;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
udResult udVirtualChunkedArray<T>::Deinit()
{
  udResult result;

  udDestroyMutex(&pLock);
  udFree(pChunkInfo);
  UD_ERROR_CHECK(array.Deinit());
  if (pTempFile)
  {
    udFile_Close(&pTempFile);
    udFileDelete(pTempFilename);
  }
  udFree(pTempFilename);
  result = udR_Success;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
udResult udVirtualChunkedArray<T>::PushBack(T **ppElement, bool zeroMemory)
{
  udResult result;
  udScopeLock l(pLock);

  UD_ERROR_CHECK(array.PushBack(ppElement, zeroMemory));
  length = array.length;
  if (pTempFile)
  {
    if (chunkInfoLength < array.chunkCount)
    {
      pChunkInfo = (ChunkInfo*)udRealloc(pChunkInfo, array.chunkCount * sizeof(*pChunkInfo));
      while (chunkInfoLength < array.chunkCount)
      {
        pChunkInfo[chunkInfoLength].fileOffset = -1L;
        pChunkInfo[chunkInfoLength].lastReference = 0;
        ++chunkInfoLength;
        ++chunksInMem;
      }
    }
    // Reference the chunk, but only if it isn't already the most recent reference
    size_t chunkIndex = (length - 1 + array.inset) >> array.chunkElementCountShift;
    if (pChunkInfo[chunkIndex].lastReference != currentReference)
      pChunkInfo[chunkIndex].lastReference = ++currentReference;
    if (chunksInMem > maxChunksInMem)
      UD_ERROR_CHECK(SaveLRUChunk());
  }

  result = udR_Success;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
bool udVirtualChunkedArray<T>::PopBack(T *pElement)
{
  bool itemFound = array.PopBack(pElement);
  length = array.length;
  if (pTempFile &&chunkInfoLength != array.chunkCount)
  {
    --chunksInMem;
    chunkInfoLength = array.chunkCount;
  }
  return itemFound;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
const T &udVirtualChunkedArray<T>::operator[](size_t index)
{
  udScopeLock l(pLock);

  if (pTempFile)
  {
    size_t chunkIndex = (index + array.inset) >> array.chunkElementCountShift;

    // Reference the chunk, but only if it isn't already the most recent reference
    if (pChunkInfo[chunkIndex].lastReference != currentReference)
      pChunkInfo[chunkIndex].lastReference = ++currentReference;

    if (!array.ppChunks[chunkIndex])
    {
      // Reload chunk from file
      size_t chunkLength = array.chunkElementCount * sizeof(T);
#if VIRTUALCHUNKEDARRAY_DEBUGGING
      udDebugPrintf("Loading chunk %d from position %08x\n", chunkIndex, pChunkInfo[chunkIndex].fileOffset);
#endif
      array.ppChunks[chunkIndex] = (T*)udAlloc(chunkLength);
      UDRELASSERT(array.ppChunks[chunkIndex] != nullptr, "Memory Allocation Failure");
      udResult result = udFile_Read(pTempFile, array.ppChunks[chunkIndex], chunkLength, pChunkInfo[chunkIndex].fileOffset, udFSW_SeekSet);
      UDRELASSERT(result == udR_Success, "Error reading back chunk data\n");
      udUnused(result);
      ++chunksInMem;
      pChunkInfo[chunkIndex].lastReference = ++currentReference;

      // Now save/free the next most LRU page to avoid memory overflowing
      if (chunksInMem > maxChunksInMem)
        SaveLRUChunk();
    }
  }
  return array[index];
}

// ****************************************************************************
// Author: Dave Pevreal, November 2022
template <typename T>
udResult udVirtualChunkedArray<T>::SaveLRUChunk()
{
  udResult result;
  size_t lruIndex = chunkInfoLength;
  size_t lruDelta = 0;

  for (size_t chunkIndex = 0; chunkIndex < chunkInfoLength; ++chunkIndex)
  {
    if (array.ppChunks[chunkIndex])
    {
      size_t delta = currentReference - pChunkInfo[chunkIndex].lastReference;
      // Don't consider a block that's been referenced too recently to possibly be removed
      if (delta >= maxChunksInMem && (lruIndex == chunkInfoLength || lruDelta < delta))
      {
        lruIndex = chunkIndex;
        lruDelta = delta;
      }
    }
  }

  // If an LRU chunk was found...
  if (lruIndex < chunkInfoLength)
  {
    // And it's not already written to disk...
    if (pChunkInfo[lruIndex].fileOffset < 0)
    {
      // Save the contents of the chunk to the end of the file, recording it's offset
      size_t chunkLength = array.chunkElementCount * sizeof(T);
      int64_t newEnd;
      UD_ERROR_CHECK(udFile_Write(pTempFile, array.ppChunks[lruIndex], chunkLength, 0, udFSW_SeekEnd, nullptr, &newEnd));
      pChunkInfo[lruIndex].fileOffset = newEnd - chunkLength;
#if VIRTUALCHUNKEDARRAY_DEBUGGING
      udDebugPrintf("Saved chunk %d at position %08x\n", lruIndex, pChunkInfo[lruIndex].fileOffset);
#endif
    }
    // Free the chunk knowing the data can be recovered from disk
#if VIRTUALCHUNKEDARRAY_DEBUGGING
    udDebugPrintf("Released chunk %d\n", lruIndex);
#endif
    udFree(array.ppChunks[lruIndex]);
    --chunksInMem;
  }
#if VIRTUALCHUNKEDARRAY_DEBUGGING
  else
  {
    udDebugPrintf("No chunks found able to be unloaded\n");
  }
#endif
  result = udR_Success;

epilogue:
  return result;
}


#endif // UDVIRTUALCHUNKEDARRAY_H
