#include "gtest/gtest.h"
#include "udValue.h"

// First-pass most basic tests for udValue
// TODO: Fix udMemoryDebugTracking to be useful and test memory leaks

static const char *pJSONTest = "{\"Settings\":{\"ProjectsPath\":\"C:\\\\Temp&\\\\\",\"ImportAtFullScale\":true,\"TerrainIndex\":2,\"Inside\":{\"Count\":5},\"Outside\":{\"Count\":2,\"content\":\"windy\"},\"EmptyArray\":[],\"Nothing\":null,\"SpecialChars\":\"<>&\\\\/?[]{}'\\\"%\",\"TestArray\":[0,1,2]}}";
static const char *pXMLTest = "<Settings ProjectsPath=\"C:\\Temp&amp;\\\" ImportAtFullScale=\"true\" TerrainIndex=\"2\" SpecialChars=\"&lt;&gt;&amp;\\/?[]{}'&quot;%\"><Inside Count=\"5\"/><Outside Count=\"2\">windy</Outside><EmptyArray></EmptyArray><Nothing/><TestArray>0</TestArray><TestArray>1</TestArray><TestArray>2</TestArray></Settings>";

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
static void udValue_TestContent(udValue &v)
{
  udValue *pTemp = nullptr;
  EXPECT_EQ(true, udStrEqual(v.Get("Settings.ProjectsPath").AsString(""), "C:\\Temp&\\"));
  EXPECT_EQ(true, udStrEqual(v.Get("Settings[,0]").AsString(""), "C:\\Temp&\\"));

  EXPECT_EQ(true, v.Get("Settings.ImportAtFullScale").AsBool());
  EXPECT_EQ(true, v.Get("Settings[,1]").AsBool());

  EXPECT_EQ(2, v.Get("Settings.TerrainIndex").AsInt());
  EXPECT_EQ(5, v.Get("Settings.Inside.Count").AsInt());
  EXPECT_EQ(true, v.Get("Settings.EmptyArray").IsArray());
  EXPECT_EQ(0, v.Get("Settings.EmptyArray").ArrayLength());
  EXPECT_EQ(udR_Success, v.Get(&pTemp, "Settings.Nothing"));
  EXPECT_EQ(true, pTemp->IsVoid());
  // And the opposite test getting a key we know doesn't exist
  g_udBreakOnError = false; // Prevent break-on-error because we're knowingly creating error case
  EXPECT_EQ(udR_ObjectNotFound, v.Get(&pTemp, "Settings.DoesntExist"));
  g_udBreakOnError = true;
  EXPECT_EQ(true, udStrEqual(v.Get("Settings.SpecialChars").AsString(), "<>&\\/?[]{}\'\"%"));

  // Test accessing objects as implicit arrays
  EXPECT_EQ(5, v.Get("Settings[0].Inside[0].Count").AsInt());
  EXPECT_EQ(2, v.Get("Settings.Outside.Count").AsInt());
  for (int i = 0; i < v.Get("Settings.TestArray").ArrayLength(); ++i)
  {
    EXPECT_EQ(i, v.Get("Settings.TestArray[%d]", i).AsInt());
  }
  EXPECT_EQ(2, v.Get("Settings.TestArray[-1]").AsInt());
  EXPECT_EQ(1, v.Get("Settings.TestArray[-2]").AsInt());
  EXPECT_EQ(0, v.Get("Settings.TestArray[-3]").AsInt());
  EXPECT_EQ(true, v.Get("Settings.TestArray[-4]").IsVoid());
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udValueTests, CreationSimple)
{
  udValue v;

  // Assign attributes, these are present in both JSON and XML
  EXPECT_EQ(udR_Success, v.Set("Settings.ProjectsPath = '%s'", "C:\\\\Temp&\\\\")); // Note strings need to be escaped
  EXPECT_EQ(udR_Success, v.Set("Settings.ImportAtFullScale = true")); // Note the true/false is NOT quoted, making it a boolean internally
  EXPECT_EQ(udR_Success, v.Set("Settings.TerrainIndex = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings.Inside.Count = %d", 5));
  EXPECT_EQ(udR_Success, v.Set("Settings.Outside.Count = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings.Outside.content = 'windy'")); // This is a special member that will export as content text to the XML element
  g_udBreakOnError = false;
  EXPECT_NE(udR_Success, v.Set("Settings.Something"));
  g_udBreakOnError = true;
  EXPECT_EQ(udR_Success, v.Set("Settings.EmptyArray = []"));
  EXPECT_EQ(udR_Success, v.Set("Settings.Nothing = null"));
  // Of note here is that the input string is currently JSON escaped, so there's some additional backslashes
  EXPECT_EQ(udR_Success, v.Set("Settings.SpecialChars = '%s'", "<>&\\/?[]{}\\\'\\\"%"));
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[] = 0")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[] = 1")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[2] = 2")); // Only allowed to create directly when adding last on the array
  udValue_TestContent(v);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udValueTests, CreationSpecial)
{
  udValue v;

  // Assign attributes, these are present in both JSON and XML
  EXPECT_EQ(udR_Success, v.Set("Settings['ProjectsPath'] = '%s'", "C:\\\\Temp&\\\\")); // Note strings need to be escaped
  EXPECT_EQ(udR_Success, v.Set("Settings['ImportAtFullScale'] = true")); // Note the true/false is NOT quoted, making it a boolean internally
  EXPECT_EQ(udR_Success, v.Set("Settings['TerrainIndex'] = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings['Inside']['Count'] = %d", 5));
  EXPECT_EQ(udR_Success, v.Set("Settings['Outside']['Count'] = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings['Outside']['content'] = 'windy'")); // This is a special member that will export as content text to the XML element
  g_udBreakOnError = false;
  EXPECT_NE(udR_Success, v.Set("Settings['Something']"));
  g_udBreakOnError = true;
  EXPECT_EQ(udR_Success, v.Set("Settings['EmptyArray'] = []"));
  EXPECT_EQ(udR_Success, v.Set("Settings['Nothing'] = null"));
  // Of note here is that the input string is currently JSON escaped, so there's some additional backslashes
  EXPECT_EQ(udR_Success, v.Set("Settings['SpecialChars'] = '%s'", "<>&\\/?[]{}\\\'\\\"%"));
  EXPECT_EQ(udR_Success, v.Set("Settings['TestArray'][] = 0")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings['TestArray'][] = 1")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings['TestArray'][2] = 2")); // Only allowed to create directly when adding last on the array
  udValue_TestContent(v);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udValueTests, ParseAndExportJSON)
{
  udValue v;
  const char *pExportText = nullptr;

  EXPECT_EQ(udR_Success, v.Parse(pJSONTest));
  udValue_TestContent(v);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udVEO_JSON | udVEO_StripWhiteSpace));
  EXPECT_EQ(true, udStrEqual(pJSONTest, pExportText));
  udFree(pExportText);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udVEO_XML | udVEO_StripWhiteSpace));
  EXPECT_EQ(true, udStrEqual(pXMLTest, pExportText));
  udFree(pExportText);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udValueTests, ParseXML)
{
  udValue v;
  const char *pExportText = nullptr;

  EXPECT_EQ(udR_Success, v.Parse(pXMLTest));
  udValue_TestContent(v);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udVEO_XML | udVEO_StripWhiteSpace));
  EXPECT_EQ(true, udStrEqual(pXMLTest, pExportText));
  udFree(pExportText);
  // We don't test that exporting back to JSON worked, because it
  // won't. This is because everything in XML is a string, the types are lost
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udValueTests, ValueShouldNotTurnIntoObjectIfAlreadyArray)
{
  udValue output;

  //Output each application to the udValue.
  for (uint32_t i = 0; i < 3; ++i)
  {
    EXPECT_EQ(udR_Success, output.Set("[] = { 'name': 'Room %d', 'address': '127.0.0.1', 'macAddress': '::1', 'status': 'Bad', 'applications': [] }", i));
    EXPECT_STREQ("::1", output.Get("[-1].macAddress").AsString());
    EXPECT_EQ(udR_Success, output.Set("[%d].applications[] = { 'name': '%s' }", i, "Hello"));
  }

  EXPECT_EQ(3, output.ArrayLength());

  //Export the udValue as a JSON string and clean up.
  const char *outputTestStr;
  EXPECT_EQ(udR_Success, output.Export(&outputTestStr));
  udFree(outputTestStr);
}
