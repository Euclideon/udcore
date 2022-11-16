#include "gtest/gtest.h"

#include "udCallback.h"

TEST(udCallbackTests, Validation)
{
  using TestCallback = udCallback<int(int)>;

  TestCallback basic = [](int a) -> int
  { return a * 2; };
  EXPECT_TRUE(basic);
  EXPECT_EQ(4, basic(2));

  int a = 3;
  TestCallback valCapture = [a](int b) -> int
  { return a * b; };
  EXPECT_EQ(6, valCapture(2));

  a = 4;
  TestCallback refCapture = [&a](int b) -> int
  { return a * b; };
  EXPECT_EQ(6, valCapture(2));
  EXPECT_EQ(8, refCapture(2));

  a = 5;
  TestCallback allRefCapture = [&](int b) -> int
  { return basic(a) * basic(b); };
  EXPECT_EQ(6, valCapture(2));
  EXPECT_EQ(10, refCapture(2)); // Note a was captured by reference, this will now produce a different result
  EXPECT_EQ(40, allRefCapture(2));

  TestCallback defaultFunc;
  EXPECT_FALSE(defaultFunc);

  TestCallback nullFunc = nullptr;
  EXPECT_FALSE(nullFunc);
  EXPECT_TRUE(nullFunc == nullptr);  // Test == operator
  EXPECT_FALSE(nullFunc != nullptr); // Test != operator

  TestCallback nullAssignFunc = [](int a) -> int
  { return a * 2; };
  EXPECT_TRUE(nullAssignFunc);
  EXPECT_FALSE(nullAssignFunc == nullptr); // Test == operator
  EXPECT_TRUE(nullAssignFunc != nullptr);  // Test != operator
  nullAssignFunc = nullptr;
  EXPECT_FALSE(nullAssignFunc);
  EXPECT_TRUE(nullAssignFunc == nullptr);  // Test == operator
  EXPECT_FALSE(nullAssignFunc != nullptr); // Test != operator

  // Test copy constructor
  TestCallback copyFunc = basic;
  EXPECT_EQ(basic(2), copyFunc(2));

  // Test move constructors and assignments
  TestCallback moveFunc = std::move(copyFunc); // Test move constructor
  EXPECT_EQ(basic(3), moveFunc(3));
  copyFunc = basic;               // Test copy assignment
  moveFunc = std::move(copyFunc); // Test move assignment
  EXPECT_EQ(basic(4), moveFunc(4));
  moveFunc = [](int a) -> int
  { return a; }; // Test direct assignment
  EXPECT_EQ(5, moveFunc(a));
}
