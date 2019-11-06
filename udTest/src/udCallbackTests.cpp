#include "gtest/gtest.h"

#include "udCallback.h"

TEST(udCallbackTests, Validation)
{
  using TestCallback = udCallback<int(int)>;

  TestCallback basic = [](int a) -> int { return a * 2; };
  EXPECT_EQ(4, basic(2));

  int a = 3;
  TestCallback valCapture = [a](int b) -> int { return a * b; };
  EXPECT_EQ(6, valCapture(2));

  a = 4;
  TestCallback refCapture = [&a](int b) -> int { return a * b; };
  EXPECT_EQ(6, valCapture(2));
  EXPECT_EQ(8, refCapture(2));

  a = 5;
  TestCallback allRefCapture = [&](int b) -> int { return basic(a) * basic(b); };
  EXPECT_EQ(6, valCapture(2));
  EXPECT_EQ(10, refCapture(2)); // Note a was captured by reference, this will now produce a different result
  EXPECT_EQ(40, allRefCapture(2));
}
