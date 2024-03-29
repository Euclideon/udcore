#ifndef UDCHUNKEDARRAY_H
#define UDCHUNKEDARRAY_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: David Ely, May 2015
//
// A simple container for keeping potentially large arrays without large contiguous allocations
//

#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udResult.h"
#include <iterator>

// --------------------------------------------------------------------------
template <typename T>
struct udChunkedArrayIterator
{
  // definitions necessary for compatibility with LegacyRandomAccessIterator
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = T;
  using pointer = T *;
  using reference = T &;

  T **ppCurrChunk;

  size_t currChunkElementIndex;
  size_t chunkElementCount;

  difference_type operator-(const udChunkedArrayIterator<T> &rhs) const;

  udChunkedArrayIterator<T> &operator++();
  udChunkedArrayIterator<T> operator++(int);
  udChunkedArrayIterator<T> &operator--();
  udChunkedArrayIterator<T> operator--(int);

  inline udChunkedArrayIterator<T> &operator+=(const difference_type a);
  inline udChunkedArrayIterator<T> operator+(const difference_type a) const;

  inline udChunkedArrayIterator<T> &operator-=(const difference_type a);
  inline udChunkedArrayIterator<T> operator-(const difference_type a) const;

  inline bool operator<(const udChunkedArrayIterator<T> &a) const { return (*this - a) < 0; }
  inline bool operator>(const udChunkedArrayIterator<T> &a) const { return (*this - a) > 0; }
  inline bool operator>=(const udChunkedArrayIterator<T> &a) const { return !(*this < a); }
  inline bool operator<=(const udChunkedArrayIterator<T> &a) const { return !(*this > a); }

  reference operator[](const size_t &a) const;
  reference operator*() const;
  bool operator!=(const udChunkedArrayIterator<T> &rhs) const;
  bool operator==(const udChunkedArrayIterator<T> &rhs) const;
};

template <typename T>
inline udChunkedArrayIterator<T> operator+(typename udChunkedArrayIterator<T>::difference_type a, udChunkedArrayIterator<T> b) {
  b += a;
  return b;
}

template <typename T>
inline udChunkedArrayIterator<T> operator-(typename udChunkedArrayIterator<T>::difference_type a, udChunkedArrayIterator<T> b) {
  b -= a;
  return b;
}

template <typename T>
struct udChunkedArrayConstIterator
{
  // definitions necessary for compatibility with LegacyRandomAccessIterator
  using iterator_category = std::random_access_iterator_tag;
  using difference_type = ptrdiff_t;
  using value_type = const T;
  using pointer = const T *;
  using reference = const T &;

  T **ppCurrChunk;

  size_t currChunkElementIndex;
  size_t chunkElementCount;

  difference_type operator-(const udChunkedArrayConstIterator<T> &rhs) const;

  udChunkedArrayConstIterator<T> &operator++();
  udChunkedArrayConstIterator<T> operator++(int);
  udChunkedArrayConstIterator<T> &operator--();
  udChunkedArrayConstIterator<T> operator--(int);

  inline udChunkedArrayConstIterator<T> &operator+=(const difference_type a);
  inline udChunkedArrayConstIterator<T> operator+(const difference_type a) const;

  inline udChunkedArrayConstIterator<T> &operator-=(const difference_type a);
  inline udChunkedArrayConstIterator<T> operator-(const difference_type a) const;

  inline bool operator<(const udChunkedArrayConstIterator<T> &a) const { return (*this - a) < 0; }
  inline bool operator>(const udChunkedArrayConstIterator<T> &a) const { return (*this - a) > 0; }
  inline bool operator>=(const udChunkedArrayConstIterator<T> &a) const { return !(*this < a); }
  inline bool operator<=(const udChunkedArrayConstIterator<T> &a) const { return !(*this > a); }

