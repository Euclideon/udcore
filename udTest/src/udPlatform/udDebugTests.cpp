#include "gtest/gtest.h"

#include "udDebug.h"

TEST(udDebugTests, ShortDebugPrintf)
{
  gpudDebugPrintfOutputCallback = [](const char *pBuffer) { EXPECT_STRCASEEQ("This is a short udDebugPrintf\n", pBuffer); };
  udDebugPrintf("This is a short %s\n", "udDebugPrintf");
}

TEST(udDebugTests, LongDebugPrintf)
{
  gpudDebugPrintfOutputCallback = [](const char *pBuffer) { EXPECT_STRCASEEQ("This is a really really really really really really really really really really really really long udDebugPrintf\n", pBuffer); };
  udDebugPrintf("This is a really really really really really really really really really really really really long %s\n", "udDebugPrintf");
}

