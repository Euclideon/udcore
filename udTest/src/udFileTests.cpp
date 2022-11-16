#include "udCrypto.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udMath.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "gtest/gtest.h"

static const size_t s_QBF_Len = 43; // Not including NUL character
static const char *s_pQBF_Text = "The quick brown fox jumps over the lazy dog";
static const char *s_pQBF_Uncomp = "raw://VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==";
static const char *s_pQBF_RawDef = "raw://compression=RawDeflate,size=43@C8lIVSgszUzOVkgqyi/PU0jLr1DIKs0tKFbIL0stUigBSuckVlUqpOSnAwA=";
static const char *s_pQBF_GzipDef = "raw://compression=GzipDeflate,size=43@H4sIAAAAAAAA/wvJSFUoLM1MzlZIKsovz1NIy69QyCrNLShWyC9LLVIoAUrnJFZVKqTkpwMAOaNPQSsAAAA=";
static const char *s_pQBF_ZlibDef = "raw://compression=ZlibDeflate,size=43@eJwLyUhVKCzNTM5WSCrKL89TSMuvUMgqzS0oVsgvSy1SKAFK5yRWVSqk5KcDAFvcD9o=";
static const char *s_pQBF_DataBase64 = "data:text/plain;base64,VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==";

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udFileTests, GeneralFileTests)
{
  const char *pFilename = "._donotcommit";
  EXPECT_NE(udR_Success, udFileExists(pFilename));

  int64_t currentTime = udGetEpochSecsUTCd();

  udFile *pFile;
  if (udFile_Open(&pFile, pFilename, udFOF_Write) == udR_Success)
  {
    udFile_Write(pFile, "TEST", 4);
    udFile_Close(&pFile);
  }

  int64_t size = 0;
  int64_t modifyTime = 0;

  EXPECT_EQ(udR_Success, udFileExists(pFilename, &size, &modifyTime));

  EXPECT_EQ(4, size);
  EXPECT_GT(2, udAbs(currentTime - modifyTime));

  EXPECT_EQ(udR_Success, udFileDelete(pFilename));
  EXPECT_EQ(udR_NotFound, udFileExists(pFilename));

  // Additional destructions of non-existent objects
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));
  EXPECT_EQ(udR_InvalidParameter, udFile_Close(nullptr));
}

TEST(udFileTests, GeneralDirectoryTests)
{
  const char *pDirectoryName = "._notadirectory";

  udFindDir *pDir = nullptr;
  EXPECT_EQ(udR_OpenFailure, udOpenDir(&pDir, pDirectoryName));
  ASSERT_EQ(nullptr, pDir);
}

TEST(udFileTests, BasicReadWriteLoadFILE)
{
  const char *pFilename = "._donotcommit_FILEtest";
  const char writeBuffer[] = "Testing!";
  EXPECT_NE(udR_Success, udFileExists(pFilename));

  // Write
  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, writeBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  // Read
  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Read));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer)));
  EXPECT_STREQ(writeBuffer, readBuffer);

  // Check performance
  udFilePerformance performance;
  EXPECT_EQ(udR_Success, udFile_GetPerformance(pFile, &performance));
  EXPECT_NE(0, performance.throughput);
  EXPECT_NE(0.f, performance.mbPerSec);
  EXPECT_EQ(0, performance.requestsInFlight);

  // Release file
  EXPECT_EQ(udR_Success, udFile_Release(pFile));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer), 0, udFSW_SeekSet));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  // Change seek base
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Read));
  udFile_SetSeekBase(pFile, 1);
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer) - 1));
  EXPECT_STREQ(&writeBuffer[1], readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  // Load
  char *pLoadBuffer = nullptr;
  EXPECT_EQ(udR_Success, udFile_Load(pFilename, (void **)&pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFile_Load(pFilename, &pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFileExists(pFilename));
  EXPECT_EQ(udR_Success, udFileDelete(pFilename));
  EXPECT_NE(udR_Success, udFileExists(pFilename));
}

TEST(udFileTests, EncryptedReadWriteFILE)
{
  udCrypto_Init();

  const char *pFilename = "._donotcommit_EncryptedFILEtest";
  const char writeBuffer[] = "Testing!asdfasdfasdfasdfasdfasd";
  char cipherBuffer[UDARRAYSIZE(writeBuffer)];
  uint8_t *pKey = nullptr;
  size_t keyLen = 0;
  const char *pKeyBase64 = nullptr;
  ASSERT_EQ(udR_Success, udCryptoKey_DeriveFromRandom(&pKeyBase64, udCCKL_AES256KeyLength));
  EXPECT_EQ(udR_Success, udBase64Decode(&pKey, &keyLen, pKeyBase64));
  EXPECT_NE(udR_Success, udFileExists(pFilename));

  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Write));
  udCryptoCipherContext *pCipherCtx;
  EXPECT_EQ(udR_Success, udCryptoCipher_Create(&pCipherCtx, udCC_AES256, udCPM_None, pKeyBase64, udCCM_CTR));
  udCryptoIV iv;
  EXPECT_EQ(udR_Success, udCrypto_CreateIVForCTRMode(pCipherCtx, &iv, 12, 0));
  EXPECT_EQ(udR_Success, udCryptoCipher_Encrypt(pCipherCtx, &iv, writeBuffer, UDARRAYSIZE(writeBuffer), cipherBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pCipherCtx));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, cipherBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Read));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer)));
  EXPECT_STRNE(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_SetEncryption(pFile, pKey, (int)keyLen, 12));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer), 0, udFSW_SeekSet));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  udFree(pKey);
  udFree(pKeyBase64);

  EXPECT_EQ(udR_Success, udFileExists(pFilename));
  EXPECT_EQ(udR_Success, udFileDelete(pFilename));
  EXPECT_NE(udR_Success, udFileExists(pFilename));

  udCrypto_Deinit();
}

