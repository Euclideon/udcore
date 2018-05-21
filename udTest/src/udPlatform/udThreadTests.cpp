#include "gtest/gtest.h"

#include "udThread.h"

TEST(udThreadTests, Mutex)
{
  udMutex *pMutex = udCreateMutex();
  EXPECT_NE(nullptr, pMutex);
  udDestroyMutex(&pMutex);
}

TEST(udThreadTests, ConditionVariable)
{
  udConditionVariable *pCondition = udCreateConditionVariable();
  EXPECT_NE(nullptr, pCondition);
  udDestroyConditionVariable(&pCondition);
}

TEST(udThreadTests, Semaphore)
{
  udSemaphore *pSemaphore = udCreateSemaphore();
  EXPECT_NE(nullptr, pSemaphore);
  udDestroySemaphore(&pSemaphore);
}

TEST(udThreadTests, Thread)
{
  int value = 0;
  udThread *pThread;
  udThreadStart *pStartFunc = [](void *data) -> unsigned int { int *pValue = (int*)data; (*pValue) = 1; return 0; };
  udResult result = udThread_Create(&pThread, pStartFunc, (void*)&value);

  EXPECT_EQ(udR_Success, result);

  result = udThread_Join(pThread);

  EXPECT_EQ(udR_Success, result);
  EXPECT_EQ(1, value);

  udThread_Destroy(&pThread);
}

TEST(udThreadTests, ThreadConditionVariable)
{
  struct TestStruct
  {
    udMutex *pMutex;
    udConditionVariable *pConditionVariable;
  };

  TestStruct data;
  data.pMutex = udCreateMutex();
  data.pConditionVariable = udCreateConditionVariable();

  udThread *pThread;
  udThreadStart *pStartFunc = [](void *data) -> unsigned int {
    TestStruct *pData = (TestStruct*)data;
    udSleep(100);

    udLockMutex(pData->pMutex);
    udSignalConditionVariable(pData->pConditionVariable, 1);
    udReleaseMutex(pData->pMutex);

    return 0;
  };

  udResult result = udThread_Create(&pThread, pStartFunc, (void*)&data);
  EXPECT_EQ(udR_Success, result);

  udLockMutex(data.pMutex);
  EXPECT_NE(0, udWaitConditionVariable(data.pConditionVariable, data.pMutex, 1));
  EXPECT_EQ(0, udWaitConditionVariable(data.pConditionVariable, data.pMutex, 1000));
  udReleaseMutex(data.pMutex);

  result = udThread_Join(pThread);

  EXPECT_EQ(udR_Success, result);

  udThread_Destroy(&pThread);

  udDestroyMutex(&data.pMutex);
  udDestroyConditionVariable(&data.pConditionVariable);
}

