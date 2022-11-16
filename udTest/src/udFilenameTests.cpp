#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "gtest/gtest.h"

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, July 2021
TEST(udFilenameTests, Validate)
{
  const char filename[] = "test";
  const char filenameWithExt[] = "test.ext";
  const char filenameWithFolder[] = "folder/test";
  const char filenameWithFolderAndExt[] = "folder/test.ext";

  // Test default constructor
  {
    udFilename fn = udFilename();
    EXPECT_STREQ("", fn.GetPath());
    EXPECT_STREQ("", fn.GetFilenameWithExt());
    EXPECT_STREQ("", fn.GetExt());
  }

  // Test string constructor - No folder, no ext
  {
    udFilename fn = udFilename(filename);
    EXPECT_STREQ("test", fn.GetPath());
    EXPECT_STREQ("test", fn.GetFilenameWithExt());
    EXPECT_STREQ("", fn.GetExt());
  }

  // Test string constructor - No folder
  {
    udFilename fn = udFilename(filenameWithExt);
    EXPECT_STREQ("test.ext", fn.GetPath());
    EXPECT_STREQ("test.ext", fn.GetFilenameWithExt());
    EXPECT_STREQ(".ext", fn.GetExt());
  }

  // Test string constructor - no ext
  {
    udFilename fn = udFilename(filenameWithFolder);
    EXPECT_STREQ("folder/test", fn.GetPath());
    EXPECT_STREQ("test", fn.GetFilenameWithExt());
    EXPECT_STREQ("", fn.GetExt());
  }

  // Test string constructor
  {
    udFilename fn = udFilename(filenameWithFolderAndExt);
    EXPECT_STREQ("folder/test.ext", fn.GetPath());
    EXPECT_STREQ("test.ext", fn.GetFilenameWithExt());
    EXPECT_STREQ(".ext", fn.GetExt());
  }

  // Test ExtractFolder - empty path
  {
    udFilename fn = udFilename();
    char folder[256] = {};
    int folderLen = fn.ExtractFolder(folder, (int)udLengthOf(folder));
    const char expected[] = "";
    EXPECT_EQ(udLengthOf(expected), folderLen);
    EXPECT_STREQ(expected, folder);
    EXPECT_EQ(folderLen, udStrlen(folder) + 1);
  }

  // Test ExtractFolder
  {
    udFilename fn = udFilename(filenameWithFolderAndExt);
    char folder[256] = {};
    int folderLen = fn.ExtractFolder(folder, (int)udLengthOf(folder));
    const char expected[] = "folder/";
    EXPECT_EQ(udLengthOf(expected), folderLen);
    EXPECT_STREQ(expected, folder);
    EXPECT_EQ(folderLen, udStrlen(folder) + 1);
  }

  // Test ExtractFilenameOnly - empty path
  {
    udFilename fn = udFilename();
    char filenameOnly[256] = {};
    int filenameLen = fn.ExtractFilenameOnly(filenameOnly, (int)udLengthOf(filenameOnly));
    const char expected[] = "";
    EXPECT_EQ(udLengthOf(expected), filenameLen);
    EXPECT_STREQ(expected, filenameOnly);
    EXPECT_EQ(filenameLen, udStrlen(filenameOnly) + 1);
  }

  // Test ExtractFilenameOnly
  {
    udFilename fn = udFilename(filenameWithFolderAndExt);
    char filenameOnly[256] = {};
    int filenameLen = fn.ExtractFilenameOnly(filenameOnly, (int)udLengthOf(filenameOnly));
    const char expected[] = "test";
    EXPECT_EQ(udLengthOf(expected), filenameLen);
    EXPECT_STREQ(expected, filenameOnly);
    EXPECT_EQ(filenameLen, udStrlen(filenameOnly) + 1);
  }

  // Test SetFromFullPath
  {
    udFilename fn;
    fn.SetFromFullPath("%s", filenameWithFolderAndExt);
    EXPECT_STREQ(filenameWithFolderAndExt, fn.GetPath());
    EXPECT_STREQ("test.ext", fn.GetFilenameWithExt());
    EXPECT_STREQ(".ext", fn.GetExt());
  }

  // Test SetFolder
  {
    udFilename fn;
    fn.SetFolder("folder");
    EXPECT_STREQ("folder/", fn.GetPath());
    EXPECT_STREQ("", fn.GetFilenameWithExt());
    EXPECT_STREQ("", fn.GetExt());
  }

  // Test SetFilenameNoExt
  {
    udFilename fn;
    fn.SetFilenameNoExt("test");
    EXPECT_STREQ("test", fn.GetPath());
    EXPECT_STREQ("test", fn.GetFilenameWithExt());
    EXPECT_STREQ("", fn.GetExt());
  }

  // Test SetFilenameWithExt
  {
    udFilename fn;
    fn.SetFilenameWithExt("test.ext");
    EXPECT_STREQ("test.ext", fn.GetPath());
    EXPECT_STREQ("test.ext", fn.GetFilenameWithExt());
    EXPECT_STREQ(".ext", fn.GetExt());
  }

  // Test SetExtension
  {
    udFilename fn;
    fn.SetExtension(".ext");
    EXPECT_STREQ(".ext", fn.GetPath());
    EXPECT_STREQ(".ext", fn.GetFilenameWithExt());
    EXPECT_STREQ(".ext", fn.GetExt());
  }

  // Test copy assignment operator
  {
    udFilename fn = filenameWithFolderAndExt;
    EXPECT_STREQ(filenameWithFolderAndExt, fn.GetPath());
    udFilename fn2;
    fn2 = fn;
    EXPECT_STREQ(filenameWithFolderAndExt, fn2.GetPath());

    // Confirm values are copied correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
    EXPECT_STRNE(fn.GetPath(), fn2.GetPath());
  }

  // Test copy constructor
  {
    udFilename fn = filenameWithFolderAndExt;
    EXPECT_STREQ(filenameWithFolderAndExt, fn.GetPath());
    udFilename fn2 = fn;
    EXPECT_STREQ(filenameWithFolderAndExt, fn2.GetPath());

    // Confirm values are copied correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
    EXPECT_STRNE(fn.GetPath(), fn2.GetPath());
  }

  // Test move assignment operator
  {
    udFilename fn = filenameWithFolderAndExt;
    EXPECT_STREQ(filenameWithFolderAndExt, fn.GetPath());
    udFilename fn2;
    fn2 = std::move(fn);
    EXPECT_STREQ(filenameWithFolderAndExt, fn2.GetPath());

    // Confirm values are moved correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
  }

  // Test move constructor
  {
    udFilename fn = filenameWithFolderAndExt;
    EXPECT_STREQ(filenameWithFolderAndExt, fn.GetPath());
    udFilename fn2 = std::move(fn);
    EXPECT_STREQ(filenameWithFolderAndExt, fn2.GetPath());

    // Confirm values are moved correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
  }
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, July 2021
TEST(udFilenameTests, LongFilenameValidate)
{
#define filenameSuperLongFolder      "https://staging.udcloud.euclideon.com/api/workspace/_sharecode/cvPF2W714ky68u1svMhKRw/_file/project/"
#define filenameSuperLongFilename    "%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99%E3%81%93%E3%82%8C%E3%81%AF%E3%83%86%E3%82%B9%E3%83%88%E3%81%A7%E3%81%99"
#define filenameSuperLongFilenameExt ".uds"
  const char filenameSuperLong[] = filenameSuperLongFolder filenameSuperLongFilename filenameSuperLongFilenameExt;
  UDCOMPILEASSERT(udLengthOf(filenameSuperLong) >= udFilename::MaxPath, "This filename isn't long enough!");

  udFilename fn = udFilename(filenameSuperLong);
  EXPECT_STREQ(filenameSuperLongFolder filenameSuperLongFilename filenameSuperLongFilenameExt, fn.GetPath());
  EXPECT_STREQ(filenameSuperLongFilename filenameSuperLongFilenameExt, fn.GetFilenameWithExt());
  EXPECT_STREQ(filenameSuperLongFilenameExt, fn.GetExt());

  // Test extract folder
  {
    int requiredSize = fn.ExtractFolder(nullptr, 0);
    EXPECT_EQ(udLengthOf(filenameSuperLongFolder), requiredSize);
    char *pFolder = udAllocType(char, requiredSize, udAF_None);
    fn.ExtractFolder(pFolder, requiredSize);
    EXPECT_STREQ(filenameSuperLongFolder, pFolder);
    EXPECT_EQ(requiredSize, udStrlen(pFolder) + 1);
    udFree(pFolder);
  }

  // Test extract filename
  {
    int requiredSize = fn.ExtractFilenameOnly(nullptr, 0);
    EXPECT_EQ(udLengthOf(filenameSuperLongFilename), requiredSize);
    char *pFilename = udAllocType(char, requiredSize, udAF_None);
    fn.ExtractFilenameOnly(pFilename, requiredSize);
    EXPECT_STREQ(filenameSuperLongFilename, pFilename);
    EXPECT_EQ(requiredSize, udStrlen(pFilename) + 1);
    udFree(pFilename);
  }

  // Test copy assignment operator
  {
    udFilename fn2;
    fn2 = fn;
    EXPECT_STREQ(filenameSuperLong, fn2.GetPath());

    // Confirm values are copied correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
    EXPECT_STRNE(fn.GetPath(), fn2.GetPath());
  }

  // Test copy constructor
  {
    udFilename fn2 = fn;
    EXPECT_STREQ(filenameSuperLong, fn2.GetPath());

    // Confirm values are copied correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
    EXPECT_STRNE(fn.GetPath(), fn2.GetPath());
  }

  // Test move assignment operator
  {
    udFilename fn1 = filenameSuperLong;
    EXPECT_STREQ(filenameSuperLong, fn1.GetPath());
    udFilename fn2;
    fn2 = std::move(fn1);
    EXPECT_STREQ(filenameSuperLong, fn2.GetPath());

    // Confirm values are moved correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
  }

  // Test move constructor
  {
    udFilename fn1 = filenameSuperLong;
    EXPECT_STREQ(filenameSuperLong, fn1.GetPath());
    udFilename fn2 = std::move(fn1);
    EXPECT_STREQ(filenameSuperLong, fn2.GetPath());

    // Confirm values are moved correctly, should crash if `pPath` isn't set properly
    EXPECT_TRUE(fn2.SetExtension(".test"));
  }
}