static char s_customFileHandler_buffer[32];
udResult udFileTests_CustomFileHandler_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags /*flags*/)
{
  udFile *pFile = nullptr;
  udResult result;

  pFile = udAllocType(udFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  result = udFileExists(pFilename, &pFile->fileLength);
  if (result != udR_Success)
    pFile->fileLength = 0;

  pFile->fpRead = [](udFile *, void *pBuffer, size_t, int64_t, size_t *pActualRead, udFilePipelinedRequest *) -> udResult
  {
    memcpy(pBuffer, s_customFileHandler_buffer, udStrlen(s_customFileHandler_buffer) + 1);
    if (pActualRead)
      *pActualRead = udStrlen(s_customFileHandler_buffer) + 1;
    return udR_Success;
  };
  pFile->fpWrite = [](udFile *, const void *pBuffer, size_t bufferLength, int64_t, size_t *pActualWritten) -> udResult
  {
    memcpy(s_customFileHandler_buffer, pBuffer, bufferLength);
    if (pActualWritten)
      *pActualWritten = bufferLength;
    return udR_Success;
  };
  pFile->fpRelease = [](udFile *)
  { return udR_Success; };
  pFile->fpClose = [](udFile **ppFile)
  {
    udFree(*ppFile);
    return udR_Success;
  };

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFree(pFile);

  return result;
}

TEST(udFileTests, CustomFileHandler)
{
  EXPECT_EQ(udR_Success, udFile_RegisterHandler(udFileTests_CustomFileHandler_Open, "CUSTOM:"));

  const char *pFilename = "CUSTOM://._donotcommit_CUSTOMtest";
  const char writeBuffer[] = "Testing!";

  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, writeBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Read));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer)));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char *pLoadBuffer = nullptr;
  EXPECT_EQ(udR_Success, udFile_Load(pFilename, (void **)&pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFile_DeregisterHandler(udFileTests_CustomFileHandler_Open));
  EXPECT_NE(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Write));
}

// Emscripten does not have a "home" directory concept
#if !UDPLATFORM_EMSCRIPTEN
TEST(udFileTests, TranslatingPaths)
{
  const char *pNewPath = nullptr;
  const char *pPath = "~";

  EXPECT_EQ(udR_Success, udFile_TranslatePath(&pNewPath, pPath));
  EXPECT_NE(nullptr, pNewPath);
#  if UDPLATFORM_WINDOWS
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "C:\\Users\\"));
#  elif UDPLATFORM_OSX
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/Users/"));
#  else
  // '/home/' for regular users and '/root' for the root user
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/home/") || udStrBeginsWith(pNewPath, "/root"));
#  endif
  udFree(pNewPath);

  // Test with additional path
  pPath = "~/test.file";
  EXPECT_EQ(udR_Success, udFile_TranslatePath(&pNewPath, pPath));
  EXPECT_NE(nullptr, pNewPath);
