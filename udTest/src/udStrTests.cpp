#include "udPlatform.h"
#include "udStringUtil.h"
#include "gtest/gtest.h"

#define GENERATE_EXPECTED 0

#define GETSIGN(x) (((x) < 0) ? -1 : ((x) > 0) ? 1 \
                                               : 0)

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udStrTests, udStrncmp)
{
  static const char *strings[] = {
    "", "abc", "ABC", "123", "Hello", "[symbols%!]"
  };
  static int expectedStrcmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] = {
    { 0, -1, -1, -1, -1, -1 },
    { 1, 0, 1, 1, 1, 1 },
    { 1, -1, 0, 1, -1, -1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, -1, 1, 1, 0, -1 },
    { 1, -1, 1, 1, 1, 0 },
  };
  static int expectedStrcmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] = {
    { 0, -1, -1, -1, -1, -1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, 1, 1, 1, 0, 1 },
    { 1, -1, -1, 1, -1, 0 },
  };
  static int expectedStrncmp[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] = {
    { 0, 0, 0, 0, 0, 0 },
    { 1, 0, 1, 1, 1, 1 },
    { 1, -1, 0, 1, -1, -1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, -1, 1, 1, 0, -1 },
    { 1, -1, 1, 1, 1, 0 },
  };
  static int expectedStrncmpi[UDARRAYSIZE(strings)][UDARRAYSIZE(strings)] = {
    { 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, 0, 0, 1, -1, 1 },
    { 1, -1, -1, 0, -1, -1 },
    { 1, 1, 1, 1, 0, 1 },
    { 1, -1, -1, 1, -1, 0 },
  };

  for (size_t i = 0; i < UDARRAYSIZE(strings); ++i)
  {
    for (size_t j = 0; j < UDARRAYSIZE(strings); ++j)
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
  EXPECT_NE(udStrchr(input, "t"), udStrchri(input, "t"));
  EXPECT_EQ(udStrchr(input, "T"), udStrchri(input, "T"));
  EXPECT_EQ(udStrrchr(input, "t"), udStrrchri(input, "t"));
  EXPECT_NE(udStrrchr(input, "T"), udStrrchri(input, "T"));
}

TEST(udStrTests, udStrstr)
{
  const char input[] = "That last string wasn't special at all, this one is special though.";
  EXPECT_STREQ(&input[24], udStrstr(input, UDARRAYSIZE(input), "special"));
  EXPECT_EQ(&input[24], udStrstr(input, UDARRAYSIZE(input), "special"));
  EXPECT_STREQ(input, udStrstri(input, udLengthOf(input), "th"));
  EXPECT_STREQ(input, udStrstri(input, udLengthOf(input), "TH"));

  // Special cases
  const char specialInput[] = "]]]]>";
  // Partial match overlapping with actual match
  EXPECT_STREQ(&specialInput[1], udStrstr(specialInput, udLengthOf(specialInput), "]]]>"));
  // Partial match immediately before actual match
  EXPECT_STREQ(&specialInput[2], udStrstr(specialInput, udLengthOf(specialInput), "]]>"));
}

TEST(udStrTests, udStrAto)
{
  // Positive
  EXPECT_FLOAT_EQ(52.5f, udStrAtof("52.5"));
  EXPECT_DOUBLE_EQ(52.5, udStrAtof64("52.5"));
  EXPECT_EQ(52, udStrAtoi("52"));
  EXPECT_EQ(52, udStrAtoi64("52"));
  EXPECT_EQ(52U, udStrAtou("52"));
  EXPECT_EQ(52U, udStrAtou64("52"));

  // Negative
  EXPECT_FLOAT_EQ(-52.5f, udStrAtof("-52.5"));
  EXPECT_DOUBLE_EQ(-52.5, udStrAtof64("-52.5"));
  EXPECT_EQ(-52, udStrAtoi("-52"));
  EXPECT_EQ(-52, udStrAtoi64("-52"));
  EXPECT_EQ(0U, udStrAtou("-52"));
  EXPECT_EQ(0U, udStrAtou64("-52"));

  // Radix-16 - Uppercase
  EXPECT_EQ(164, udStrAtoi("A4", nullptr, 16));
  EXPECT_EQ(163, udStrAtoi64("A3", nullptr, 16));
  EXPECT_EQ(162U, udStrAtou("A2", nullptr, 16));
  EXPECT_EQ(161U, udStrAtou64("A1", nullptr, 16));

  // Radix-16 - Lowercase
  EXPECT_EQ(164, udStrAtoi("a4", nullptr, 16));
  EXPECT_EQ(163, udStrAtoi64("a3", nullptr, 16));
  EXPECT_EQ(162U, udStrAtou("a2", nullptr, 16));
  EXPECT_EQ(161U, udStrAtou64("a1", nullptr, 16));
}

