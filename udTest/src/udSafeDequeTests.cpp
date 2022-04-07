#include "gtest/gtest.h"
#include "udSafeDeque.h"

bool largerThan(int a, int b)
{
  return a > b;
}

bool equivalent(int a, int b)
{
  return a == b;
}

TEST(udSafeDequeTests, ValidationTests)
{
  udSafeDeque<int> *pQueue = nullptr;
  int result = -1;

  EXPECT_EQ(udR_Success, udSafeDeque_Create(&pQueue, 32));
  ASSERT_NE(nullptr, pQueue);

  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 5));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 3));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 9));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, -1));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 27));

  // Queue should be { 27, 3, 5, 9, -1 }

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(-1, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopFront(pQueue, &result));
  EXPECT_EQ(27, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopFront(pQueue, &result));
  EXPECT_EQ(3, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopFront(pQueue, &result));
  EXPECT_EQ(5, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopFront(pQueue, &result));
  EXPECT_EQ(9, result);

  EXPECT_EQ(udR_NotFound, udSafeDeque_PopFront(pQueue, &result));
  EXPECT_EQ(udR_NotFound, udSafeDeque_PopBack(pQueue, &result));

  EXPECT_EQ(9, result); // This should not have changed



  udSafeDeque_Destroy(&pQueue);
  EXPECT_EQ(nullptr, pQueue);

  // Additional destruction of non-existent objects
  udSafeDeque_Destroy(&pQueue);
  udSafeDeque_Destroy((udSafeDeque<int> **)nullptr);
}


TEST(udSafeDequeTests, SortTest)
{
  udSafeDeque<int> *pQueue = nullptr;
  int result = -1;

  EXPECT_EQ(udR_Success, udSafeDeque_Create(&pQueue, 32));
  ASSERT_NE(nullptr, pQueue);

  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 5));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 3));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 9));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, -1));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 27));
  // Queue should be { 27, 3, 5, 9, -1 }
  udSafeDeque_SortSS(pQueue, largerThan);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(27, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(9, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(5, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(3, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(-1, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 5));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 3));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 9));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, -1));
  EXPECT_EQ(udR_Success, udSafeDeque_PushFront(pQueue, 27));
  EXPECT_EQ(udR_Success, udSafeDeque_PushBack(pQueue, 9));
  // Queue should be { 27, 3, 5, 9, -1 , 9}
  //sort using equivalcence defined:
  udSafeDeque_SortSS(pQueue, largerThan, equivalent);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(27, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(9, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(9, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(5, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(3, result);

  EXPECT_EQ(udR_Success, udSafeDeque_PopBack(pQueue, &result));
  EXPECT_EQ(-1, result);

  udSafeDeque_Destroy(&pQueue);
}