#  if UDPLATFORM_WINDOWS
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "C:\\Users\\"));
#  elif UDPLATFORM_OSX
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/Users/"));
#  else
  // '/home/' for regular users and '/root' for the root user
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/home/") || udStrBeginsWith(pNewPath, "/root"));
#  endif
  EXPECT_NE(nullptr, udStrstr(pNewPath, 0, "/test.file"));
  EXPECT_EQ(nullptr, udStrstr(pNewPath, 0, "//test.file"));
  udFree(pNewPath);

  // Test invalid translation
  pPath = "~MagicWindowsFile";
  EXPECT_NE(udR_Success, udFile_TranslatePath(&pNewPath, pPath));
  EXPECT_EQ(nullptr, pNewPath);

  // Test invalid input
  EXPECT_NE(udR_Success, udFile_TranslatePath(nullptr, pPath));
  EXPECT_NE(udR_Success, udFile_TranslatePath(&pNewPath, nullptr));
  EXPECT_NE(udR_Success, udFile_TranslatePath(nullptr, nullptr));
}
#endif

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
TEST(udFileTests, RawLoad)
{
  udResult result;
  void *pMemory = nullptr;
  int64_t len;

  result = udFile_Load("raw://SGVsbG8gV29ybGQ=", &pMemory, &len);
  EXPECT_EQ(udR_Success, result);
  EXPECT_EQ(11, len);
  EXPECT_STREQ("Hello World", (char *)pMemory); // Can do strcmp here because udFile_Load always adds a nul
  udFree(pMemory);

  udFile_Load(s_pQBF_Uncomp, &pMemory, &len);
  EXPECT_EQ(s_QBF_Len, (size_t)len);
  EXPECT_STREQ(s_pQBF_Text, (char *)pMemory);
  udFree(pMemory);

  udFile_Load(s_pQBF_RawDef, &pMemory, &len);
  EXPECT_EQ(s_QBF_Len, (size_t)len);
  EXPECT_STREQ(s_pQBF_Text, (char *)pMemory);
  udFree(pMemory);

  udFile_Load(s_pQBF_GzipDef, &pMemory, &len);
  EXPECT_EQ(s_QBF_Len, (size_t)len);
  EXPECT_STREQ(s_pQBF_Text, (char *)pMemory);
  udFree(pMemory);

  udFile_Load(s_pQBF_ZlibDef, &pMemory, &len);
  EXPECT_EQ(s_QBF_Len, (size_t)len);
  EXPECT_STREQ(s_pQBF_Text, (char *)pMemory);
  udFree(pMemory);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2019
TEST(udFileTests, RawWrite)
{
  udFile *pFile = nullptr;
  const char *pRawFilename = nullptr;
  int64_t length;
  size_t actualWritten;
  void *pReload = nullptr;

  // Files not specifically set up for writing should fail when opened for write
  ASSERT_EQ(udR_OpenFailure, udFile_Open(&pFile, s_pQBF_Uncomp, udFOF_Write));

  // Simple test writing in all compression modes
  for (udCompressionType ct = udCT_None; ct < udCT_Count; ct = (udCompressionType)(ct + 1))
  {
    ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawFilename, nullptr, 0, ct, "QBF Test", 200)); // 200 chars big enough for all compression modes
    ASSERT_EQ(udR_Success, udFile_Open(&pFile, pRawFilename, udFOF_Write, &length));
    EXPECT_STREQ(pRawFilename, udFile_GetFilename(pFile));
    EXPECT_EQ(0, length);
    EXPECT_EQ(udR_Success, udFile_Write(pFile, s_pQBF_Text, s_QBF_Len, 0, udFSW_SeekSet, &actualWritten));
    EXPECT_EQ(s_QBF_Len, actualWritten);
    EXPECT_EQ(udR_Success, udFile_Close(&pFile));
    EXPECT_EQ(udR_Success, udFile_Load(pRawFilename, &pReload, &length));
    EXPECT_EQ(s_QBF_Len, (size_t)length);
    EXPECT_EQ(0, memcmp(pReload, s_pQBF_Text, s_QBF_Len));
    udFree(pRawFilename);
    udFree(pReload);
  }

  // Test writing to a known-too-small buffer
  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawFilename, nullptr, 0, udCT_ZlibDeflate, "Buffer too small test", 158));
  EXPECT_EQ(udR_BufferTooSmall, udFile_Save(pRawFilename, s_pQBF_Text, s_QBF_Len));
  udFree(pRawFilename);

  // Test writing to a knownexact size buffer
  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pRawFilename, nullptr, 0, udCT_ZlibDeflate, "Buffer too small test", 159));
  EXPECT_EQ(udR_Success, udFile_Save(pRawFilename, s_pQBF_Text, s_QBF_Len));
  udFree(pRawFilename);
}

