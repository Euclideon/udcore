#include "gtest/gtest.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udCrypto.h"
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
  EXPECT_NE(udR_Success, udFileExists(pFileName));
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

  EXPECT_EQ(udR_Success, udFileExists(pFileName));
  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
  EXPECT_NE(udR_Success, udFileExists(pFileName));
}

TEST(udFileTests, EncryptedReadWriteFILE)
{
  const char *pFileName = "._donotcommit_EncryptedFILEtest";
  const char writeBuffer[] = "Testing!asdfasdfasdfasdfasdfasd";
  char cipherBuffer[UDARRAYSIZE(writeBuffer)];
  uint8_t *pKey = nullptr;
  size_t keyLen = 0;
  const char *pKeyBase64 = nullptr;
  udCryptoKey_DeriveFromRandom(&pKeyBase64, udCCKL_AES256KeyLength);
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
  EXPECT_EQ(udR_Success, udFile_SetEncryption(pFile, pKey, keyLen, 12));
  EXPECT_EQ(udR_Success, udFile_Read(pFile, readBuffer, UDARRAYSIZE(readBuffer), 0, udFSW_SeekSet));
  EXPECT_STREQ(writeBuffer, readBuffer);
  EXPECT_EQ(udR_Success, udFile_Close(&pFile));

  udFree(pKey);
  udFree(pKeyBase64);

  EXPECT_EQ(udR_Success, udFileExists(pFileName));
  EXPECT_EQ(udR_Success, udFileDelete(pFileName));
  EXPECT_NE(udR_Success, udFileExists(pFileName));
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
