#ifndef UDCHUNKEDARRAY_H
#define UDCHUNKEDARRAY_H
#include "udPlatform.h"
#include "udResult.h"

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
struct udChunkedArray
{
  udResult Init();
  udResult Deinit();
  udResult Clear();

  T &operator[](size_t index);
  const T &operator[](size_t index) const;
  T *GetElement(size_t index);
  const T *GetElement(size_t index) const;
  void SetElement(size_t index, const T &data);

  udResult PushBack(const T &v);
  udResult PushBack(T **ppElement);              // NOTE: Does not zero memory, can fail if memory allocation fails
  T *PushBack();                                 // DEPRECATED: Please use PushBack(const T&) or PushBack(T **)

  udResult PushFront(const T &v);
  udResult PushFront(T **ppElement);             // NOTE: Does not zero memory, can fail if memory allocation fails
  T *PushFront();                                // DEPRECATED: Please use PushFront(const T&) or PushFront(T **)

  udResult Insert(size_t index, const T *pData = nullptr);  // Insert the element at index, pushing and moving all elements after to make space.

  bool PopBack(T *pData = nullptr);              // Returns false if no element to pop
  bool PopFront(T *pData = nullptr);             // Returns false if no element to pop
  void RemoveAt(size_t index);                   // Remove the element at index, moving all elements after to fill the gap.
  void RemoveSwapLast(size_t index);             // Remove the element at index, swapping with the last element to ensure array is contiguous

  udResult GrowBack(size_t numberOfNewElements); // Push back a number of new elements, zeroing the memory
  udResult ReserveBack(size_t newCapacity);      // Reserve memory for a given number of elements without changing 'length'  NOTE: Does not reduce in size
  udResult AddChunks(size_t numberOfNewChunks);  // Add a given number of chunks capacity without changing 'length'


  size_t ChunkElementCount()                     { return chunkElementCount; }
  size_t ElementSize()                           { return sizeof(T); }

  template <typename _T, size_t _chunkElementCount>
  struct chunk
  {
    _T data[_chunkElementCount];
  };

  typedef chunk<T, chunkElementCount> chunk_t;
  enum { ptrArrayInc = 32};

  chunk_t **ppChunks;
  size_t ptrArraySize;
  size_t chunkCount;

