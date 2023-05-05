#include "gtest/gtest.h"
#include "udJSON.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udPlatform.h"
#include "udFile.h"

// First-pass most basic tests for udJSON
// TODO: Fix udMemoryDebugTracking to be useful and test memory leaks

static const char *pJSONTest = "{\"Settings\":{\"ProjectsPath\":\"C:\\\\Temp&\\\\\",\"ImportAtFullScale\":true,\"TerrainIndex\":2,\"Inside\":{\"Count\":5},\"Outside\":{\"Count\":2,\"content\":\"windy\"},\"EmptyArray\":[],\"Nothing\":null,\"SpecialChars\":\"<>&\\\\/?[]{}'\\\"%\",\"TestArray\":[0,1,2]}}";
static const char *pXMLTest = "<Settings ProjectsPath=\"C:\\Temp&amp;\\\" ImportAtFullScale=\"true\" TerrainIndex=\"2\" SpecialChars=\"&lt;&gt;&amp;\\/?[]{}'&quot;%\"><Inside Count=\"5\"/><Outside Count=\"2\">windy</Outside><EmptyArray></EmptyArray><Nothing/><TestArray>0</TestArray><TestArray>1</TestArray><TestArray>2</TestArray></Settings>";

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
static void udJSON_TestContent(udJSON &v)
{
  udJSON *pTemp = nullptr;
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
  EXPECT_EQ(udR_NotFound, v.Get(&pTemp, "Settings.DoesntExist"));
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
TEST(udJSONTests, CreationSimple)
{
  udJSON v;

  // Assign attributes, these are present in both JSON and XML
  EXPECT_EQ(udR_Success, v.Set("Settings.ProjectsPath = '%s'", "C:\\\\Temp&\\\\")); // Note strings need to be escaped
  EXPECT_EQ(udR_Success, v.Set("Settings.ImportAtFullScale = true")); // Note the true/false is NOT quoted, making it a boolean internally
  EXPECT_EQ(udR_Success, v.Set("Settings.TerrainIndex = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings.Inside.Count = %d", 5));
  EXPECT_EQ(udR_Success, v.Set("Settings.Outside.Count = %d", 2));
  EXPECT_EQ(udR_Success, v.Set("Settings.Outside.content = 'windy'")); // This is a special member that will export as content text to the XML element
  g_udBreakOnError = false;
  EXPECT_NE(udR_Success, v.Set("Settings.Something"));
  EXPECT_NE(udR_Success, v.Set("Settings.MyString = 'has ' quote'")); // Should fail because of errant quote character
  g_udBreakOnError = true;
  EXPECT_EQ(udR_Success, v.Set("Settings.MyString = 'has \\' quote'")); // Quote character escaped
  EXPECT_EQ(0, udStrcmp(v.Get("Settings.MyString").AsString(), "has ' quote"));
  EXPECT_EQ(udR_Success, v.Set("Settings.EmptyArray = []"));
  EXPECT_EQ(udR_Success, v.Set("Settings.Nothing = null"));
  // Of note here is that the input string is currently JSON escaped, so there's some additional backslashes
  EXPECT_EQ(udR_Success, v.Set("Settings.SpecialChars = '%s'", "<>&\\/?[]{}\\\'\\\"%"));
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[] = 0")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[] = 1")); // Append
  EXPECT_EQ(udR_Success, v.Set("Settings.TestArray[2] = 2")); // Only allowed to create directly when adding last on the array
  udJSON_TestContent(v);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udJSONTests, CreationSpecial)
{
  udJSON v;

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
  udJSON_TestContent(v);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
TEST(udJSONTests, ArrayEdgeCases)
{
  udJSON v;
  udJSON temp;

  temp.SetString("one");
  EXPECT_EQ(udR_Success, v.Set(&temp, "array[]"));
  EXPECT_EQ(1, v.Get("array").ArrayLength());
  EXPECT_EQ(udR_Success, v.Set("array[] = 'two'"));
  EXPECT_EQ(2, v.Get("array").ArrayLength());
  EXPECT_EQ(udR_Success, v.Set("array[] = 'three'"));
  EXPECT_EQ(3, v.Get("array").ArrayLength());
  EXPECT_EQ(udR_Success, v.Set("notarray = 1"));
  EXPECT_EQ(0, v.Get("notarray").ArrayLength());
  EXPECT_EQ(udR_ParseError, v.Set("notarray[] = 2"));
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udJSONTests, ParseAndExportJSON)
{
  udJSON v;
  const char *pExportText = nullptr;

  EXPECT_EQ(udR_Success, v.Parse(pJSONTest));
  udJSON_TestContent(v);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udJEO_JSON));
  EXPECT_EQ(true, udStrEqual(pJSONTest, pExportText));
  udFree(pExportText);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udJEO_XML));
  EXPECT_EQ(true, udStrEqual(pXMLTest, pExportText));
  udFree(pExportText);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udJSONTests, ParseXML)
{
  udJSON v;
  const char *pExportText = nullptr;

  EXPECT_EQ(udR_Success, v.Parse(pXMLTest));
  udJSON_TestContent(v);
  ASSERT_EQ(udR_Success, v.Export(&pExportText, udJEO_XML));
  EXPECT_EQ(true, udStrEqual(pXMLTest, pExportText));
  udFree(pExportText);
  // We don't test that exporting back to JSON worked, because it
  // won't. This is because everything in XML is a string, the types are lost
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udJSONTests, ValueShouldNotTurnIntoObjectIfAlreadyArray)
{
  udJSON output;

  //Output each application to the udJSON.
  for (uint32_t i = 0; i < 3; ++i)
  {
    EXPECT_EQ(udR_Success, output.Set("[] = { 'name': 'Room %d', 'address': '127.0.0.1', 'macAddress': '::1', 'status': 'Bad', 'applications': [] }", i));
    EXPECT_STREQ("::1", output.Get("[-1].macAddress").AsString());
    EXPECT_EQ(udR_Success, output.Set("[%d].applications[] = { 'name': '%s' }", i, "Hello"));
  }

  EXPECT_EQ(3, output.ArrayLength());

  //Export the udJSON as a JSON string and clean up.
  const char *outputTestStr;
  EXPECT_EQ(udR_Success, output.Export(&outputTestStr));
  udFree(outputTestStr);
}

