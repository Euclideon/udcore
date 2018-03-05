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
  for (int i = 0; i < (int)v.Get("Settings.TestArray").ArrayLength(); ++i)
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

TEST(udValueTests, CreateArray)
{
  udValue json;
  for (size_t i = 1; i < 5; ++i)
  {
    udResult result = json.Set("sequences[%d].id = %d", i - 1, i);
    EXPECT_EQ(0, result);
    EXPECT_EQ(i, json.Get("sequences[%d].id", i - 1).AsInt());
  }
  EXPECT_EQ(4, json.Get("sequences").ArrayLength());
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udValueTests, RemoveKey)
{
  udValue v;

  // Assign attributes, these are present in both JSON and XML
  EXPECT_EQ(udR_Success, v.Set("Settings.One = 1"));
  EXPECT_EQ(udR_Success, v.Set("Settings.Two = 1"));
  EXPECT_EQ(udR_Success, v.Set("Settings.Three = 1"));
  EXPECT_EQ(3, v.Get("Settings").MemberCount());
  EXPECT_EQ(udR_Success, v.Set("Settings.Two"));
  EXPECT_EQ(2, v.Get("Settings").MemberCount());
  EXPECT_EQ(udR_Success, v.Set("Settings.Three"));
  EXPECT_EQ(1, v.Get("Settings").MemberCount());
  EXPECT_EQ(udR_Success, v.Set("Settings"));
  EXPECT_EQ(0, v.MemberCount());
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, February 2018
TEST(udValueTests, udMathSpecialSupport)
{
  udValue v;

  udDouble3 vec3 = udDouble3::create(1.0, 2.0, 3.0);
  udDouble4 vec4 = udDouble4::create(1.0, 2.0, 3.0, 4.0);
  udDoubleQuat quat = udDoubleQuat::identity();
  quat.x = 1.0;
  quat.y = 2.0;
  quat.z = 3.0;
  quat.w = 4.0;
  udDouble4x4 mat3 = udDouble4x4::create(1.0, 2.0, 3.0, 0.0, 4.0, 5.0, 6.0, 0.0, 7.0, 8.0, 9.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  udDouble4x4 mat4 = udDouble4x4::create(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0);

  EXPECT_EQ(udR_Success, v.Set("vec2 = [ 1.0, 2.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("vec3 = [ 1.0, 2.0, 3.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("vec4 = [ 1.0, 2.0, 3.0, 4.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("quat = [ 1.0, 2.0, 3.0, 4.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("mat3 = [ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("mat4 = [ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 ]"));

  EXPECT_EQ(vec3, v.Get("vec3").AsDouble3());
  EXPECT_EQ(vec4, v.Get("vec4").AsDouble4());
  // udQuaternion doesn't have an equal operator overload,
  // I believe this is due to the complexity of comparing two quaternions.
  // In this particular case, we're satisfied with the simple case.
  EXPECT_EQ(quat.toVector4(), v.Get("quat").AsQuaternion().toVector4());
  EXPECT_EQ(mat3, v.Get("mat3").AsDouble4x4());
  EXPECT_EQ(mat4, v.Get("mat4").AsDouble4x4());

  // Expected failures
  EXPECT_EQ(udDouble3::zero(), v.Get("vec2").AsDouble3());
  EXPECT_EQ(udDouble4::zero(), v.Get("vec2").AsDouble4());
  EXPECT_EQ(udDoubleQuat::identity().toVector4(), v.Get("vec2").AsQuaternion().toVector4());
  EXPECT_EQ(udDouble4x4::identity(), v.Get("vec2").AsDouble4x4());
  EXPECT_EQ(udDouble4x4::identity(), v.Get("vec2").AsDouble4x4());

  // Set udFloat3 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(vec3));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(3, v.ArrayLength());
  for (size_t i = 0; i < vec3.ElementCount; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set udFloat4 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(vec4));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(4, v.ArrayLength());
  for (size_t i = 0; i < vec4.ElementCount; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set udQuaternion function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(quat));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(4, v.ArrayLength());
  for (size_t i = 0; i < quat.ElementCount; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set udFloat3x3 function - minimally
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(mat3, true));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(9, v.ArrayLength());
  for (size_t i = 0; i < 9; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set udFloat3x3 function - not minimally
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(mat3, false));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(16, v.ArrayLength());
  for (size_t i = 0; i < 9; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", (i / 3) * 4 + (i % 3)).AsDouble());
  }

  // Set udFloat4x4 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.Set(mat4));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(16, v.ArrayLength());
  for (size_t i = 0; i < 16; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }
}
