#include "gtest/gtest.h"
#include "udPlatform.h"
#include "udPlatformUtil.h"

#define GENERATE_EXPECTED 0

#define GETSIGN(x) (((x) < 0) ? -1 : ((x) > 0) ? 1 : 0)

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udStrTests, udStrncmp)
{
  static const char *strings[] =
  {
    "", "abc", "ABC", "123", "Hello", "[symbols%!]"
  };
  static int expectedStrcmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =
  {
    { 0, -1, -1, -1, -1, -1 },
    { 1, 0, 1, 1, 1, 1 },
    { 1, -1, 0, 1, -1, -1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, -1, 1, 1, 0, -1 },
    { 1, -1, 1, 1, 1, 0 },
  };
  static int expectedStrcmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =
  {
    { 0, -1, -1, -1, -1, -1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, 1, 1, 1, 0, 1 },
    { 1, -1, -1, 1, -1, 0 },
  };
  static int expectedStrncmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =
  {
    { 0, 0, 0, 0, 0, 0 },
    { 1, 0, 1, 1, 1, 1 },
    { 1, -1, 0, 1, -1, -1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, -1, 1, 1, 0, -1 },
    { 1, -1, 1, 1, 1, 0 },
  };
  static int expectedStrncmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =
  {
    { 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, 1, 1, 1, 0, 1 },
    { 1, -1, -1, 1, -1, 0 },
  };

  for (int i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    for (int j = 0; j < UDARRAYSIZE(strings); ++j)
    {
#if GENERATE_EXPECTED
      int vs = GETSIGN(strcmp(strings[i], strings[j]));
      int vi = GETSIGN(_strcmpi(strings[i], strings[j]));
      int vn = GETSIGN(strncmp(strings[i], strings[j], i));
      int vni = GETSIGN(_strnicmp(strings[i], strings[j], i));
      expectedStrcmp[i][j] = vs;
      expectedStrcmpi[i][j] = vi;
      expectedStrncmp[i][j] = vn;
      expectedStrncmpi[i][j] = vni;
#else
      int vs = GETSIGN(udStrcmp(strings[i], strings[j]));
      int vi = GETSIGN(udStrcmpi(strings[i], strings[j]));
      int vn = GETSIGN(udStrncmp(strings[i], strings[j], i));
      int vni = GETSIGN(udStrncmpi(strings[i], strings[j], i));
      EXPECT_EQ(expectedStrcmp[i][j], vs);
      EXPECT_EQ(expectedStrcmpi[i][j], vi);
      EXPECT_EQ(expectedStrncmp[i][j], vn);
      EXPECT_EQ(expectedStrncmpi[i][j], vni);
#endif
    }
  }

#if GENERATE_EXPECTED
  udDebugPrintf("static int expectedStrcmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =\n{\n");
  for (int i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    udDebugPrintf("  { %d, %d, %d, %d, %d, %d },\n", expectedStrcmp[i][0], expectedStrcmp[i][1], expectedStrcmp[i][2], expectedStrcmp[i][3], expectedStrcmp[i][4], expectedStrcmp[i][5]);
  }
  udDebugPrintf("};\n");

  udDebugPrintf("static int expectedStrcmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =\n{\n");
  for (int i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    udDebugPrintf("  { %d, %d, %d, %d, %d, %d },\n", expectedStrcmpi[i][0], expectedStrcmpi[i][1], expectedStrcmpi[i][2], expectedStrcmpi[i][3], expectedStrcmpi[i][4], expectedStrcmpi[i][5]);
  }
  udDebugPrintf("};\n");

  udDebugPrintf("static int expectedStrncmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =\n{\n");
  for (int i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    udDebugPrintf("  { %d, %d, %d, %d, %d, %d },\n", expectedStrncmp[i][0], expectedStrncmp[i][1], expectedStrncmp[i][2], expectedStrncmp[i][3], expectedStrncmp[i][4], expectedStrncmp[i][5]);
  }
  udDebugPrintf("};\n");

  udDebugPrintf("static int expectedStrncmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] =\n{\n");
  for (int i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    udDebugPrintf("  { %d, %d, %d, %d, %d, %d },\n", expectedStrncmpi[i][0], expectedStrncmpi[i][1], expectedStrncmpi[i][2], expectedStrncmpi[i][3], expectedStrncmpi[i][4], expectedStrncmpi[i][5]);
  }
  udDebugPrintf("};\n");
  udDebugPrintf("\n");
#endif
}