TEST(udThreadTests, ThreadSemaphore)
{
  //gpudDebugPrintfOutputCallback = [](const char *pBuffer) { printf(pBuffer); };

  struct TestStruct
  {
    udSemaphore *pSemaphore;
    udSemaphore *pModified;
    udSemaphore *p500;
    volatile int value;
  };
  udThread *pThread1, *pThread2, *pThread3;
  TestStruct data;
  data.pSemaphore = udCreateSemaphore();
  data.pModified = udCreateSemaphore();
  data.p500 = udCreateSemaphore();
  data.value = 0;

  udThreadStart *pStartFunc1 = [](void *data) -> unsigned int {
    TestStruct *pData = (TestStruct*)data;
    bool running = true;
    while (running)
    {
      //printf("1: pData->pSemaphore wait: %d\n", udWaitSemaphore(pData->pSemaphore));
      udWaitSemaphore(pData->pSemaphore);
      pData->value++;
      if (pData->value > 1000)
        running = false;

      if (pData->value == 500)
        udIncrementSemaphore(pData->p500);

      udIncrementSemaphore(pData->pModified);
    }
    return 0;
  };
  udResult result = udThread_Create(&pThread1, pStartFunc1, (void*)&data);
  EXPECT_EQ(udR_Success, result);

  udThreadStart *pStartFunc2 = [](void *data) -> unsigned int {
    TestStruct *pData = (TestStruct*)data;
    bool running = true;
    while (running)
    {
      //printf("2: pData->pSemaphore wait: %d\n", udWaitSemaphore(pData->pSemaphore));
      udWaitSemaphore(pData->pSemaphore);
      pData->value++;
      if (pData->value > 2000)
        running = false;

      if (pData->value == 500)
        udIncrementSemaphore(pData->p500);

      udIncrementSemaphore(pData->pModified);
    }
    return 0;
  };
  result = udThread_Create(&pThread2, pStartFunc2, (void*)&data);
  EXPECT_EQ(udR_Success, result);

  udThreadStart *pStartFunc3 = [](void *data) -> unsigned int {
    TestStruct *pData = (TestStruct*)data;

    udWaitSemaphore(pData->p500);
    EXPECT_GE(pData->value, 500);

    bool running = true;
    while (running)
    {
      //printf("3: pData->pSemaphore wait: %d\n", udWaitSemaphore(pData->pSemaphore));
      udWaitSemaphore(pData->pSemaphore);
      pData->value++;
      if (pData->value > 3000)
        running = false;

      udIncrementSemaphore(pData->pModified);
    }
    return 0;
  };
  result = udThread_Create(&pThread3, pStartFunc3, (void*)&data);
  EXPECT_EQ(udR_Success, result);

  for (int i = 0; i < 3000; i++)
  {
    EXPECT_EQ(i, data.value);
    udIncrementSemaphore(data.pSemaphore);
    //printf("0: data.pModified wait: %d\n", udWaitSemaphore(data.pModified));
    udWaitSemaphore(data.pModified);
  }

  // Ensure the three threads finish
  for (int i = 0; i < 3; i++)
    udIncrementSemaphore(data.pSemaphore);

  EXPECT_GE(data.value, 3000);

  result = udThread_Join(pThread1);
  EXPECT_EQ(udR_Success, result);
  result = udThread_Join(pThread2);
  EXPECT_EQ(udR_Success, result);
  result = udThread_Join(pThread3);
  EXPECT_EQ(udR_Success, result);

  udDestroySemaphore(&data.pSemaphore);
  udDestroySemaphore(&data.pModified);
  udDestroySemaphore(&data.p500);

  udThread_Destroy(&pThread1);
  udThread_Destroy(&pThread2);
  udThread_Destroy(&pThread3);
}

TEST(udThreadTests, TimeSemaphore)
{
  udSemaphore *pSemaphore = udCreateSemaphore();
  udThread *pThread;
  udThread_Create(&pThread, [](void *pData) -> unsigned int { udSemaphore *pSemaphore = (udSemaphore*)pData; EXPECT_EQ(0, udWaitSemaphore(pSemaphore, 5000)); return 0; }, pSemaphore);

  EXPECT_NE(0, udWaitSemaphore(pSemaphore, 200));

  udIncrementSemaphore(pSemaphore);

  udThread_Join(pThread);
  udThread_Destroy(&pThread);

  udDestroySemaphore(&pSemaphore);
}

TEST(udThreadTests, DeletedSemaphore)
{
  // Need this so that the pointer is also zeroed in the thread
  // this is closer to the expected use-case when deleting an
  // object that's being used everywhere.
  struct TestStruct
  {
    udSemaphore *pSemaphore;
  };

  TestStruct data;
  data.pSemaphore = udCreateSemaphore();

  udThread *pThread;
  udThread_Create(&pThread, [](void *pData) -> unsigned int { TestStruct *pTestData = (TestStruct*)pData; EXPECT_NE(0, udWaitSemaphore(pTestData->pSemaphore)); return 0; }, &data);

  // Sleep for 2 seconds
  udSleep(2000);

  udDestroySemaphore(&data.pSemaphore);

  udThread_Join(pThread);
  udThread_Destroy(&pThread);
}

TEST(udThreadTests, MultipleIncrements)
{
  udSemaphore *pSemaphore = udCreateSemaphore();
  EXPECT_NE(nullptr, pSemaphore);
  udIncrementSemaphore(pSemaphore, 10);
  EXPECT_EQ(0, udWaitSemaphore(pSemaphore));
  udDestroySemaphore(&pSemaphore);
}
