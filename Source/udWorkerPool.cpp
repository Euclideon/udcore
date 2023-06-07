#include "udWorkerPool.h"

#include "udSafeDeque.h"

#include "udChunkedArray.h"
#include "udPlatformUtil.h"
#include "udThread.h"
#include "udStringUtil.h"
#include <atomic>
#include <algorithm>

struct udWorkerPoolTask
{
  const char *pTaskName;
  double startTime;

  udWorkerPoolCallback function;
  udWorkerPoolCallback postFunction; // runs on main thread

  void *pDataBlock;
  bool freeDataBlock;
};

struct udWorkerPoolThread
{
  udWorkerPool *pPool;
  udThread *pThread;
  udWorkerPoolTask currentTask; // Used mostly for debugging
};

struct udWorkerPool
{
  udRWLock *pRWLock;

  udChunkedArray<udWorkerPoolTask> queuedTasks;
  udChunkedArray<udWorkerPoolTask> queuedPostTasks;

  udSemaphore *pSemaphore;
  std::atomic<int32_t> activeThreads;

  uint8_t totalThreads;
  udWorkerPoolThread *pThreadData;

  std::atomic<bool> isRunning;
};

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
uint32_t udWorkerPool_DoWork(void *pPoolPtr)
{
  UDRELASSERT(pPoolPtr != nullptr, "Bad Pool Ptr!");

  udWorkerPoolThread *pThreadData = (udWorkerPoolThread*)pPoolPtr;
  udWorkerPool *pPool = pThreadData->pPool;

  int waitValue;

  while (pPool->isRunning)
  {
    waitValue = udWaitSemaphore(pPool->pSemaphore, 100);

    if (waitValue != 0)
      continue;

    ++pPool->activeThreads;

    udWriteLockRWLock(pPool->pRWLock);
    bool poppedOK = pPool->queuedTasks.PopFront(&pThreadData->currentTask);
    udWriteUnlockRWLock(pPool->pRWLock);

    if (!poppedOK)
    {
      --pPool->activeThreads;
      continue;
    }

    if (pThreadData->currentTask.function)
      pThreadData->currentTask.function(pThreadData->currentTask.pDataBlock);

    if (pThreadData->currentTask.postFunction)
    {
      udWriteLockRWLock(pPool->pRWLock);
      pPool->queuedPostTasks.PushBack(pThreadData->currentTask);
      udWriteUnlockRWLock(pPool->pRWLock);
    }
    else
    {
      if (pThreadData->currentTask.freeDataBlock)
        udFree(pThreadData->currentTask.pDataBlock);

      udFree(pThreadData->currentTask.pTaskName);
    }

    --pPool->activeThreads;
  }

  return 0;
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
udResult udWorkerPool_Create(udWorkerPool **ppPool, uint8_t totalThreads, const char *pThreadNamePrefix /*= "udWorkerPool"*/)
{
  udResult result = udR_Failure;
  udWorkerPool *pPool = nullptr;

  UD_ERROR_NULL(ppPool, udR_InvalidParameter);
  UD_ERROR_IF(totalThreads == 0, udR_InvalidParameter);

  pPool = udAllocType(udWorkerPool, 1, udAF_Zero);
  UD_ERROR_NULL(pPool, udR_MemoryAllocationFailure);

  pPool->pRWLock = udCreateRWLock();
  pPool->pSemaphore = udCreateSemaphore();
  UD_ERROR_NULL(pPool, udR_MemoryAllocationFailure);

  UD_ERROR_CHECK(pPool->queuedTasks.Init(32));
  UD_ERROR_CHECK(pPool->queuedPostTasks.Init(32));

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

  udWriteLockRWLock(pPool->pRWLock);

  udWorkerPoolTask currentTask;
  while (pPool->queuedTasks.PopFront(&currentTask))
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
    udFree(currentTask.pTaskName);
  }

  while (pPool->queuedPostTasks.PopFront(&currentTask))
  {
    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);
    udFree(currentTask.pTaskName);
  }

  pPool->queuedTasks.Deinit();
  pPool->queuedPostTasks.Deinit();
  udDestroySemaphore(&pPool->pSemaphore);

  udWriteUnlockRWLock(pPool->pRWLock);
  udDestroyRWLock(&pPool->pRWLock);

  udFree(pPool->pThreadData);
  udFree(pPool);
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, May 2015
udResult udWorkerPool_AddNamedTask(udWorkerPool *pPool, const char *pTaskName, udWorkerPoolCallback func, void *pUserData /*= nullptr*/, bool clearMemory /*= true*/, udWorkerPoolCallback postFunction /*= nullptr*/)
{
  udResult result = udR_Failure;
  udWorkerPoolTask tempTask;

  UD_ERROR_NULL(pPool, udR_InvalidParameter);
  UD_ERROR_NULL(pPool->pSemaphore, udR_NotInitialized);
  UD_ERROR_NULL(pPool->pRWLock, udR_NotInitialized);
  UD_ERROR_IF(!pPool->isRunning, udR_NotAllowed);

  tempTask.function = func;
  tempTask.postFunction = postFunction;
  tempTask.pDataBlock = pUserData;
  tempTask.freeDataBlock = clearMemory;


  tempTask.pTaskName = udStrdup(pTaskName);
  tempTask.startTime = udGetEpochSecsUTCf();

  udWriteLockRWLock(pPool->pRWLock);
  if (func == nullptr && postFunction != nullptr)
    UD_ERROR_CHECK(udSafeDeque_PushBack(pPool->pQueuedPostTasks, tempTask));
  else
    UD_ERROR_CHECK(udSafeDeque_PushBack(pPool->pQueuedTasks, tempTask));
  udWriteUnlockRWLock(pPool->pRWLock);


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
  bool popSuccess = false;

  UD_ERROR_NULL(pPool, udR_InvalidParameter);
  UD_ERROR_NULL(pPool->pRWLock, udR_NotInitialized);
  UD_ERROR_NULL(pPool->pSemaphore, udR_NotInitialized);
  UD_ERROR_IF(!pPool->isRunning, udR_NotAllowed);

  
  while (true)
  {
    udWriteLockRWLock(pPool->pRWLock);
    popSuccess = pPool->queuedPostTasks.PopFront(&currentTask);
    udWriteUnlockRWLock(pPool->pRWLock);

    if (!popSuccess)
      break;

    currentTask.postFunction(currentTask.pDataBlock);

    if (currentTask.freeDataBlock)
      udFree(currentTask.pDataBlock);

    udFree(currentTask.pTaskName);

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
bool udWorkerPool_HasActiveWorkers(udWorkerPool *pPool, size_t *pActiveThreads /*= nullptr*/, size_t *pQueuedWTTasks /*= nullptr*/, size_t *pQueuedMTTasks /*= nullptr*/)
{
  if (pPool == nullptr)
    return false;

  udReadLockRWLock(pPool->pRWLock);
  int32_t activeThreads = pPool->activeThreads;
  size_t queuedWTTasks = pPool->queuedTasks.length;
  size_t queuedMTTasks = pPool->queuedPostTasks.length;
  udReadUnlockRWLock(pPool->pRWLock);

  if (pActiveThreads)
    *pActiveThreads = (size_t)std::max(activeThreads, 0);

  if (pQueuedWTTasks)
    *pQueuedWTTasks = queuedWTTasks;

  if (pQueuedMTTasks)
    *pQueuedMTTasks = queuedMTTasks;

  return (activeThreads > 0 || queuedWTTasks > 0 || queuedMTTasks > 0);
}
