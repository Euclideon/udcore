#include "gtest/gtest.h"

#include "udAsyncJob.h"

udResult udAsyncJobTestsFuncOld(int a, udAsyncJob *pAsyncJob)
{
  UDASYNC_CALL1(udAsyncJobTestsFuncOld, int, a);

  return udR_Count;
}

udResult udAsyncJobTestsFuncNew(int a, udAsyncJob *pAsyncJob)
{
  UDASYNC_CALL(udAsyncJobTestsFuncNew(a, nullptr));

  return udR_Count;
}

TEST(udAsyncJobTests, Validation)
{
  udAsyncJob *pAsyncJob = nullptr;
  EXPECT_EQ(udR_Count, udAsyncJobTestsFuncOld(2, pAsyncJob));
  EXPECT_EQ(udR_Count, udAsyncJobTestsFuncNew(2, pAsyncJob));

  udAsyncJob_Create(&pAsyncJob);

  EXPECT_EQ(udR_Success, udAsyncJobTestsFuncOld(2, pAsyncJob));
  EXPECT_EQ(udR_Count, udAsyncJob_GetResult(pAsyncJob));

  EXPECT_EQ(udR_Success, udAsyncJobTestsFuncNew(2, pAsyncJob));
  EXPECT_EQ(udR_Count, udAsyncJob_GetResult(pAsyncJob));

  udAsyncJob_Destroy(&pAsyncJob);
}