  reference operator[](const size_t &a) const;
  reference operator*() const;
  bool operator!=(const udChunkedArrayConstIterator<T> &rhs) const;
  bool operator==(const udChunkedArrayConstIterator<T> &rhs) const;
};

template <typename T>
inline udChunkedArrayConstIterator<T> operator+(typename udChunkedArrayConstIterator<T>::difference_type a, udChunkedArrayConstIterator<T> b) {
  b += a;
  return b;
}

template <typename T>
inline udChunkedArrayConstIterator<T> operator-(typename udChunkedArrayConstIterator<T>::difference_type a, udChunkedArrayConstIterator<T> b) {
  b -= a;
  return b;
}


template <typename T>
struct udChunkedArray
{
  udResult Init(size_t chunkElementCount);
  udResult Deinit();
  udResult Clear();

  T &operator[](size_t index);
  const T &operator[](size_t index) const;
  T *GetElement(size_t index);
  const T *GetElement(size_t index) const;
  size_t FindIndex(const T &element, size_t compareLen = sizeof(T)) const; // Linear search for matching element (first compareLen bytes compared)
  void SetElement(size_t index, const T &data);

  udResult PushBack(const T &v);                              // Push a copy of v to the back of the array, can fail if memory allocation fails
  udResult PushBack(T **ppElement, bool zeroMemory = true);   // Get pointer to new element at back of the array, can fail if memory allocation fails when growing array
  T *PushBack();                                              // Get pointer to new zeroed element at back of the array, or NULL on failure

  udResult PushFront(const T &v);                             // Push a copy of v to the front of the array, can fail if memory allocation fails
  udResult PushFront(T **ppElement, bool zeroMemory = true);  // Get pointer to new element at front of the array, can fail if memory allocation fails when growing array
  T *PushFront();                                             // Get pointer to new zeroed element at front of the array, or NULL on failure

  udResult Insert(size_t index, const T *pData = nullptr);  // Insert the element at index, pushing and moving all elements after to make space.

  bool PopBack(T *pData = nullptr);              // Returns false if no element to pop
  bool PopFront(T *pData = nullptr);             // Returns false if no element to pop
  void RemoveAt(size_t index);                   // Remove the element at index, moving all elements after to fill the gap.
  void RemoveSwapLast(size_t index);             // Remove the element at index, swapping with the last element to ensure array is contiguous

  udResult ToArray(T *pArray, size_t arrayLength, size_t startIndex = 0, size_t count = 0) const; // Copy elements to an array supplied by caller
  udResult ToArray(T **ppArray, size_t startIndex = 0, size_t count = 0) const;                   // Copy elements to an array allocated and returned to caller

  udResult GrowBack(size_t numberOfNewElements, bool zeroMemory = true); // Push back a number of new elements
  udResult ReserveBack(size_t newCapacity);                              // Reserve memory for a given number of elements without changing 'length'  NOTE: Does not reduce in size
  udResult AddChunks(size_t numberOfNewChunks);                          // Add a given number of chunks capacity without changing 'length'

  // At element index, return the number of elements including index that follow in the same chunk (ie can be indexed directly)
  // Optionally, if elementsBehind is true, returns the the number of elements BEHIND index in the same chunk instead
  size_t GetElementRunLength(size_t index, bool elementsBehind = false) const;
  size_t ChunkElementCount() const { return chunkElementCount; }
  size_t ElementSize() const { return sizeof(T); }

  // Iterators
  typedef udChunkedArrayIterator<T> iterator;
  iterator begin();
  iterator end();

  typedef udChunkedArrayConstIterator<T> const_iterator;
  const_iterator begin() const;
  const_iterator end() const;

  enum { ptrArrayInc = 32 };

  T **ppChunks;
  size_t ptrArraySize;
  size_t chunkElementCount;
  size_t chunkElementCountShift;
  size_t chunkElementCountMask;
  size_t chunkCount;

  size_t length;
  size_t inset;
};