  size_t length;
  size_t inset;
};

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T,chunkElementCount>::Init()
{
  UDCOMPILEASSERT(chunkElementCount >= 32, _Chunk_Count_Must_Be_At_Least_32);
  udResult result = udR_Success;
  size_t c = 0;

  chunkCount = 1;
  length = 0;
  inset = 0;

  if (chunkCount > ptrArrayInc)
    ptrArraySize = ((chunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
  else
    ptrArraySize = ptrArrayInc;

  ppChunks = udAllocType(chunk_t*, ptrArraySize, udAF_Zero);
  if (!ppChunks)
  {
    result = udR_MemoryAllocationFailure;
    goto epilogue;
  }

  for (; c < chunkCount; ++c)
  {
    ppChunks[c] = udAllocType(chunk_t, 1, udAF_None);
    if (!ppChunks[c])
    {
      result = udR_MemoryAllocationFailure;
      goto epilogue;
    }
  }

  return result;
epilogue:

  for (size_t i = 0; i < c; ++i)
    udFree(ppChunks[i]);

  udFree(ppChunks);

  chunkCount = 0;

  return result;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T,chunkElementCount>::Deinit()
{
  for (size_t c = 0; c < chunkCount; ++c)
    udFree(ppChunks[c]);

  udFree(ppChunks);

  chunkCount = 0;
  length = 0;
  inset = 0;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, June 2015
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::Clear()
{
  length = 0;
  inset = 0;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T,chunkElementCount>::AddChunks(size_t numberOfNewChunks)
{
  size_t newChunkCount = chunkCount + numberOfNewChunks;

  if (newChunkCount > ptrArraySize)
  {
    size_t newPtrArraySize = ((newChunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
    chunk_t **newppChunks = udAllocType(chunk_t*, newPtrArraySize, udAF_Zero);
    if (!newppChunks)
      return udR_MemoryAllocationFailure;

    memcpy(newppChunks, ppChunks, ptrArraySize * sizeof(chunk_t*));
    udFree(ppChunks);

    ppChunks = newppChunks;
    ptrArraySize = newPtrArraySize;
  }

  for (size_t c = chunkCount; c < newChunkCount; ++c)
  {
    ppChunks[c] = udAllocType(chunk_t, 1, udAF_None);
    if (!ppChunks[c])
    {
      chunkCount = c;
      return udR_MemoryAllocationFailure;
    }
  }

  chunkCount = newChunkCount;
  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T,chunkElementCount>::GrowBack(size_t numberOfNewElements)
{
  if (numberOfNewElements == 0)
    return udR_InvalidParameter_;

  size_t oldLength = inset + length;
  size_t newLength = oldLength + numberOfNewElements;
  size_t prevUsedChunkCount = (oldLength + chunkElementCount - 1) / chunkElementCount;

  udResult res = ReserveBack(length + numberOfNewElements);
  if (res != udR_Success)
    return res;

  // Zero new elements
  size_t newUsedChunkCount = (newLength + chunkElementCount - 1) / chunkElementCount;
  size_t usedChunkDelta = newUsedChunkCount - prevUsedChunkCount;
  size_t head = oldLength % chunkElementCount;

  if (usedChunkDelta)
  {
    if (head)
      memset(&ppChunks[prevUsedChunkCount - 1]->data[head], 0, (chunkElementCount - head) * sizeof(T));

    size_t tail = newLength % chunkElementCount;

    for (size_t chunkIndex = prevUsedChunkCount; chunkIndex < (newUsedChunkCount - 1 + !tail); ++chunkIndex)
      memset(ppChunks[chunkIndex]->data, 0, sizeof(chunk_t));

    if (tail)
      memset(&ppChunks[newUsedChunkCount - 1]->data[0], 0, tail * sizeof(T));
  }
  else
  {
    memset(&ppChunks[prevUsedChunkCount - 1]->data[head], 0, numberOfNewElements * sizeof(T));
  }

  length += numberOfNewElements;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::ReserveBack(size_t newCapacity)
{
  udResult res = udR_Success;
  size_t oldCapacity = chunkElementCount * chunkCount - inset;
  if (newCapacity > oldCapacity)
  {
    size_t newChunksCount = (newCapacity - oldCapacity + chunkElementCount - 1) / chunkElementCount;
    res = AddChunks(newChunksCount);
  }
  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline T &udChunkedArray<T, chunkElementCount>::operator[](size_t index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return ppChunks[chunkIndex]->data[index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, September 2015
template <typename T, size_t chunkElementCount>
inline const T &udChunkedArray<T, chunkElementCount>::operator[](size_t index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return ppChunks[chunkIndex]->data[index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline T* udChunkedArray<T,chunkElementCount>::GetElement(size_t  index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return &ppChunks[chunkIndex]->data[index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, January 2016
template <typename T, size_t chunkElementCount>
inline const T* udChunkedArray<T, chunkElementCount>::GetElement(size_t  index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  return &ppChunks[chunkIndex]->data[index % chunkElementCount];
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline void udChunkedArray<T,chunkElementCount>::SetElement(size_t index, const T &data)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index / chunkElementCount;
  ppChunks[chunkIndex]->data[index % chunkElementCount] = data;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::PushBack(T **ppElement)
{
  UDASSERT(ppElement, "parameter is null");

  udResult res = ReserveBack(length + 1);
  if (res != udR_Success)
    return res;

  size_t newIndex = inset + length;
  size_t chunkIndex = size_t(newIndex / chunkElementCount);

  *ppElement = ppChunks[chunkIndex]->data + (newIndex % chunkElementCount);

  ++length;
  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline T *udChunkedArray<T,chunkElementCount>::PushBack()
{
  T *pElement = nullptr;

  if (PushBack(&pElement) == udR_Success)
    memset(pElement, 0, sizeof(T));

  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::PushBack(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushBack(&pElement);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::PushFront(T **ppElement)
{
  UDASSERT(ppElement, "parameter is null");

  if (inset)
  {
    --inset;
  }
  else
  {
    if (length)
    {
      // Are we out of pointers
      if ((chunkCount + 1) > ptrArraySize)
      {
        chunk_t **ppNewChunks = udAllocType(chunk_t*, (ptrArraySize + ptrArrayInc), udAF_Zero);
        if (!ppNewChunks)
          return udR_MemoryAllocationFailure;

        ptrArraySize += ptrArrayInc;
        memcpy(ppNewChunks + 1, ppChunks, chunkCount * sizeof(chunk_t*));

        udFree(ppChunks);
        ppChunks = ppNewChunks;
      }
      else
      {
        memmove(ppChunks + 1, ppChunks, chunkCount * sizeof(chunk_t*));
      }

      // See if we have an unused chunk at the end of the array
      if (((chunkCount * chunkElementCount) - length) > chunkElementCount)
      {
        ppChunks[0] = ppChunks[chunkCount - 1];
        ppChunks[chunkCount - 1] = nullptr;
      }
      else
      {
        chunk_t *pNewBlock = udAllocType(chunk_t, 1, udAF_None);
        if (!pNewBlock)
        {
          memmove(ppChunks, ppChunks + 1, chunkCount * sizeof(chunk_t*));
          return udR_MemoryAllocationFailure;
        }
        ppChunks[0] = pNewBlock;
        ++chunkCount;
      }
    }
    inset = chunkElementCount - 1;
  }

  ++length;

  *ppElement = ppChunks[0]->data + inset;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline T *udChunkedArray<T,chunkElementCount>::PushFront()
{
  T *pElement = nullptr;

  if (PushFront(&pElement) == udR_Success)
    memset(pElement, 0, sizeof(T));

  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::PushFront(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushFront(&pElement);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline bool udChunkedArray<T, chunkElementCount>::PopBack(T *pDest)
{
  if (length)
  {
    if (pDest)
      *pDest = *GetElement(length - 1);
    --length;

    if (length == 0)
      inset = 0;
    return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T, size_t chunkElementCount>
inline bool udChunkedArray<T, chunkElementCount>::PopFront(T *pDest)
{
  if (length)
  {
    if (pDest)
      *pDest = *GetElement(0);
    ++inset;
    if (inset == chunkElementCount)
    {
      inset = 0;
      if (chunkCount > 1)
      {
        chunk_t *pHead = ppChunks[0];
        memmove(ppChunks, ppChunks + 1, (chunkCount - 1) * sizeof(chunk_t*));
        ppChunks[chunkCount - 1] = pHead;
      }
    }

    --length;

    if (length == 0)
      inset = 0;
    return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2015
template <typename T, size_t chunkElementCount>
inline void udChunkedArray<T, chunkElementCount>::RemoveSwapLast(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");

  // Only copy the last element over if the element being removed isn't the last element
  if (index != (length - 1))
    SetElement(index, *GetElement(length - 1));
  PopBack();
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, October 2015
template <typename T, size_t chunkElementCount>
inline void udChunkedArray<T, chunkElementCount>::RemoveAt(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");

  if (index == 0)
  {
    PopFront();
  }
  else if (index == (length - 1))
  {
    PopBack();
  }
  else
  {
    index += inset;

    size_t chunkIndex = index / chunkElementCount;

    // Move within the chunk of the remove item
    if ((index % chunkElementCount) != (chunkElementCount - 1)) // If there are items after the remove item
      memmove(&ppChunks[chunkIndex]->data[index % chunkElementCount], &ppChunks[chunkIndex]->data[(index + 1) % chunkElementCount], sizeof(T) * (chunkElementCount - 1 - (index % chunkElementCount)));

    // Handle middle chunks
    for (size_t i = (chunkIndex + 1); i < (chunkCount - 1); ++i)
    {
      // Move first item down
      memcpy(&ppChunks[i - 1]->data[chunkElementCount - 1], &ppChunks[i]->data[0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[i]->data[0], &ppChunks[i]->data[1], sizeof(T) * (chunkElementCount - 1));
    }

    // Handle last chunk
    if (chunkIndex != (chunkCount - 1))
    {
      // Move first item down
      memcpy(&ppChunks[chunkCount - 2]->data[chunkElementCount - 1], &ppChunks[chunkCount - 1]->data[0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[chunkCount - 1]->data[0], &ppChunks[chunkCount - 1]->data[1], sizeof(T) * ((length + (inset - 1)) % chunkElementCount));
    }

    PopBack();
  }
}

// --------------------------------------------------------------------------
// Author: Bryce Kiefer, November 2015
template <typename T, size_t chunkElementCount>
inline udResult udChunkedArray<T, chunkElementCount>::Insert(size_t index, const T *pData)
{
  UDASSERT(index <= length, "Index out of bounds");

  // Make room for new element
  udResult result = GrowBack(1);
  if (result != udR_Success)
    return result;

  // TODO: This should be changed to a per-chunk loop,
  // using memmove to move all but the last element, and a
  // memcpy from the previous chunk for the (now first)
  // element of each chunk

  // Move each element at and after the insertion point to the right by one
  for (size_t i = length - 1; i > index; --i)
  {
    memcpy(&ppChunks[i / chunkElementCount]->data[i % chunkElementCount], &ppChunks[(i - 1) / chunkElementCount]->data[(i - 1) % chunkElementCount], sizeof(T));
  }

  // Copy the new element into the insertion point if it exists
  if (pData != nullptr)
    memcpy(&ppChunks[index / chunkElementCount]->data[index % chunkElementCount], pData, sizeof(T));

  return result;
}

#endif // UDCHUNKEDARRAY_H
