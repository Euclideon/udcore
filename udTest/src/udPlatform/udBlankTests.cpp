#include "gtest/gtest.h"

//THIS FILE IS JUST A BLANK FILE TO COPY FOR OTHER TEST FILES

TEST(BlankTests, Validate)
{
  const float realFloatValue = 0.75f;
  const char *realStringValue = "good gTest";
  const int realHexValue = 0x1337;
  const int realIntValue = -88;

  float copyFloatValue = realFloatValue;
  const char *copyStringValue = realStringValue;
  int copyHexValue = realHexValue;
  int copyIntValue = realIntValue;
  void *pNullPtr = nullptr;

  //Compare the values
  EXPECT_FLOAT_EQ(realFloatValue, copyFloatValue);
  EXPECT_STRCASEEQ(realStringValue, copyStringValue);
  EXPECT_EQ(realHexValue, copyHexValue);
  EXPECT_EQ(realIntValue, copyIntValue);
  EXPECT_NE((void*)-1, pNullPtr);
}
