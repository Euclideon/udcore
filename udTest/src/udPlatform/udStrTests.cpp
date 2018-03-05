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

TEST(udStrTests, udStrchr)
{
  const char input[] = "This is a special string, can you tell how?";
  EXPECT_NE(udStrchr(input, "s"), udStrrchr(input, "s"));
  EXPECT_STREQ(&input[3], udStrchr(input, "s"));
  EXPECT_EQ(&input[3], udStrchr(input, "s"));
  EXPECT_STREQ(&input[18], udStrrchr(input, "s"));
  EXPECT_EQ(&input[18], udStrrchr(input, "s"));
}

TEST(udStrTests, udStrstr)
{
  const char input[] = "That last string wasn't special at all, this one is special though.";
  EXPECT_STREQ(&input[24], udStrstr(input, UDARRAYSIZE(input), "special"));
  EXPECT_EQ(&input[24], udStrstr(input, UDARRAYSIZE(input), "special"));
}

TEST(udStrTests, udStrAto)
{
  // Positive
  EXPECT_FLOAT_EQ(52.5f, udStrAtof("52.5"));
  EXPECT_DOUBLE_EQ(52.5, udStrAtof64("52.5"));
  EXPECT_EQ(52, udStrAtoi("52"));
  EXPECT_EQ(52, udStrAtoi64("52"));
  EXPECT_EQ(52, udStrAtou("52"));
  EXPECT_EQ(52, udStrAtou64("52"));

  // Negative
  EXPECT_FLOAT_EQ(-52.5f, udStrAtof("-52.5"));
  EXPECT_DOUBLE_EQ(-52.5, udStrAtof64("-52.5"));
  EXPECT_EQ(-52, udStrAtoi("-52"));
  EXPECT_EQ(-52, udStrAtoi64("-52"));
  EXPECT_EQ(0, udStrAtou("-52"));
  EXPECT_EQ(0, udStrAtou64("-52"));

  // Radix-16 - Uppercase
  EXPECT_EQ(164, udStrAtoi("A4", nullptr, 16));
  EXPECT_EQ(163, udStrAtoi64("A3", nullptr, 16));
  EXPECT_EQ(162, udStrAtou("A2", nullptr, 16));
  EXPECT_EQ(161, udStrAtou64("A1", nullptr, 16));

  // Radix-16 - Lowercase
  EXPECT_EQ(164, udStrAtoi("a4", nullptr, 16));
  EXPECT_EQ(163, udStrAtoi64("a3", nullptr, 16));
  EXPECT_EQ(162, udStrAtou("a2", nullptr, 16));
  EXPECT_EQ(161, udStrAtou64("a1", nullptr, 16));
}

TEST(udStrTests, udStrtoa)
{
  char buffer[1024];

  // Positive
  EXPECT_EQ(2, udStrUtoa(buffer, UDARRAYSIZE(buffer), 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa(buffer, UDARRAYSIZE(buffer), 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa64(buffer, UDARRAYSIZE(buffer), 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(5, udStrFtoa(buffer, UDARRAYSIZE(buffer), 52.5, 2));
  EXPECT_STREQ("52.50", buffer);
  buffer[0] = '\0';

  // Negative
  EXPECT_EQ(20, udStrUtoa(buffer, UDARRAYSIZE(buffer), -52));
  EXPECT_STREQ("18446744073709551564", buffer); // This probably should generate 0
  EXPECT_EQ(3, udStrItoa(buffer, UDARRAYSIZE(buffer), -52));
  EXPECT_STREQ("-52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(3, udStrItoa64(buffer, UDARRAYSIZE(buffer), -52));
  EXPECT_STREQ("-52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(6, udStrFtoa(buffer, UDARRAYSIZE(buffer), -52.5, 2));
  EXPECT_STREQ("-52.50", buffer);
  buffer[0] = '\0';

  // Radix-16 - Uppercase
  EXPECT_EQ(2, udStrUtoa(buffer, UDARRAYSIZE(buffer), 164, -16));
  EXPECT_STREQ("A4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(0, udStrItoa(buffer, UDARRAYSIZE(buffer), 164, -16));
  EXPECT_STREQ("", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(0, udStrItoa64(buffer, UDARRAYSIZE(buffer), 164, -16));
  EXPECT_STREQ("", buffer);
  buffer[0] = '\0';

  // Radix-16 - Lowercase
  EXPECT_EQ(2, udStrUtoa(buffer, UDARRAYSIZE(buffer), 164, 16));
  EXPECT_STREQ("a4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa(buffer, UDARRAYSIZE(buffer), 164, 16));
  EXPECT_STREQ("a4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa64(buffer, UDARRAYSIZE(buffer), 164, 16));
  EXPECT_STREQ("a4", buffer);
  buffer[0] = '\0';
}

TEST(udStrTests, udStrStringWhitespace)
{
  char input[] = "  \n\t\r\n\r\t Will Thou Strip All Whitespace? \t\r\n\r\t\n  ";
  const char output[] = "WillThouStripAllWhitespace?";
  EXPECT_EQ(UDARRAYSIZE(output), udStrStripWhiteSpace(input));
  EXPECT_STREQ(output, input);
}

TEST(udStrTests, udAddToStringTable)
{
  char *pStringTable = nullptr;
  uint32_t stringTableLength = 0;
  EXPECT_EQ(0, udAddToStringTable(pStringTable, &stringTableLength, "StringOne"));
  EXPECT_EQ(0, memcmp("StringOne", pStringTable, UDARRAYSIZE("StringOne")));
  EXPECT_EQ(10, udAddToStringTable(pStringTable, &stringTableLength, "StringTwo"));
  EXPECT_EQ(0, memcmp("StringOne\0StringTwo", pStringTable, UDARRAYSIZE("StringOne\0StringTwo")));
  EXPECT_EQ(20, udAddToStringTable(pStringTable, &stringTableLength, "StringThree"));
  EXPECT_EQ(0, memcmp("StringOne\0StringTwo\0StringThree", pStringTable, UDARRAYSIZE("StringOne\0StringTwo\0StringThree")));
  udFree(pStringTable);
}