// --------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
template <typename T>
inline udChunkedArrayIterator<T> &udChunkedArrayIterator<T>::operator++()
{
  ++currChunkElementIndex;
  if (currChunkElementIndex == chunkElementCount)
  {
    currChunkElementIndex = 0;
    ++ppCurrChunk;
  }
  return *this;
}

template <typename T>
inline udChunkedArrayIterator<T> udChunkedArrayIterator<T>::operator++(int)
{
  udChunkedArrayIterator<T> ret = *this;
  operator++();
  return ret;
}

template <typename T>
inline udChunkedArrayIterator<T> &udChunkedArrayIterator<T>::operator--()
{
  if (currChunkElementIndex == 0)
  {
    currChunkElementIndex = chunkElementCount;
    --ppCurrChunk;
  }
  --currChunkElementIndex;
  return *this;
}

template <typename T>
inline udChunkedArrayIterator<T> udChunkedArrayIterator<T>::operator--(int)
{
  udChunkedArrayIterator<T> ret = *this;
  operator--();
  return ret;
}

template <typename T>
inline udChunkedArrayIterator<T> &udChunkedArrayIterator<T>::operator+=(const difference_type a)
{
  // the 'index' within the current block we get to without taking into account chunk length
  difference_type rawInd = (a + (difference_type)currChunkElementIndex);
  //index within the chunk adjusting for block size
  difference_type newInd = rawInd % (difference_type)chunkElementCount;
  if (newInd < 0)
    newInd += (difference_type)chunkElementCount;

  // change in chunk index
  difference_type chunkChange = (rawInd < 0 && newInd != 0 ? -1 : 0) + rawInd / (difference_type)chunkElementCount;

  ppCurrChunk += chunkChange;
  currChunkElementIndex = newInd;
  return *this;
}

template <typename T>
inline udChunkedArrayIterator<T> udChunkedArrayIterator<T>::operator+(const difference_type a) const
{
  udChunkedArrayIterator<T> ret = *this;
  ret += a;
  return ret;
}

template <typename T>
inline udChunkedArrayIterator<T> &udChunkedArrayIterator<T>::operator-=(const difference_type a)
{
  return *this += -a;
}

template <typename T>
inline udChunkedArrayIterator<T> udChunkedArrayIterator<T>::operator-(const difference_type a) const
{
  udChunkedArrayIterator<T> ret = *this;
  ret -= a;
  return ret;
}