TEST(udJSONTests, CreateArray)
{
  udJSON json;
  for (int i = 1; i < 5; ++i)
  {
    udResult result = json.Set("sequences[%d].id = %d", i - 1, i);
    EXPECT_EQ(0, result);
    EXPECT_EQ(i, json.Get("sequences[%d].id", i - 1).AsInt());
  }
  EXPECT_EQ(4, json.Get("sequences").ArrayLength());
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
TEST(udJSONTests, RemoveKey)
{
  udJSON v;

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
TEST(udJSONTests, udMathSpecialSupport)
{
  udJSON v;

  double zero3[3] = { 0.0, 0.0, 0.0 };
  double one3[3] = { 1.0, 1.0, 1.0 };
  double ident4x4[16] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
  double vec3[3] = { 1.0, 2.0, 3.0 };
  double vec4[4] = { 1.0, 2.0, 3.0, 4.0 };
  double mat3[16] = { 1.0, 2.0, 3.0, 0.0, 4.0, 5.0, 6.0, 0.0, 7.0, 8.0, 9.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
  double mat4[16] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 };

  EXPECT_EQ(udR_Success, v.Set("vec2 = [ 1.0, 2.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("vec3 = [ 1.0, 2.0, 3.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("vec4 = [ 1.0, 2.0, 3.0, 4.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("mat3 = [ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 ]"));
  EXPECT_EQ(udR_Success, v.Set("mat4 = [ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 ]"));

  double tempSpace[16];
  EXPECT_TRUE(!memcmp(vec3, v.Get("vec3").AsDoubleArray(tempSpace, 3), sizeof(vec3)));
  EXPECT_TRUE(!memcmp(vec4, v.Get("vec4").AsDoubleArray(tempSpace, 4), sizeof(vec4)));
  EXPECT_FALSE(!memcmp(mat3, v.Get("mat3").AsDoubleArray(tempSpace, 16, nullptr, false), sizeof(mat3)));
  EXPECT_TRUE(!memcmp(mat3, v.Get("mat3").AsDoubleArray(tempSpace, 16, nullptr, true), sizeof(mat3)));
  EXPECT_TRUE(!memcmp(mat4, v.Get("mat4").AsDoubleArray(tempSpace, 16), sizeof(mat4)));

  // Expected failures
  EXPECT_TRUE(!memcmp(zero3, v.Get("vec2").AsDoubleArray(tempSpace, 3), sizeof(zero3)));
  EXPECT_TRUE(!memcmp(one3, v.Get("vec2").AsDoubleArray(tempSpace, 3, one3), sizeof(one3)));
  EXPECT_TRUE(!memcmp(ident4x4, v.Get("vec2").AsDoubleArray(tempSpace, 16, ident4x4), sizeof(ident4x4)));

  // Set udFloat3 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.SetDoubleArray(vec3, udLengthOf(vec3)));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(3, v.ArrayLength());
  for (int i = 0; i < (int)3; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set udFloat4 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.SetDoubleArray(vec4, udLengthOf(vec4)));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(4, v.ArrayLength());
  for (int i = 0; i < 4; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set 3x3 matrix - minimally
  v.Destroy();
  EXPECT_EQ(udR_Success, v.SetDoubleArray(mat3, udLengthOf(mat3), true));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(9, v.ArrayLength());
  for (int i = 0; i < 9; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }

  // Set 3x3 matrix - not minimally
  v.Destroy();
  EXPECT_EQ(udR_Success, v.SetDoubleArray(mat3, udLengthOf(mat3), false));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(16, v.ArrayLength());
  for (int i = 0; i < 9; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", (i / 3) * 4 + (i % 3)).AsDouble());
  }

  // Set 4x4 function
  v.Destroy();
  EXPECT_EQ(udR_Success, v.SetDoubleArray(mat4, udLengthOf(mat4)));
  EXPECT_TRUE(v.IsArray());
  EXPECT_EQ(16, v.ArrayLength());
  for (int i = 0; i < 16; i++)
  {
    EXPECT_EQ(double(i) + 1.0, v.Get("[%d]", i).AsDouble());
  }
}

static const char *pTestWKTs[] =
{
  "PROJCS[\"GDA94 / MGA zone 52\",GEOGCS[\"GDA94\",DATUM[\"Geocentric_Datum_of_Australia_1994\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6283\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4283\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",129],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"28352\"]]",
  "PROJCS[\"GDA94 / MGA zone 53\",GEOGCS[\"GDA94\",DATUM[\"Geocentric_Datum_of_Australia_1994\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6283\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4283\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",135],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"28353\"]]",
  "PROJCS[\"GDA94 / MGA zone 54\",GEOGCS[\"GDA94\",DATUM[\"Geocentric_Datum_of_Australia_1994\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6283\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4283\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",141],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"28354\"]]",
  "PROJCS[\"GDA94 / MGA zone 55\",GEOGCS[\"GDA94\",DATUM[\"Geocentric_Datum_of_Australia_1994\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6283\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4283\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",147],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"28355\"]]",
  "PROJCS[\"GDA94 / MGA zone 56\",GEOGCS[\"GDA94\",DATUM[\"Geocentric_Datum_of_Australia_1994\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6283\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4283\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",153],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"28356\"]]",
  "PROJCS[\"WGS 84 / UTM zone 18N\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-75],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"32618\"]]",
  "PROJCS[\"WGS 84 / UTM zone 33N\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",15],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"32633\"]]",
  "PROJCS[\"WGS 84 / UTM zone 54S\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",141],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"32754\"]]",
  "PROJCS[\"WGS 84 / UTM zone 54S\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",141],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"32754\"]]",
  "PROJCS[\"WGS 84 / UTM zone 55S\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",147],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"32755\"]]",
  "PROJCS[\"WGS 84 / UTM zone 56S\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",153],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",10000000],AUTHORITY[\"EPSG\",\"32756\"],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]",
  "PROJCS[\"NAD83 / UTM zone 14N\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-99],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"26914\"]]",
  "PROJCS[\"NAD83 / UTM zone 6N\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-147],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"26906\"]]",
  "PROJCS[\"NAD83 / California zone 6 (ftUS)\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\",33.88333333333333],PARAMETER[\"standard_parallel_2\",32.78333333333333],PARAMETER[\"latitude_of_origin\",32.16666666666666],PARAMETER[\"central_meridian\",-116.25],PARAMETER[\"false_easting\",6561666.667],PARAMETER[\"false_northing\",1640416.667],UNIT[\"US survey foot\",0.3048006096012192,AUTHORITY[\"EPSG\",\"9003\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"2230\"]]",
  "PROJCS[\"NAD83(HARN) / California zone 6\",GEOGCS[\"NAD83(HARN)\",DATUM[\"NAD83_High_Accuracy_Reference_Network\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6152\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4152\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\",33.88333333333333],PARAMETER[\"standard_parallel_2\",32.78333333333333],PARAMETER[\"latitude_of_origin\",32.16666666666666],PARAMETER[\"central_meridian\",-116.25],PARAMETER[\"false_easting\",2000000],PARAMETER[\"false_northing\",500000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"2771\"]]",
  "PROJCS[\"NAD83 / Texas South Central (ftUS)\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\",30.28333333333334],PARAMETER[\"standard_parallel_2\",28.38333333333333],PARAMETER[\"latitude_of_origin\",27.83333333333333],PARAMETER[\"central_meridian\",-99],PARAMETER[\"false_easting\",1968500],PARAMETER[\"false_northing\",13123333.333],UNIT[\"US survey foot\",0.3048006096012192,AUTHORITY[\"EPSG\",\"9003\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"2278\"]]",
  "PROJCS[\"NAD83 / Alberta 10-TM (Forest)\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-115],PARAMETER[\"scale_factor\",0.9992],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"3400\"]]",
  "PROJCS[\"OSGB 1936 / British National Grid\",GEOGCS[\"OSGB 1936\",DATUM[\"OSGB_1936\",SPHEROID[\"Airy 1830\",6377563.396,299.3249646,AUTHORITY[\"EPSG\",\"7001\"]],TOWGS84[375,-111,431,0,0,0,0],AUTHORITY[\"EPSG\",\"6277\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4277\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",49],PARAMETER[\"central_meridian\",-2],PARAMETER[\"scale_factor\",0.9996012717],PARAMETER[\"false_easting\",400000],PARAMETER[\"false_northing\",-100000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"27700\"]]",
  "PROJCS[\"JGD2000 / Japan Plane Rectangular CS VI\",GEOGCS[\"JGD2000\",DATUM[\"Japanese_Geodetic_Datum_2000\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6612\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4612\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",36],PARAMETER[\"central_meridian\",136],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",NORTH],AXIS[\"Y\",EAST],AUTHORITY[\"EPSG\",\"2448\"]]",
  "PROJCS[\"JGD2000 / Japan Plane Rectangular CS II\",GEOGCS[\"JGD2000\",DATUM[\"Japanese_Geodetic_Datum_2000\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6612\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4612\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",33],PARAMETER[\"central_meridian\",131],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",NORTH],AXIS[\"Y\",EAST],AUTHORITY[\"EPSG\",\"2444\"]]",
  "PROJCS[\"MGI / Austria GK East\",GEOGCS[\"MGI\",DATUM[\"Militar_Geographische_Institute\",SPHEROID[\"Bessel 1841\",6377397.155,299.1528128,AUTHORITY[\"EPSG\",\"7004\"]],TOWGS84[577.326,90.129,463.919,5.137,1.474,5.297,2.4232],AUTHORITY[\"EPSG\",\"6312\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4312\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",16.33333333333333],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",-5000000],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AUTHORITY[\"EPSG\",\"31256\"]]",
  "PROJCS[\"NAD83(NSRS2007) / Alaska zone 3\",GEOGCS[\"NAD83(NSRS2007)\",DATUM[\"NAD83_National_Spatial_Reference_System_2007\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6759\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4759\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",54],PARAMETER[\"central_meridian\",-146],PARAMETER[\"scale_factor\",0.9999],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"3470\"]]",
  "PROJCS[\"NAD83(NSRS2007) / Colorado North (ftUS)\",GEOGCS[\"NAD83(NSRS2007)\",DATUM[\"NAD83_National_Spatial_Reference_System_2007\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6759\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4759\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"standard_parallel_1\",40.78333333333333],PARAMETER[\"standard_parallel_2\",39.71666666666667],PARAMETER[\"latitude_of_origin\",39.33333333333334],PARAMETER[\"central_meridian\",-105.5],PARAMETER[\"false_easting\",3000000],PARAMETER[\"false_northing\",1000000],UNIT[\"US survey foot\",0.3048006096012192,AUTHORITY[\"EPSG\",\"9003\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"3504\"]]",
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
TEST(udJSONTests, WKT)
{
  for (size_t testNr = 0; testNr < UDARRAYSIZE(pTestWKTs); ++testNr)
  {
    udJSON v;
    const char *pWKT = nullptr;

    udResult result = udParseWKT(&v, pTestWKTs[testNr]);
    EXPECT_EQ(udR_Success, result);

    result = udExportWKT(&pWKT, &v);
    EXPECT_EQ(udR_Success, result);

    EXPECT_STREQ(pTestWKTs[testNr], pWKT);

    udFree(pWKT);
    v.Destroy();
  }
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
TEST(udJSONTests, XMLExport)
{
  udResult result;
  const char *pXML = nullptr;
  udJSON value;

  // Two child elements with no attributes
  result = value.Set("a.b[] = null");
  EXPECT_EQ(udR_Success, result);
  result = value.Set("a.b[] = null");
  EXPECT_EQ(udR_Success, result);
  result = value.Export(&pXML, udJEO_XML);
  EXPECT_EQ(udR_Success, result);
  EXPECT_STREQ("<a><b></b><b></b></a>", pXML);
  udFree(pXML);
  value.Destroy();

  // An attribute and no child elements
  result = value.Set("a.c = 'string'");
  EXPECT_EQ(udR_Success, result);
  result = value.Export(&pXML, udJEO_XML);
  EXPECT_STREQ("<a c=\"string\"/>", pXML);
  EXPECT_EQ(udR_Success, result);
  udFree(pXML);
  value.Destroy();

  // Attribute and child elements
  result = value.Set("a.b[] = null");
  EXPECT_EQ(udR_Success, result);
  result = value.Set("a.c = 'string'");
  EXPECT_EQ(udR_Success, result);
  result = value.Export(&pXML, udJEO_XML);
  EXPECT_STREQ("<a c=\"string\"><b></b></a>", pXML);
  EXPECT_EQ(udR_Success, result);
  udFree(pXML);
  value.Destroy();
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, November 2018
TEST(udJSONTests, SpecialCharacterCompliance)
{
  udJSON in, out, temp;
  const char *pExportText = nullptr;
  const char validStr[] = R"str({"quotationMark":"\"","reverseSolidus":"\\","backspace":"\b","formFeed":"\f","lineFeed":"\n","carriageReturn":"\r","tabulation":"\t"})str";

  in.SetMemberString("\"", "quotationMark");
  in.SetMemberString("\\", "reverseSolidus");
  in.SetMemberString("\b", "backspace");
  in.SetMemberString("\f", "formFeed");
  in.SetMemberString("\n", "lineFeed");
  in.SetMemberString("\r", "carriageReturn");
  in.SetMemberString("\t", "tabulation");

  ASSERT_EQ(udR_Success, in.Export(&pExportText));
  EXPECT_TRUE(udStrEqual(validStr, pExportText));

  EXPECT_EQ(udR_Success, out.Parse(pExportText));

  EXPECT_STREQ("\"", out.Get("quotationMark").AsString());
  EXPECT_STREQ("\\", out.Get("reverseSolidus").AsString());
  EXPECT_STREQ("\b", out.Get("backspace").AsString());
  EXPECT_STREQ("\f", out.Get("formFeed").AsString());
  EXPECT_STREQ("\n", out.Get("lineFeed").AsString());
  EXPECT_STREQ("\r", out.Get("carriageReturn").AsString());
  EXPECT_STREQ("\t", out.Get("tabulation").AsString());

  udFree(pExportText);
}

TEST(udJSONTests, XMLEncoding)
{
  udJSON v;

  v.Parse("<root tag=\"tag:&quot;&#xE3;&#x81;&#x99;&apos;\">content:&lt;&#xE3;&#x81;&#x99;&gt;</root>");
  EXPECT_STREQ("tag:\"\xE3\x81\x99\'", v.Get("root.tag").AsString());
  EXPECT_STREQ("content:<\xE3\x81\x99>", v.Get("root.content").AsString());
}

TEST(udJSONTests, ExponentStr)
{
  const char *pData = R"json({
  "46CDC": {
    "uuid": "610fe368-91ed-4f74-885a-d2a166425f43",
    "cloudpercent": 0.0,
    "area": 2e-06,
    "length": 3E-07,
    "date": "2019-12-01T07:00:20.885Z",
    "id": 23732830,
    "thumb": true,
    "processed": true
  }
})json";

  udJSON data;
  ASSERT_EQ(udR_Success, data.Parse(pData));

  EXPECT_STREQ("610fe368-91ed-4f74-885a-d2a166425f43", data.Get("46CDC.uuid").AsString());
  EXPECT_DOUBLE_EQ(0.0, data.Get("46CDC.cloudpercent").AsDouble());
  EXPECT_DOUBLE_EQ(2e-6, data.Get("46CDC.area").AsDouble());
  EXPECT_DOUBLE_EQ(3e-7, data.Get("46CDC.length").AsDouble());
  EXPECT_STREQ("2019-12-01T07:00:20.885Z", data.Get("46CDC.date").AsString());
  EXPECT_EQ(23732830, data.Get("46CDC.id").AsInt());
  EXPECT_TRUE(data.Get("46CDC.thumb").AsBool());
  EXPECT_TRUE(data.Get("46CDC.processed").AsBool());
}

