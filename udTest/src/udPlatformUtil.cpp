#include "gtest/gtest.h"
#include "udPlatformUtil.h"

TEST(udPlatformUtilTests, udTime)
{
  constexpr const char time[] = "2023-08-30T12:34:56.789Z";
  double epoch = udTime_StringToEpoch(time);
  EXPECT_EQ(1693398896.789, epoch);

  char buffer[50]{};
  EXPECT_EQ(udR_Success, udTime_EpochToString(buffer, udLengthOf(buffer), epoch));
  EXPECT_STREQ(time, buffer);
}