template <typename T>
typename udChunkedArrayIterator<T>::difference_type udChunkedArrayIterator<T>::operator-(const udChunkedArrayIterator<T> &rhs) const
{
  return (this->ppCurrChunk - rhs.ppCurrChunk) * this->chunkElementCount + this->currChunkElementIndex - rhs.currChunkElementIndex;
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
template <typename T>
typename udChunkedArrayIterator<T>::reference udChunkedArrayIterator<T>::operator*() const
{
  return (*ppCurrChunk)[currChunkElementIndex];
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
template <typename T>
bool udChunkedArrayIterator<T>::operator==(const udChunkedArrayIterator<T> &rhs) const
{
  return (ppCurrChunk == rhs.ppCurrChunk && currChunkElementIndex == rhs.currChunkElementIndex);
}

template <typename T>
bool udChunkedArrayIterator<T>::operator!=(const udChunkedArrayIterator<T> &rhs) const
{
  return !(*this == rhs);
}

template<typename T>
typename udChunkedArrayIterator<T>::reference udChunkedArrayIterator<T>::operator[](const size_t &a) const
{
  return *(*this + a);
}

template <typename T>
inline udChunkedArrayConstIterator<T> &udChunkedArrayConstIterator<T>::operator++()
{
  ++currChunkElementIndex;
  if (currChunkElementIndex == chunkElementCount)
  {
    currChunkElementIndex = 0;
    ++ppCurrChunk;
  }
  return *this;
}

template <typename T>
inline udChunkedArrayConstIterator<T> udChunkedArrayConstIterator<T>::operator++(int)
{
  udChunkedArrayConstIterator<T> ret = *this;
  operator++();
  return ret;
}

template <typename T>
inline udChunkedArrayConstIterator<T> &udChunkedArrayConstIterator<T>::operator--()
{
  if (currChunkElementIndex == 0)
  {
    currChunkElementIndex = chunkElementCount;
    --ppCurrChunk;
  }
  --currChunkElementIndex;
  return *this;
}

template <typename T>
inline udChunkedArrayConstIterator<T> udChunkedArrayConstIterator<T>::operator--(int)
{
  udChunkedArrayConstIterator<T> ret = *this;
  operator--();
  return ret;
}

template <typename T>
inline udChunkedArrayConstIterator<T> &udChunkedArrayConstIterator<T>::operator+=(const difference_type a)
{
  // the 'index' within the current block we get to without taking into account chunk length
  difference_type rawInd = (a + (difference_type)currChunkElementIndex);
  //index within the chunk adjusting for block size
  difference_type newInd = rawInd % (difference_type)chunkElementCount;
  if (newInd < 0)
    newInd += (difference_type)chunkElementCount;

  // change in chunk index
  difference_type chunkChange = (rawInd < 0 && newInd != 0 ? -1 : 0) + rawInd / (difference_type)chunkElementCount;

  ppCurrChunk += chunkChange;
  currChunkElementIndex = newInd;
  return *this;
}

template <typename T>
inline udChunkedArrayConstIterator<T> udChunkedArrayConstIterator<T>::operator+(const difference_type a) const
{
  udChunkedArrayConstIterator<T> ret = *this;
  ret += a;
  return ret;
}

template <typename T>
inline udChunkedArrayConstIterator<T> &udChunkedArrayConstIterator<T>::operator-=(const difference_type a)
{
  return *this += -a;
}

template <typename T>
inline udChunkedArrayConstIterator<T> udChunkedArrayConstIterator<T>::operator-(const difference_type a) const
{
  udChunkedArrayConstIterator<T> ret = *this;
  ret -= a;
  return ret;
}

template <typename T>
typename udChunkedArrayConstIterator<T>::difference_type udChunkedArrayConstIterator<T>::operator-(const udChunkedArrayConstIterator<T> &rhs) const
{
  return (this->ppCurrChunk - rhs.ppCurrChunk) * this->chunkElementCount + this->currChunkElementIndex - rhs.currChunkElementIndex;
}

template <typename T>
typename udChunkedArrayConstIterator<T>::reference udChunkedArrayConstIterator<T>::operator*() const
{
  return (*ppCurrChunk)[currChunkElementIndex];
}

template <typename T>
bool udChunkedArrayConstIterator<T>::operator==(const udChunkedArrayConstIterator<T> &rhs) const
{
  return (ppCurrChunk == rhs.ppCurrChunk && currChunkElementIndex == rhs.currChunkElementIndex);
}

template <typename T>
bool udChunkedArrayConstIterator<T>::operator!=(const udChunkedArrayConstIterator<T> &rhs) const
{
  return !(*this == rhs);
}

template<typename T>
typename udChunkedArrayConstIterator<T>::reference udChunkedArrayConstIterator<T>::operator[](const size_t &a) const
{
  return *(*this + a);
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::Init(size_t a_chunkElementCount)
{
  udResult result = udR_Success;
  size_t c = 0;

  ppChunks = nullptr;
  chunkElementCount = 0;
  chunkCount = 0;
  length = 0;
  inset = 0;

  // Must be power of 2.
  UD_ERROR_IF(!a_chunkElementCount || (a_chunkElementCount & (a_chunkElementCount - 1)), udR_InvalidParameter);

  chunkElementCount = a_chunkElementCount;
  chunkElementCountMask = a_chunkElementCount - 1;
  chunkElementCountShift = udCountBits64((uint64_t)chunkElementCountMask);
  chunkCount = 1;

  if (chunkCount > ptrArrayInc)
    ptrArraySize = ((chunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
  else
    ptrArraySize = ptrArrayInc;

  ppChunks = udAllocType(T *, ptrArraySize, udAF_Zero);
  UD_ERROR_NULL(ppChunks, udR_MemoryAllocationFailure);

  for (; c < chunkCount; ++c)
  {
    ppChunks[c] = udAllocType(T, chunkElementCount, udAF_None);
    UD_ERROR_NULL(ppChunks[c], udR_MemoryAllocationFailure);
  }

epilogue:
  if (result != udR_Success)
  {
    for (size_t i = 0; i < c; ++i)
      udFree(ppChunks[i]);
    udFree(ppChunks);
  }

  return result;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::Deinit()
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
template <typename T>
inline udResult udChunkedArray<T>::Clear()
{
  length = 0;
  inset = 0;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline udResult udChunkedArray<T>::AddChunks(size_t numberOfNewChunks)
{
  size_t newChunkCount = chunkCount + numberOfNewChunks;

  if (newChunkCount > ptrArraySize)
  {
    size_t newPtrArraySize = ((newChunkCount + ptrArrayInc - 1) / ptrArrayInc) * ptrArrayInc;
    T **newppChunks = udAllocType(T *, newPtrArraySize, udAF_Zero);
    if (!newppChunks)
      return udR_MemoryAllocationFailure;

    memcpy(newppChunks, ppChunks, ptrArraySize * sizeof(T *));
    udFree(ppChunks);

    ppChunks = newppChunks;
    ptrArraySize = newPtrArraySize;
  }

  for (size_t c = chunkCount; c < newChunkCount; ++c)
  {
    ppChunks[c] = udAllocType(T, chunkElementCount, udAF_None);
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
template <typename T>
inline udResult udChunkedArray<T>::GrowBack(size_t numberOfNewElements, bool zeroMemory)
{
  if (numberOfNewElements == 0)
    return udR_InvalidParameter;

  size_t oldLength = inset + length;
  size_t newLength = oldLength + numberOfNewElements;
  size_t prevUsedChunkCount = (oldLength + chunkElementCount - 1) >> chunkElementCountShift;

  udResult res = ReserveBack(length + numberOfNewElements);
  if (res != udR_Success)
    return res;

  if (zeroMemory)
  {
    // Zero new elements
    size_t newUsedChunkCount = (newLength + chunkElementCount - 1) >> chunkElementCountShift;
    size_t usedChunkDelta = newUsedChunkCount - prevUsedChunkCount;
    size_t head = oldLength & chunkElementCountMask;

    if (usedChunkDelta)
    {
      if (head)
        memset(&ppChunks[prevUsedChunkCount - 1][head], 0, (chunkElementCount - head) * sizeof(T));

      size_t tail = newLength & chunkElementCountMask;

      for (size_t chunkIndex = prevUsedChunkCount; chunkIndex < (newUsedChunkCount - 1 + !tail); ++chunkIndex)
        memset(ppChunks[chunkIndex], 0, sizeof(T) * chunkElementCount);

      if (tail)
        memset(&ppChunks[newUsedChunkCount - 1][0], 0, tail * sizeof(T));
    }
    else
    {
      memset(&ppChunks[prevUsedChunkCount - 1][head], 0, numberOfNewElements * sizeof(T));
    }
  }

  length += numberOfNewElements;

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::ReserveBack(size_t newCapacity)
{
  udResult res = udR_Success;
  size_t oldCapacity = chunkElementCount * chunkCount - inset;
  if (newCapacity > oldCapacity)
  {
    size_t newChunksCount = (newCapacity - oldCapacity + chunkElementCount - 1) >> chunkElementCountShift;
    res = AddChunks(newChunksCount);
  }
  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline T &udChunkedArray<T>::operator[](size_t index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index >> chunkElementCountShift;
  return ppChunks[chunkIndex][index & chunkElementCountMask];
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, September 2015
template <typename T>
inline const T &udChunkedArray<T>::operator[](size_t index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index >> chunkElementCountShift;
  return ppChunks[chunkIndex][index & chunkElementCountMask];
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline T *udChunkedArray<T>::GetElement(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index >> chunkElementCountShift;
  return &ppChunks[chunkIndex][index & chunkElementCountMask];
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, January 2016
template <typename T>
inline const T *udChunkedArray<T>::GetElement(size_t index) const
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index >> chunkElementCountShift;
  return &ppChunks[chunkIndex][index & chunkElementCountMask];
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, March 2018
template <typename T>
inline size_t udChunkedArray<T>::FindIndex(const T &element, size_t compareLen) const
{
  size_t index = 0;
  while (index < length)
  {
    size_t runLen = GetElementRunLength(index);
    const T *pRun = GetElement(index);
    for (size_t runIndex = 0; runIndex < runLen; ++runIndex)
    {
      if (memcmp(&element, &pRun[runIndex], compareLen) == 0)
        return index + runIndex;
    }
    index += runLen;
  }
  return length; // Sentinal to indicate not found
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline void udChunkedArray<T>::SetElement(size_t index, const T &data)
{
  UDASSERT(index < length, "Index out of bounds");
  index += inset;
  size_t chunkIndex = index >> chunkElementCountShift;
  ppChunks[chunkIndex][index & chunkElementCountMask] = data;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushBack(T **ppElement, bool zeroMemory)
{
  UDASSERT(ppElement, "parameter is null");

  udResult res = ReserveBack(length + 1);
  if (res != udR_Success)
    return res;

  size_t newIndex = inset + length;
  size_t chunkIndex = size_t(newIndex >> chunkElementCountShift);

  *ppElement = ppChunks[chunkIndex] + (newIndex & chunkElementCountMask);
  if (zeroMemory)
    memset((void*)*ppElement, 0, sizeof(T));

  ++length;
  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline T *udChunkedArray<T>::PushBack()
{
  T *pElement = nullptr;
  PushBack(&pElement, true);
  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushBack(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushBack(&pElement, false);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, March 2016
template <typename T>
inline udResult udChunkedArray<T>::PushFront(T **ppElement, bool zeroMemory)
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
        T **ppNewChunks = udAllocType(T *, (ptrArraySize + ptrArrayInc), udAF_Zero);
        if (!ppNewChunks)
          return udR_MemoryAllocationFailure;

        ptrArraySize += ptrArrayInc;
        memcpy(ppNewChunks + 1, ppChunks, chunkCount * sizeof(T *));

        udFree(ppChunks);
        ppChunks = ppNewChunks;
      }
      else
      {
        memmove(ppChunks + 1, ppChunks, chunkCount * sizeof(T *));
      }

      // See if we have an unused chunk at the end of the array
      if (((chunkCount * chunkElementCount) - length) > chunkElementCount)
      {
        //Note that these operations are not chunkCount-1 as the array has been moved above and chunked-1 is actually @chunkcount now
        ppChunks[0] = ppChunks[chunkCount];
        ppChunks[chunkCount] = nullptr;
      }
      else
      {
        T *pNewBlock = udAllocType(T, chunkElementCount, udAF_None);
        if (!pNewBlock)
        {
          memmove(ppChunks, ppChunks + 1, chunkCount * sizeof(T *));
          return udR_MemoryAllocationFailure;
        }
        ppChunks[0] = pNewBlock;
        ++chunkCount;
      }
    }
    inset = chunkElementCount - 1;
  }

  ++length;

  *ppElement = ppChunks[0] + inset;
  if (zeroMemory)
    memset((void*)*ppElement, 0, sizeof(T));

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline T *udChunkedArray<T>::PushFront()
{
  T *pElement = nullptr;
  PushFront(&pElement, true);
  return pElement;
}

// --------------------------------------------------------------------------
// Author: Khan Maxfield, February 2016
template <typename T>
inline udResult udChunkedArray<T>::PushFront(const T &v)
{
  T *pElement = nullptr;

  udResult res = PushFront(&pElement, false);
  if (res == udR_Success)
    *pElement = v;

  return res;
}

// --------------------------------------------------------------------------
// Author: David Ely, May 2015
template <typename T>
inline bool udChunkedArray<T>::PopBack(T *pDest)
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
template <typename T>
inline bool udChunkedArray<T>::PopFront(T *pDest)
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
        T *pHead = ppChunks[0];
        memmove(ppChunks, ppChunks + 1, (chunkCount - 1) * sizeof(T *));
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
// Author: Samuel Surtees, October 2015
template <typename T>
inline void udChunkedArray<T>::RemoveAt(size_t index)
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

    size_t chunkIndex = index >> chunkElementCountShift;

    // Move within the chunk of the remove item
    if ((index % chunkElementCount) != (chunkElementCount - 1)) // If there are items after the remove item
      memmove(&ppChunks[chunkIndex][index & chunkElementCountMask], &ppChunks[chunkIndex][(index + 1) & chunkElementCountMask], sizeof(T) * (chunkElementCount - 1 - (index & chunkElementCountMask)));

    // Handle middle chunks
    for (size_t i = (chunkIndex + 1); i < (chunkCount - 1); ++i)
    {
      // Move first item down
      memcpy(&ppChunks[i - 1][chunkElementCount - 1], &ppChunks[i][0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[i][0], &ppChunks[i][1], sizeof(T) * (chunkElementCount - 1));
    }

    // Handle last chunk
    if (chunkIndex != (chunkCount - 1))
    {
      // Move first item down
      memcpy(&ppChunks[chunkCount - 2][chunkElementCount - 1], &ppChunks[chunkCount - 1][0], sizeof(T));

      // Move remaining items
      memmove(&ppChunks[chunkCount - 1][0], &ppChunks[chunkCount - 1][1], sizeof(T) * ((length + (inset - 1)) & chunkElementCountMask));
    }

    PopBack();
  }
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2015
template <typename T>
inline void udChunkedArray<T>::RemoveSwapLast(size_t index)
{
  UDASSERT(index < length, "Index out of bounds");

  // Only copy the last element over if the element being removed isn't the last element
  if (index != (length - 1))
    SetElement(index, *GetElement(length - 1));
  PopBack();
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
template <typename T>
inline udResult udChunkedArray<T>::ToArray(T *pArray, size_t arrayLength, size_t startIndex, size_t count) const
{
  udResult result;

  if (count == 0)
    count = length - startIndex;
  UD_ERROR_IF(startIndex >= length, udR_OutOfRange);
  UD_ERROR_IF((startIndex + count) > length, udR_OutOfRange);
  UD_ERROR_NULL(pArray, udR_InvalidParameter);
  UD_ERROR_IF(arrayLength < count, udR_BufferTooSmall);
  while (count)
  {
    size_t runLen = GetElementRunLength(startIndex);
    if (runLen > count)
      runLen = count;
    memcpy(pArray, GetElement(startIndex), runLen * sizeof(T));
    pArray += runLen;
    startIndex += runLen;
    count -= runLen;
  }
  result = udR_Success;

epilogue:
  return result;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
template <typename T>
udResult udChunkedArray<T>::ToArray(T **ppArray, size_t startIndex, size_t count) const
{
  udResult result;
  T *pArray = nullptr;

  if (count == 0)
    count = length - startIndex;
  UD_ERROR_IF(startIndex >= length, udR_OutOfRange);
  UD_ERROR_NULL(ppArray, udR_InvalidParameter);

  if (count)
  {
    pArray = udAllocType(T, count, udAF_None);
    UD_ERROR_NULL(pArray, udR_MemoryAllocationFailure);
    UD_ERROR_CHECK(ToArray(pArray, count, startIndex, count));
  }
  // Transfer ownership of array and assign success
  *ppArray = pArray;
  pArray = nullptr;
  result = udR_Success;

epilogue:
  udFree(pArray);
  return result;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
template <typename T>
inline udResult udChunkedArray<T>::Insert(size_t index, const T *pData)
{
  UDASSERT(index <= length, "Index out of bounds");

  if (inset != 0 && (index + inset) < chunkElementCount)
  {
    // Special case: if inserting into the first chunk and there's an inset,
    // move everything before the index back one and decrement the inset
    if (index > 0)
      memmove(&ppChunks[0][inset - 1], &ppChunks[0][inset], index * sizeof(T));
    --inset;
    ++length;
  }
  else
  {
    // Make room for new element
    udResult result = GrowBack(1, false);
    if (result != udR_Success)
      return result;

    for (size_t dst = length - 1; dst > index; )
    {
      size_t src = dst - 1;
      size_t srcBackRunLen = GetElementRunLength(src, true);
      size_t dstBackRunLen = GetElementRunLength(dst, true);
      if ((src - srcBackRunLen) < index)
        srcBackRunLen = src - index;
      size_t runLen = ((dstBackRunLen < srcBackRunLen) ? dstBackRunLen : srcBackRunLen) + 1;
      T *pSrc = GetElement(src - runLen + 1);
      T *pDst = GetElement(dst - runLen + 1);
      memmove(pDst, pSrc, runLen * sizeof(T));
      dst -= runLen;
    }
  }

  // Copy the new element into the insertion point if it exists
  if (pData)
    memcpy(GetElement(index), pData, sizeof(T));

  return udR_Success;
}

// --------------------------------------------------------------------------
// Author: Dave Pevreal, November 2017
template <typename T>
inline size_t udChunkedArray<T>::GetElementRunLength(size_t index, bool elementsBehind) const
{
  if (index < length)
  {
    size_t indexInChunk = ((index + inset) & chunkElementCountMask);
    if (elementsBehind)
      return ((index + inset) >= chunkElementCount) ? indexInChunk : (indexInChunk - inset);
    size_t runLength = chunkElementCount - indexInChunk;
    return (runLength > (length - index)) ? (length - index) : runLength;
  }
  return 0;
}

#define CreateIterator(ppChunks, inset, chunkElementCount, chunkElementCountShift, chunkElementCountMask, startInd) { \
  /*ppCurrChunk =*/ &ppChunks[((inset) + (startInd)) >> (chunkElementCountShift)], \
  /*currChunkElementIndex =*/ ((inset) + (startInd)) &(chunkElementCountMask), \
  /*chunkElementCount =*/ (chunkElementCount) \
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
template <typename T>
inline typename udChunkedArray<T>::iterator udChunkedArray<T>::begin()
{
  return CreateIterator(this->ppChunks, this->inset, this->chunkElementCount, this->chunkElementCountShift, this->chunkElementCountMask, 0);
}

// --------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
template <typename T>
inline typename udChunkedArray<T>::iterator udChunkedArray<T>::end()
{
  return CreateIterator(this->ppChunks, this->inset, this->chunkElementCount, this->chunkElementCountShift, this->chunkElementCountMask, this->length);
}

template <typename T>
inline typename udChunkedArray<T>::const_iterator udChunkedArray<T>::begin() const
{
  return CreateIterator(this->ppChunks, this->inset, this->chunkElementCount, this->chunkElementCountShift, this->chunkElementCountMask, 0);
}

template <typename T>
inline typename udChunkedArray<T>::const_iterator udChunkedArray<T>::end() const
{
  return CreateIterator(this->ppChunks, this->inset, this->chunkElementCount, this->chunkElementCountShift, this->chunkElementCountMask, this->length);
}

#undef CreateIterator

#endif // UDCHUNKEDARRAY_H