TEST(udFileTests, RecursiveCreateDirectoryTests)
{
  const char *pFilename = "./some/.hidden/testFile.txt";
  const char *pFilename2 = "./some/folder.name/subdir/testFile.txt";
  const char *pFilename3 = "~/folder/testFile.txt";
  const char *pOutput = "Test Output";
  udFile *pFile = nullptr;

  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename, udFOF_Create | udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, pOutput, udStrlen(pOutput)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  EXPECT_EQ(udR_Success, udFileExists(pFilename));
  EXPECT_EQ(udR_Success, udFileDelete(pFilename));
  EXPECT_NE(udR_Success, udFileExists(pFilename));

  EXPECT_EQ(udR_Success, udRemoveDir("./some/.hidden"));

  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename2, udFOF_Create | udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, pOutput, udStrlen(pOutput)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  EXPECT_EQ(udR_Success, udFileExists(pFilename2));
  EXPECT_EQ(udR_Success, udFileDelete(pFilename2));
  EXPECT_NE(udR_Success, udFileExists(pFilename2));

  EXPECT_EQ(udR_Success, udRemoveDir("./some/folder.name/subdir"));
  EXPECT_EQ(udR_Failure, udRemoveDir("./some"));
  EXPECT_EQ(udR_Success, udRemoveDir("./some/folder.name"));
  EXPECT_EQ(udR_Success, udRemoveDir("./some"));

  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFilename3, udFOF_Create | udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, pOutput, udStrlen(pOutput)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  EXPECT_EQ(udR_Success, udFileExists(pFilename3));
  EXPECT_EQ(udR_Success, udFileDelete(pFilename3));
  EXPECT_NE(udR_Success, udFileExists(pFilename3));

  EXPECT_EQ(udR_Success, udRemoveDir("~/folder"));

  int newFolders = 0;
  EXPECT_EQ(udR_Success, udCreateDir("./dir", &newFolders));
  EXPECT_EQ(1, newFolders);
  EXPECT_EQ(udR_Success, udCreateDir("./dir/two/more", &newFolders));
  EXPECT_EQ(2, newFolders);
  EXPECT_EQ(udR_Success, udRemoveDir("./dir/two/more", 2));
  EXPECT_EQ(udR_Success, udRemoveDir("./dir"));

  // Lots of folders should work
  EXPECT_EQ(udR_Success, udCreateDir("./dir/1/2/3/4/5/6/7/8/9", &newFolders));
  EXPECT_EQ(10, newFolders);
  EXPECT_EQ(udR_Success, udRemoveDir("./dir/1/2/3/4/5/6/7/8/9", 10));
  EXPECT_EQ(udR_NotFound, udFileExists("./dir"));

  // Empty directory should always succeed
  EXPECT_EQ(udR_Success, udCreateDir("", &newFolders));
  EXPECT_EQ(0, newFolders);

  // Creating a directory where a file already exists shouldn't infinite loop
  // this infinite loop also occurs when a user has insufficient permissions.
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, "./file", udFOF_Create));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));
  EXPECT_EQ(udR_Failure, udCreateDir("./file/dir"));
  EXPECT_EQ(udR_Success, udFileDelete("./file"));
}

TEST(udFileTests, DataLoad)
{
  udResult result;
  void *pMemory = nullptr;
  int64_t len;

  result = udFile_Load("data:,Hello%20World", &pMemory, &len);
  EXPECT_EQ(udR_Success, result);
  EXPECT_EQ(11, len);
  EXPECT_STREQ("Hello World", (char *)pMemory); // Can do strcmp here because udFile_Load always adds a nul
  udFree(pMemory);

  udFile_Load(s_pQBF_DataBase64, &pMemory, &len);
  EXPECT_EQ(s_QBF_Len, (size_t)len);
  EXPECT_STREQ(s_pQBF_Text, (char *)pMemory);
  udFree(pMemory);
}