TEST(udJSONTests, CDATAXML)
{
  const char *pData = R"xml(
  <vectorChild type="Structure">
    <guid type="String"><![CDATA[{D5D720DD-E02F-4342-9AE1-49D91341FD2F}]]></guid>
    <name type="String"><![CDATA[Station 018]]></name>
  </vectorChild>
  )xml";

  udJSON json = {};

  ASSERT_EQ(udR_Success, json.Parse(pData));

  EXPECT_STREQ("{D5D720DD-E02F-4342-9AE1-49D91341FD2F}", json.Get("vectorChild.guid.content").AsString());
  EXPECT_STREQ("Station 018", json.Get("vectorChild.name.content").AsString());
}


#if !UDPLATFORM_EMSCRIPTEN // Disabled because of local file access issues
TEST(udJSONTests, CDATAXMLE57)
{
  const char *pFilename = "testfiles/e57.xml";
  if (udFileExists("../../../testfiles/e57.xml") == udR_Success)
    pFilename = "../../../testfiles/e57.xml";

  const char *pBuffer = nullptr;
  int64_t fsize = 0;
  ASSERT_EQ(udR_Success, udFile_Load(pFilename, &pBuffer, &fsize)) << "Could not open spatial reference file";

  udJSON e57Data = {};
  EXPECT_EQ(udR_Success, e57Data.Parse(pBuffer));

  EXPECT_STREQ("{93228F34-467F-4FBE-95FA-5D02D5EF5FB6}", e57Data.Get("e57Root.images2D.vectorChild.guid.content").AsString());
  EXPECT_STREQ("Station 018", e57Data.Get("e57Root.images2D.vectorChild.name.content").AsString());

  udFree(pBuffer);
}
#endif
