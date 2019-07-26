#include "gtest/gtest.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udCrypto.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udMath.h"

// ----------------------------------------------------------------------------
// Author: Paul Fox, July 2017
TEST(udFileTests, GeneralFileTests)
{
  const char *pFileName = "._donotcommit";
  EXPECT_NE(udR_Success, udFileExists(pFileName));

  int64_t currentTime = udGetEpochSecsUTCd();

  udFile *pFile;
  if (udFile_Open(&pFile, pFileName, udFOF_Write) == udR_Success)
  {
    udFile_Write(pFile, "TEST", 4);
    udFile_Close(&pFile);
  }

  int64_t size = 0;
  int64_t modifyTime = 0;

  EXPECT_EQ(udR_Success, udFileExists(pFileName, &size, &modifyTime));

  EXPECT_EQ(4, size);
  EXPECT_GT(2, udAbs(currentTime - modifyTime));

  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
  EXPECT_EQ(udR_ObjectNotFound, udFileExists(pFileName));
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
  const char *pFileName = "._donotcommit_FILEtest";
  const char writeBuffer[] = "Testing!";
  EXPECT_NE(udR_Success, udFileExists(pFileName));

  // Write
  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, writeBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  // Read
  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Read));
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
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Read));
  udFile_SetSeekBase(pFile, 1);
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer) - 1));
  EXPECT_STREQ(&writeBuffer[1], readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  // Load
  char *pLoadBuffer = nullptr;
  EXPECT_EQ(udR_Success, udFile_Load(pFileName, (void**)&pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFile_Load(pFileName, &pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFileExists(pFileName));
  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
  EXPECT_NE(udR_Success, udFileExists(pFileName));
}

TEST(udFileTests, EncryptedReadWriteFILE)
{
  udCrypto_Init();

  const char *pFileName = "._donotcommit_EncryptedFILEtest";
  const char writeBuffer[] = "Testing!asdfasdfasdfasdfasdfasd";
  char cipherBuffer[UDARRAYSIZE(writeBuffer)];
  uint8_t *pKey = nullptr;
  size_t keyLen = 0;
  const char *pKeyBase64 = nullptr;
  ASSERT_EQ(udR_Success, udCryptoKey_DeriveFromRandom(&pKeyBase64, udCCKL_AES256KeyLength));
  EXPECT_EQ(udR_Success, udBase64Decode(&pKey, &keyLen, pKeyBase64));
  EXPECT_NE(udR_Success, udFileExists(pFileName));

  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Write));
  udCryptoCipherContext *pCipherCtx;
  EXPECT_EQ(udR_Success, udCryptoCipher_Create(&pCipherCtx, udCC_AES256, udCPM_None, pKeyBase64, udCCM_CTR));
  udCryptoIV iv;
  EXPECT_EQ(udR_Success, udCrypto_CreateIVForCTRMode(pCipherCtx, &iv, 12, 0));
  EXPECT_EQ(udR_Success, udCryptoCipher_Encrypt(pCipherCtx, &iv, writeBuffer, UDARRAYSIZE(writeBuffer), cipherBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pCipherCtx));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, cipherBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Read));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer)));
  EXPECT_STRNE(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_SetEncryption(pFile, pKey, (int)keyLen, 12));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer), 0, udFSW_SeekSet));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  udFree(pKey);
  udFree(pKeyBase64);

  EXPECT_EQ(udR_Success, udFileExists(pFileName));
  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
  EXPECT_NE(udR_Success, udFileExists(pFileName));

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

  pFile->fpRead = [](udFile *, void *pBuffer, size_t , int64_t , size_t *pActualRead, udFilePipelinedRequest *) -> udResult {
    memcpy(pBuffer, s_customFileHandler_buffer, udStrlen(s_customFileHandler_buffer) + 1);
    if (pActualRead)
      *pActualRead = udStrlen(s_customFileHandler_buffer) + 1;
    return udR_Success;
  };
  pFile->fpWrite = [](udFile *, const void *pBuffer, size_t bufferLength, int64_t , size_t *pActualWritten) -> udResult {
    memcpy(s_customFileHandler_buffer, pBuffer, bufferLength);
    if (pActualWritten)
      *pActualWritten = bufferLength;
    return udR_Success;
  };
  pFile->fpRelease = [](udFile *) { return udR_Success; };
  pFile->fpClose = [](udFile **ppFile) { udFree(*ppFile); return udR_Success; };

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

  const char *pFileName = "CUSTOM://._donotcommit_CUSTOMtest";
  const char writeBuffer[] = "Testing!";

  udFile *pFile = nullptr;
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Write));
  EXPECT_EQ(udR_Success, udFile_Write(pFile, writeBuffer, UDARRAYSIZE(writeBuffer)));
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char readBuffer[UDARRAYSIZE(writeBuffer)];
  EXPECT_EQ(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Read));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer)));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  char *pLoadBuffer = nullptr;
  EXPECT_EQ(udR_Success, udFile_Load(pFileName, (void**)&pLoadBuffer));
  EXPECT_STREQ(writeBuffer, pLoadBuffer);
  udFree(pLoadBuffer);

  EXPECT_EQ(udR_Success, udFile_DeregisterHandler(udFileTests_CustomFileHandler_Open));
  EXPECT_NE(udR_Success, udFile_Open(&pFile, pFileName, udFOF_Write));
}

