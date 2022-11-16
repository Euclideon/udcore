#include "gtest/gtest.h"

#include "udDebug.h"

// Skip the prefix generated when multiple threads have accessed udDebugPrintf
static const char *SkipThreadPrefix(const char *pBuffer)
{
  if (pBuffer[0] >= '0' && pBuffer[0] <= '9' && pBuffer[1] >= '0' && pBuffer[1] <= '9' && pBuffer[2] == '>')
    return pBuffer + 3;
  return pBuffer;
}

TEST(udDebugTests, ShortDebugPrintf)
{
  gpudDebugPrintfOutputCallback = [](const char *pBuffer)
  { EXPECT_STRCASEEQ("This is a short udDebugPrintf\n", SkipThreadPrefix(pBuffer)); };
  udDebugPrintf("This is a short %s\n", "udDebugPrintf");
  gpudDebugPrintfOutputCallback = nullptr;
}

TEST(udDebugTests, LongDebugPrintf)
{
  gpudDebugPrintfOutputCallback = [](const char *pBuffer)
  { EXPECT_STRCASEEQ("This is a really really really really really really really really really really really really long udDebugPrintf\n", SkipThreadPrefix(pBuffer)); };
  udDebugPrintf("This is a really really really really really really really really really really really really long %s\n", "udDebugPrintf");
  gpudDebugPrintfOutputCallback = nullptr;
}
