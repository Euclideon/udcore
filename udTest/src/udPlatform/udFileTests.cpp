#include "gtest/gtest.h"
#include "udFile.h"
#include "udPlatformUtil.h"

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udFileTests, GeneralFileTests)
{
  const char *pFileName = "._donotcommit";
  EXPECT_NE(udR_Success, udFileExists(pFileName));

  udFile *pFile;
  if (udFile_Open(&pFile, pFileName, udFOF_Write) == udR_Success)
    udFile_Close(&pFile);

  EXPECT_EQ(udR_Success, udFileExists(pFileName));
  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
}

TEST(udFileTests, GeneralDirectoryTests)
{
  const char *pDirectoryName = "._notadirectory";

  udFindDir *pDir = nullptr;
  EXPECT_EQ(udR_File_OpenFailure, udOpenDir(&pDir, pDirectoryName));
  ASSERT_EQ(nullptr, pDir);
}
