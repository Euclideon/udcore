#include "udWorkerPool.h"

#include "udSafeDeque.h"

#include "udChunkedArray.h"
#include "udPlatformUtil.h"
#include "udThread.h"
#include "udMath.h"
#include "udStringUtil.h"

struct udWorkerPoolThread
{
  udWorkerPool *pPool;
  udThread *pThread;
};

struct udWorkerPoolTask
{
  udWorkerPoolCallback function;
  udWorkerPoolCallback postFunction; // runs on main thread
  void *pDataBlock;
  bool freeDataBlock;
};

struct udWorkerPool
{
  udSafeDeque<udWorkerPoolTask> *pQueuedTasks;
  udSafeDeque<udWorkerPoolTask> *pQueuedPostTasks;

  udSemaphore *pSemaphore;
  volatile int32_t activeThreads;

  uint8_t totalThreads;
  udWorkerPoolThread *pThreadData;

  udInterlockedBool isRunning;
};

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
uint32_t udWorkerPool_DoWork(void *pPoolPtr)
{
  UDRELASSERT(pPoolPtr != nullptr, "Bad Pool Ptr!");

  udWorkerPoolThread *pThreadData = (udWorkerPoolThread*)pPoolPtr;
  udWorkerPool *pPool = pThreadData->pPool;

  udWorkerPoolTask currentTask;
  int waitValue;

  while (pPool->isRunning)
  {
    waitValue = udWaitSemaphore(pPool->pSemaphore, 100);

    if (waitValue != 0)
      continue;

    udInterlockedPreIncrement(&pPool->activeThreads);

    if (udSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) != udR_Success)
    {
      udInterlockedPreIncrement(&pPool->activeThreads);
      continue;
    }

    if (currentTask.function)
      currentTask.function(currentTask.pDataBlock);

    if (currentTask.postFunction)
      udSafeDeque_PushBack(pPool->pQueuedPostTasks, currentTask);
    else if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);

    udInterlockedPreDecrement(&pPool->activeThreads);
  }

  return 0;
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
udResult udWorkerPool_Create(udWorkerPool **ppPool, uint8_t totalThreads, const char *pThreadNamePrefix /*= "udWorkerPool"*/)
{
  udResult result = udR_Failure_;
  udWorkerPool *pPool = nullptr;

  UD_ERROR_NULL(ppPool, udR_InvalidParameter_);
  UD_ERROR_IF(totalThreads == 0, udR_InvalidParameter_);

  pPool = udAllocType(udWorkerPool, 1, udAF_Zero);
  UD_ERROR_NULL(pPool, udR_MemoryAllocationFailure);

  pPool->pSemaphore = udCreateSemaphore();
  UD_ERROR_NULL(pPool, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(udSafeDeque_Create(&pPool->pQueuedTasks, 32));
  UD_ERROR_CHECK(udSafeDeque_Create(&pPool->pQueuedPostTasks, 32));

  pPool->isRunning = true;
  pPool->totalThreads = totalThreads;
  pPool->pThreadData = udAllocType(udWorkerPoolThread, pPool->totalThreads, udAF_Zero);
  UD_ERROR_NULL(pPool->pThreadData, udR_MemoryAllocationFailure);

  for (int i = 0; i < pPool->totalThreads; ++i)
  {
    pPool->pThreadData[i].pPool = pPool;
    UD_ERROR_CHECK(udThread_Create(&pPool->pThreadData[i].pThread, udWorkerPool_DoWork, &pPool->pThreadData[i], udTCF_None, udTempStr("%s%d", pThreadNamePrefix, i)));
  }

  result = udR_Success;
  *ppPool = pPool;
  pPool = nullptr;

epilogue:
  udWorkerPool_Destroy(&pPool);

  return result;
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
void udWorkerPool_Destroy(udWorkerPool **ppPool)
{
  if (ppPool == nullptr || *ppPool == nullptr)
    return;

  udWorkerPool *pPool = *ppPool;
  *ppPool = nullptr;

  pPool->isRunning = false;

  for (int i = 0; i < pPool->totalThreads; i++)
  {
    udThread_Join(pPool->pThreadData[i].pThread);
    udThread_Destroy(&pPool->pThreadData[i].pThread);
  }

  udWorkerPoolTask currentTask;
  while (udSafeDeque_PopFront(pPool->pQueuedTasks, &currentTask) == udR_Success)
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }

  while (udSafeDeque_PopFront(pPool->pQueuedPostTasks, &currentTask) == udR_Success)
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
  }

  udSafeDeque_Destroy(&pPool->pQueuedTasks);
  udSafeDeque_Destroy(&pPool->pQueuedPostTasks);
  udDestroySemaphore(&pPool->pSemaphore);

  udFree(pPool->pThreadData);
  udFree(pPool);
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
udResult udWorkerPool_AddTask(udWorkerPool *pPool, udWorkerPoolCallback func, void *pUserData /*= nullptr*/, bool clearMemory /*= true*/, udWorkerPoolCallback postFunction /*= nullptr*/)
{
  udResult result = udR_Failure_;
  udWorkerPoolTask tempTask;

  UD_ERROR_NULL(pPool, udR_InvalidParameter_);
  UD_ERROR_NULL(pPool->pQueuedTasks, udR_NotInitialized_);
  UD_ERROR_NULL(pPool->pQueuedPostTasks, udR_NotInitialized_);
  UD_ERROR_NULL(pPool->pSemaphore, udR_NotInitialized_);
  UD_ERROR_IF(!pPool->isRunning, udR_NotAllowed);

  tempTask.function = func;
  tempTask.postFunction = postFunction;
  tempTask.pDataBlock = pUserData;
  tempTask.freeDataBlock = clearMemory;

  UD_ERROR_CHECK(udSafeDeque_PushBack(pPool->pQueuedTasks, tempTask));
  udIncrementSemaphore(pPool->pSemaphore);

  result = udR_Success;

epilogue:
  return result;
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
udResult udWorkerPool_DoPostWork(udWorkerPool *pPool, int processLimit /*= 0*/)
{
  udWorkerPoolTask currentTask;
  udResult result = udR_Success;
  int processedItems = 0;

  UD_ERROR_NULL(pPool, udR_InvalidParameter_);
  UD_ERROR_NULL(pPool->pQueuedTasks, udR_NotInitialized_);
  UD_ERROR_NULL(pPool->pQueuedPostTasks, udR_NotInitialized_);
  UD_ERROR_NULL(pPool->pSemaphore, udR_NotInitialized_);
  UD_ERROR_IF(!pPool->isRunning, udR_NotAllowed);

  while (udSafeDeque_PopFront(pPool->pQueuedPostTasks, &currentTask) == udR_Success)
  {
    currentTask.postFunction(currentTask.pDataBlock);

    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);

    if (++processedItems == processLimit)
      break;
  }

epilogue:
  if (result == udR_Success && processedItems == 0)
    return udR_NothingToDo;

  return result;
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
bool udWorkerPool_HasActiveWorkers(udWorkerPool *pPool)
{
  if (pPool == nullptr)
    return false;

  udLockMutex(pPool->pQueuedTasks->pMutex);
  bool isActive = (pPool->activeThreads > 0 || pPool->pQueuedTasks->chunkedArray.length > 0);
  udReleaseMutex(pPool->pQueuedTasks->pMutex);

  return isActive;
}
