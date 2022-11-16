#include "gtest/gtest.h"

#include "udPlatformUtil.h"
#include "udThread.h"
#include "udWorkerPool.h"

struct WorkerTestData
{
  udSemaphore *pSema;
  int *pInt;
};

void UpdateDataPlusOne(void *pDataPtr)
{
  WorkerTestData *pData = (WorkerTestData *)pDataPtr;

  *pData->pInt = (*pData->pInt) + 1;

  udIncrementSemaphore(pData->pSema);
}

void UpdateDataMultiNeg25(void *pDataPtr)
{
  WorkerTestData *pData = (WorkerTestData *)pDataPtr;

  *pData->pInt = (*pData->pInt) * -25;

  udIncrementSemaphore(pData->pSema);
}

TEST(udWorkerPoolTests, Validate)
{
  udWorkerPool *pPool = nullptr;

  int value = 0;

  WorkerTestData data;
  data.pInt = &value;
  data.pSema = udCreateSemaphore();

  EXPECT_EQ(udR_Success, udWorkerPool_Create(&pPool, 1));
  EXPECT_NE(nullptr, pPool);

  EXPECT_EQ(udR_NothingToDo, udWorkerPool_DoPostWork(pPool)); // Nothing has been queued yet

  // 1 thread doing 2 tasks doesn't give race conditions
  EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, UpdateDataPlusOne, &data, false, UpdateDataMultiNeg25));
  EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, UpdateDataPlusOne, &data, false, UpdateDataMultiNeg25));

  // Should be hit twice
  udWaitSemaphore(data.pSema);
  udWaitSemaphore(data.pSema);

  while (udWorkerPool_HasActiveWorkers(pPool))
    continue;

  // 1+1
  EXPECT_EQ(2, value);

  EXPECT_EQ(udR_Success, udWorkerPool_DoPostWork(pPool)); // The work was exhausted

  while (udWaitSemaphore(data.pSema, 1) == 0)
    continue; // loop until the semaphore is empty

  // (1+1)*-25*-25 = 1250
  EXPECT_EQ(1250, value);

  // Queue other combinations
  EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, nullptr, &data, false));                       // Doesn't do anything
  EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, nullptr, &data, false, UpdateDataMultiNeg25)); // Multiply by 25 but don't do anything on a thread
  EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, UpdateDataPlusOne, &data, false));             // Only +1

  // Should be hit once
  udWaitSemaphore(data.pSema);

  // ((1+1)*-25*-25)+1
  EXPECT_EQ(1251, value);

  while (udWorkerPool_HasActiveWorkers(pPool))
    continue;

  EXPECT_EQ(udR_Success, udWorkerPool_DoPostWork(pPool, 1)); // Only handle 1 item

  // (((1+1)*-25*-25)+1)*-25
  EXPECT_EQ(-31275, value);

  const int TotalQueue = 100;
  const int PostLoops = 5;
  const int ItemsPerLoop = 20;

  for (int i = 0; i < TotalQueue; ++i)
  {
    WorkerTestData *pCopiedData = (WorkerTestData *)udMemDup(&data, sizeof(data), i, udAF_Zero);
    EXPECT_EQ(udR_Success, udWorkerPool_AddTask(pPool, UpdateDataPlusOne, pCopiedData, true, UpdateDataPlusOne));
  }

  while (udWorkerPool_HasActiveWorkers(pPool))
    continue;

  for (int i = 0; i < PostLoops; ++i)
  {
    EXPECT_EQ(udR_Success, udWorkerPool_DoPostWork(pPool, ItemsPerLoop));
  }

  EXPECT_EQ(udR_NothingToDo, udWorkerPool_DoPostWork(pPool));

  // ((((1+1)*-25*-25)+1)*-25 + TotalQueue*2)
  EXPECT_EQ((-31275 + TotalQueue * 2), value);

  udWorkerPool_Destroy(&pPool);
  udDestroySemaphore(&data.pSema);

  EXPECT_EQ(nullptr, pPool);

  // Additional destruction of non-existent objects
  udWorkerPool_Destroy(&pPool);
  udWorkerPool_Destroy(nullptr);
}
