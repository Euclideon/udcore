#include "gtest/gtest.h"
#include "udPlatform.h"
#include "udStringUtil.h"

// ----------------------------------------------------------------------------
// Author: Paul Fox, January 2019
TEST(udMemoryTests, Validate)
{
  char *pCharMemory = udAllocType(char, 200, udAF_Zero);
  EXPECT_NE(nullptr, pCharMemory);
  for (size_t i = 0; i < 200; ++i)
    EXPECT_EQ('\0', pCharMemory[i]);
  udFree(pCharMemory);
  EXPECT_EQ(nullptr, pCharMemory);

  // udMemdup test
  const char numbers[] = "1,2,3,4,5,6,7,8,9";
  const char expectedNumbers[] = { '1', ',', '2', ',', '3', ',', '4', ',', '5', ',', '6', ',', '7', ',', '8', ',', '9', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
  void *pNumbersMemory = udMemDup(numbers, sizeof(numbers)-1, 10, udAF_Zero); //-1 to strip the \0
  EXPECT_EQ(0, memcmp(pNumbersMemory, expectedNumbers, sizeof(expectedNumbers)));
  udFree(pNumbersMemory);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2019
TEST(udMemoryTests, GetTotalPhysicalMemory)
{
  uint64_t mem;
  udResult result;

  result = udGetTotalPhysicalMemory(&mem);
  printf("Total memory reported: %sMB (%s)\n", udTempStr_CommaInt(mem / 1048576), udResultAsString(result));
  if (result == udR_Success)
    EXPECT_NE(0, mem);
  else
    EXPECT_EQ(0, mem);
}