TEST(udStrTests, udStrtoa)
{
  char buffer[1024];

  // Positive
  EXPECT_EQ(2, udStrUtoa(buffer, 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa(buffer, 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa64(buffer, 52));
  EXPECT_STREQ("52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(5, udStrFtoa(buffer, 52.5, 2));
  EXPECT_STREQ("52.50", buffer);
  buffer[0] = '\0';

  // Negative
  EXPECT_EQ(20, udStrUtoa(buffer, (uint64_t)-52));
  EXPECT_STREQ("18446744073709551564", buffer);
  EXPECT_EQ(3, udStrItoa(buffer, -52));
  EXPECT_STREQ("-52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(3, udStrItoa64(buffer, -52));
  EXPECT_STREQ("-52", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(6, udStrFtoa(buffer, -52.5, 2));
  EXPECT_STREQ("-52.50", buffer);
  buffer[0] = '\0';

  // Radix-16 - Uppercase
  EXPECT_EQ(2, udStrUtoa(buffer, 164, -16));
  EXPECT_STREQ("A4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(0, udStrItoa(buffer, 164, -16));
  EXPECT_STREQ("", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(0, udStrItoa64(buffer, 164, -16));
  EXPECT_STREQ("", buffer);
  buffer[0] = '\0';

  // Radix-16 - Lowercase
  EXPECT_EQ(2, udStrUtoa(buffer, 164, 16));
  EXPECT_STREQ("a4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa(buffer, 164, 16));
  EXPECT_STREQ("a4", buffer);
  buffer[0] = '\0';
  EXPECT_EQ(2, udStrItoa64(buffer, 164, 16));
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

static const char *s_pTestParagraph =
    "Today, historians relate that, as a general rule, buying and selling securities was very much unorganized before the year 1792. "
    "Every person who owned a security faced the problem of finding interested buyers who might consider the purchase of a debt - free investment. "
    "This meant most people were somewhat slow in investing in stocks and bonds because these securities could not readily be converted into money. "
    "We have been told that an interesting number of traders and merchants agreed to try to do something to help correct the situation. "
    "At this first crucial meeting, they decided that it was a good idea to visit regularly on a daily basis to buy and sell securities. "
    "The group of leaders, whose meeting place was under an old, tall cottonwood tree, found the needed time to plot the financial future of our nation. ";

TEST(udStrTests, udTempStr)
{
  EXPECT_STREQ("2.01m", udTempStr_HumanMeasurement(2.01));
  EXPECT_STREQ("5.5cm", udTempStr_HumanMeasurement(0.055));
  EXPECT_STREQ("3mm", udTempStr_HumanMeasurement(0.003));
  EXPECT_STREQ("3.2mm", udTempStr_HumanMeasurement(0.0032));
  EXPECT_STREQ("01:23", udTempStr_ElapsedTime(60 + 23));
  EXPECT_STREQ("0:01:23", udTempStr_ElapsedTime(60 + 23, false));
  EXPECT_STREQ("2:01:23", udTempStr_ElapsedTime(120 * 60 + 60 + 23, false));

  EXPECT_STREQ("0", udTempStr_TrimDouble(0, 10, 0));
  EXPECT_STREQ("0.0", udTempStr_TrimDouble(0, 10, 1));
  EXPECT_STREQ("0.00", udTempStr_TrimDouble(0, 10, 2));
  EXPECT_STREQ("0.0000000000", udTempStr_TrimDouble(0, 10, 12));
  EXPECT_STREQ("10.6667", udTempStr_TrimDouble(10.666666667, 4, 0));
  EXPECT_STREQ("10.6666", udTempStr_TrimDouble(10.666666667, 4, 0, true));
  EXPECT_STREQ("123", udTempStr_TrimDouble(123.666666667, 0, 0, true));
  EXPECT_STREQ("124", udTempStr_TrimDouble(123.666666667, 0, 0, false));

  // These tests assume 32 buffers of 64 characters each
  const char *pBuffers[32];
  const int bufferLen = 64;
  size_t index;

  // Make sure we can generate all the temp strings without overwriting
  for (index = 0; index < udLengthOf(pBuffers); ++index)
    pBuffers[index] = udTempStr("%d", (int)index);
  for (index = 0; index < udLengthOf(pBuffers); ++index)
    EXPECT_EQ(index, udStrAtoi(pBuffers[index]));

  // Test two strings longer than a buffer size don't overwrite each other
  const char *pMediumString1 = udTempStr("%*s", 3 * bufferLen - 2, "@");
  const char *pMediumString2 = udTempStr("%*s", 3 * bufferLen - 2, "*");
  udStrchr(pMediumString1, "@", &index);
  EXPECT_EQ(3 * bufferLen - 3, index);
  udStrchr(pMediumString2, "*", &index);
  EXPECT_EQ(3 * bufferLen - 3, index);

  // Test a single string the size of all the buffers (force a wrap)
  const char *pLongString = udTempStr("%*s", int(udLengthOf(pBuffers)) * bufferLen - 2, "!");
  udStrchr(pLongString, "!", &index);
  EXPECT_EQ(32 * bufferLen - 3, index);
  // Make sure it is the lowest pointer of all of them
  for (index = 0; index < udLengthOf(pBuffers); ++index)
    EXPECT_TRUE(pLongString <= pBuffers[index]);

  // Test a string longer than all the buffers
  pLongString = udTempStr("%*s", int(udLengthOf(pBuffers) + 1) * bufferLen, "!");
  EXPECT_EQ(nullptr, udStrchr(pLongString, "!"));

  // Test strings of varying lengths
  for (size_t i = 0; i < udStrlen(s_pTestParagraph); ++i)
  {
    const char *pStr = udTempStr("%.*s", (int)i, s_pTestParagraph);
    const char *pStr2 = udTempStr("%d", (int)i); // Cause the following buffer to be overwritten
    EXPECT_EQ(i, udStrlen(pStr));
    EXPECT_EQ(0, memcmp(pStr, s_pTestParagraph, i));
    EXPECT_EQ(i, udStrAtoi(pStr2));
  }
}