// Emscripten does not have a "home" directory concept
#if !UDPLATFORM_EMSCRIPTEN
TEST(udFileTests, TranslatingPaths)
{
  const char *pNewPath = nullptr;
  const char *pPath = "~";

  EXPECT_EQ(udR_Success, udFile_TranslatePath(&pNewPath, pPath));
  EXPECT_NE(nullptr, pNewPath);
#if UDPLATFORM_WINDOWS
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "C:\\Users\\"));
#elif UDPLATFORM_OSX
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/Users/"));
#else
  // '/home/' for regular users and '/root' for the root user
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/home/") || udStrBeginsWith(pNewPath, "/root"));
#endif
  udFree(pNewPath);

  // Test with additional path
  pPath = "~/test.file";
  EXPECT_EQ(udR_Success, udFile_TranslatePath(&pNewPath, pPath));
  EXPECT_NE(nullptr, pNewPath);
#if UDPLATFORM_WINDOWS
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "C:\\Users\\"));
#elif UDPLATFORM_OSX
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/Users/"));
#else
  // '/home/' for regular users and '/root' for the root user
  EXPECT_TRUE(udStrBeginsWithi(pNewPath, "/home/") || udStrBeginsWith(pNewPath, "/root"));
#endif
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
TEST(udFileTests, raw)
{
  udResult result;
  void *pMemory = nullptr;
  int64_t len;

  result = udFile_Load("raw://SGVsbG8gV29ybGQ=", &pMemory, &len);
  EXPECT_EQ(udR_Success, result);
  EXPECT_EQ(11, len);
  EXPECT_STREQ("Hello World", (char*)pMemory); // Can do strcmp here because udFile_Load always adds a nul
  udFree(pMemory);

  udFile_Load("raw://VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==", &pMemory, &len);
  EXPECT_STREQ("The quick brown fox jumps over the lazy dog", (char*)pMemory);
  udFree(pMemory);

  udFile_Load("raw://compression=RawDeflate,size=43@C8lIVSgszUzOVkgqyi/PU0jLr1D"
              "IKs0tKFbIL0stUigBSuckVlUqpOSnAwA=", &pMemory, &len);
  EXPECT_STREQ("The quick brown fox jumps over the lazy dog", (char*)pMemory);
  udFree(pMemory);

  udFile_Load("raw://compression=GzipDeflate,size=43@H4sIAAAAAAAA/wvJSFUoLM1Mzl"
              "ZIKsovz1NIy69QyCrNLShWyC9LLVIoAUrnJFZVKqTkpwMAOaNPQSsAAAA=", &pMemory, &len);
  EXPECT_STREQ("The quick brown fox jumps over the lazy dog", (char*)pMemory);
  udFree(pMemory);

  udFile_Load("raw://compression=ZlibDeflate,size=43@eJwLyUhVKCzNTM5WSCrKL89TSM"
              "uvUMgqzS0oVsgvSy1SKAFK5yRWVSqk5KcDAFvcD9o=", &pMemory, &len);
  EXPECT_STREQ("The quick brown fox jumps over the lazy dog", (char*)pMemory);
  udFree(pMemory);
}
