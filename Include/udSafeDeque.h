#ifndef UDSAFEDEQUE_H
#define UDSAFEDEQUE_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Paul Fox, November 2015 (Originally from cutil and later updated in vCore)
//
// Wraps udChunkedArray to provide a thread safe strict deque interface
//

#include "udResult.h"
#include "udChunkedArray.h"
#include "udThread.h"

/// Thread safe double ended queue
template <typename T>
struct udSafeDeque
{
  udChunkedArray<T> chunkedArray;
  udMutex *pMutex = nullptr;
};

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
udResult udSafeDeque_Create(udSafeDeque<T> **ppDeque, size_t elementCount)
{
  udResult result = udR_Failure;
  udSafeDeque<T> *pDeque = nullptr;

  UD_ERROR_NULL(ppDeque, udR_InvalidParameter);
  UD_ERROR_IF(elementCount == 0, udR_InvalidParameter);

  pDeque = udAllocType(udSafeDeque<T>, 1, udAF_Zero);

  UD_ERROR_NULL(pDeque, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(pDeque->chunkedArray.Init(elementCount));

  pDeque->pMutex = udCreateMutex();
  UD_ERROR_NULL(pDeque, udR_MemoryAllocationFailure);

  *ppDeque = pDeque;
  pDeque = nullptr;

epilogue:
  udSafeDeque_Destroy(&pDeque); // Will be nullptr on success

  return result;
}

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
void udSafeDeque_Destroy(udSafeDeque<T> **ppDeque)
{
  if (ppDeque == nullptr || *ppDeque == nullptr)
    return;

  udDestroyMutex(&(*ppDeque)->pMutex);
  (*ppDeque)->chunkedArray.Deinit();

  udFree(*ppDeque);
}

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
inline udResult udSafeDeque_PushBack(udSafeDeque<T> *pDeque, const T &v)
{
  udResult result = udR_Failure;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  UD_ERROR_CHECK(pDeque->chunkedArray.PushBack(v));

epilogue:
  udReleaseMutex(pMutex);

  return result;
}

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
inline udResult udSafeDeque_PushFront(udSafeDeque<T> *pDeque, const T &v)
{
  udResult result = udR_Failure;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  UD_ERROR_CHECK(pDeque->chunkedArray.PushFront(v));

epilogue:
  udReleaseMutex(pMutex);

  return result;
}

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
inline udResult udSafeDeque_PopBack(udSafeDeque<T> *pDeque, T *pData)
{
  udResult result = udR_Success;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  UD_ERROR_IF(!pDeque->chunkedArray.PopBack(pData), udR_NotFound);

epilogue:
  udReleaseMutex(pMutex);

  return result;
}

// ****************************************************************************
// Author: Paul Fox, November 2015
template <typename T>
inline udResult udSafeDeque_PopFront(udSafeDeque<T> *pDeque, T *pData)
{
  udResult result = udR_Success;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  if (!pDeque->chunkedArray.PopFront(pData))
    result = udR_NotFound;

epilogue:
  udReleaseMutex(pMutex);

  return result;
}

template <typename T>
inline udResult udSafeDeque_GetElement(udSafeDeque<T> *pDeque, T* dest, size_t index)
{
  udResult result = udR_Success;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);
  UD_ERROR_NULL(dest, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  UD_ERROR_IF(pDeque->chunkedArray.length < index, udR_OutOfRange);

  *dest = pDeque->chunkedArray.GetElement(index);

epilogue:
  udReleaseMutex(pMutex);

  return result;
}

template <typename T>
inline bool udSafeDeque_Contains(udSafeDeque<T> *pDeque, const T &value)
{
  udResult result = udR_NotFound;
  udMutex *pMutex = nullptr;

  UD_ERROR_NULL(pDeque, udR_InvalidParameter);

  pMutex = udLockMutex(pDeque->pMutex);
  UD_ERROR_NULL(pMutex, udR_NotInitialized);
  for (size_t queueIndex = 0; queueIndex < pDeque->chunkedArray.length; ++queueIndex)
  {
    if (*pDeque->chunkedArray.GetElement(queueIndex) == value)
    {
      result = udR_Success;
      goto epilogue;
    }
  }


epilogue:
  udReleaseMutex(pMutex);

  return result;
}

// Straight selection sort
template <typename T>
inline udResult udSafeDeque_SortSS(udSafeDeque<T> *pDeque, bool (*comp)(T,T), bool (*equiv)(T,T) = nullptr)
{
  udResult result = udR_Success;
  udMutex *pMutex = udLockMutex(pDeque->pMutex);
  UD_ERROR_NULL(pDeque, udR_InvalidParameter);
  UD_ERROR_NULL(comp, udR_InvalidParameter);

  UD_ERROR_NULL(pMutex, udR_NotInitialized);

  pDeque->chunkedArray.SortSS(comp, equiv);

epilogue:
  udReleaseMutex(pMutex);

  return result;
}
#endif // UDSAFEDEQUE_H
