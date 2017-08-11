// Include any system headers that any files that are also included by code inside the namespace wrap
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "udCrypto.h"

#if UDPLATFORM_WINDOWS
#pragma warning(disable: 4267 4244)
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#endif

namespace udCrypto
{
  // Include public domain, license free source taken from https://github.com/B-Con/crypto-algorithms
#include "crypto/aes.c"
#include "crypto/sha1.c"
#undef ROTLEFT

#include "crypto/sha256.c"  // Include source directly and undef it's defines after
#undef ROTLEFT
#undef ROTRIGHT
#undef CH
#undef MAJ
#undef EP0
#undef EP1
#undef SIG0
#undef SIG1
};

struct udCryptoCipherContext
{
  udCrypto::WORD keySchedule[60];
  uint8_t nonce[AES_BLOCK_SIZE-8];
  int blockSize;
  int keyLengthInBits;
  udCryptoCiphers cipher;
  udCryptoChainMode chainMode;
  udCryptoPaddingMode padMode;
  bool nonceSet;
};

struct udCryptoHashContext
{
  udCryptoHashes hash;
  size_t hashLengthInBytes;
  union
  {
    udCrypto::SHA1_CTX sha1;
    udCrypto::SHA256_CTX sha256;
  };
};


// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_CreateCipher(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey, udCryptoChainMode chainMode)
{
  udResult result;
  udCryptoCipherContext *pCtx = nullptr;

  UD_ERROR_IF(ppCtx == nullptr || pKey == nullptr, udR_InvalidParameter_);

  pCtx = udAllocType(udCryptoCipherContext, 1, udAF_Zero);
  UD_ERROR_NULL(pCtx, udR_MemoryAllocationFailure);

  pCtx->cipher = cipher;
  pCtx->padMode = padMode;
  pCtx->chainMode = chainMode;

  switch (cipher)
  {
    case udCC_AES128:
      udCrypto::aes_key_setup(pKey, pCtx->keySchedule, 128);
      pCtx->blockSize = AES_BLOCK_SIZE;
      pCtx->keyLengthInBits = 128;
      break;
    case udCC_AES256:
      udCrypto::aes_key_setup(pKey, pCtx->keySchedule, 256);
      pCtx->blockSize = AES_BLOCK_SIZE;
      pCtx->keyLengthInBits = 256;
      break;
    default:
      result = udR_InvalidParameter_;
      goto epilogue;
  }

  // Give ownership of the context to the caller
  *ppCtx = pCtx;
  pCtx = nullptr;
  result = udR_Success;

epilogue:
  udFree(pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, July 2016
udResult udCrypto_SetNonce(udCryptoCipherContext *pCtx, const uint8_t *pNonce, int nonceLen)
{
  if (pCtx == nullptr || pNonce == nullptr)
    return udR_InvalidParameter_;
  if (pCtx->chainMode != udCCM_CTR)
    return udR_InvalidConfiguration;
  if (nonceLen < (AES_BLOCK_SIZE - 8))
    return udR_BufferTooSmall;
  memcpy(pCtx->nonce, pNonce, AES_BLOCK_SIZE - 8);
  pCtx->nonceSet = true;
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, July 2016
udResult udCrypto_CreateIVForCTRMode(udCryptoCipherContext *pCtx, uint8_t *pIV, int ivLen, uint64_t counter)
{
  if (pCtx == nullptr || pIV == nullptr)
    return udR_InvalidParameter_;
  if (pCtx->chainMode != udCCM_CTR || !pCtx->nonceSet)
    return udR_InvalidConfiguration;
  if (ivLen < AES_BLOCK_SIZE)
    return udR_BufferTooSmall;
  memcpy(pIV, pCtx->nonce, AES_BLOCK_SIZE - 8);
  // Very important for counter to be assigned big endian
  for (int i = 0; i < 8; ++i)
    pIV[8 + i] = (uint8_t)(counter >> ((7 - i) * 8));
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_Encrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, int ivLen, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen, uint8_t *pOutIV)
{
  udResult result;
  size_t paddedCliperTextLen;

  UD_ERROR_IF(!pCtx || !pPlainText || !pCipherText, udR_InvalidParameter_);

  paddedCliperTextLen = plainTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((plainTextLen % pCtx->blockSize) != 0)
      {
        result = udR_AlignmentRequirement;
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

  UD_ERROR_IF(paddedCliperTextLen < cipherTextLen, udR_BufferTooSmall);

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      {
        switch (pCtx->chainMode)
        {
          case udCCM_ECB:
            UD_ERROR_IF(pIV || ivLen || pOutIV, udR_InvalidParameter_);
            for (size_t i = 0; i < plainTextLen; i += pCtx->blockSize)
              udCrypto::aes_encrypt((udCrypto::BYTE*)pPlainText + i, (udCrypto::BYTE*)pCipherText + i, pCtx->keySchedule, pCtx->keyLengthInBits);
            break;
          case udCCM_CBC:
            UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE, udR_InvalidParameter_);
            if (!udCrypto::aes_encrypt_cbc((udCrypto::BYTE*)pPlainText, plainTextLen, (udCrypto::BYTE*)pCipherText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV))
              UD_ERROR_SET(udR_Failure_);
            // For CBC, the output IV is the last encrypted block
            if (pOutIV)
              memcpy(pOutIV, (udCrypto::BYTE*)pCipherText + paddedCliperTextLen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            break;
          case udCCM_CTR:
            UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE || pOutIV != nullptr, udR_InvalidParameter_); // Don't allow output IV in CTR mode (yet)
            udCrypto::aes_encrypt_ctr((udCrypto::BYTE*)pPlainText, plainTextLen, (udCrypto::BYTE*)pCipherText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV);
            break;
          default:
            UD_ERROR_SET(udR_InvalidConfiguration);
        }
      }
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

  if (pPaddedCipherTextLen)
    *pPaddedCipherTextLen = paddedCliperTextLen;
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_Decrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, int ivLen, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen, uint8_t *pOutIV)
{
  udResult result;
  size_t actualPlainTextLen;

  UD_ERROR_IF(!pCtx || !pPlainText || !pCipherText, udR_InvalidParameter_);

  actualPlainTextLen = cipherTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((cipherTextLen % pCtx->blockSize) != 0)
      {
        result = udR_AlignmentRequirement;
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

  UD_ERROR_IF(actualPlainTextLen < plainTextLen, udR_BufferTooSmall);

  switch (pCtx->cipher)
  {
  case udCC_AES128:
  case udCC_AES256:
  {
    switch (pCtx->chainMode)
    {
    case udCCM_ECB:
      UD_ERROR_IF(pIV || ivLen || pOutIV, udR_InvalidParameter_);
      for (size_t i = 0; i < cipherTextLen; i += pCtx->blockSize)
        udCrypto::aes_encrypt((udCrypto::BYTE*)pCipherText + i, (udCrypto::BYTE*)pPlainText + i, pCtx->keySchedule, pCtx->keyLengthInBits);
      break;
    case udCCM_CBC:
      UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE, udR_InvalidParameter_);
      // For CBC, the output IV is the last encrypted block
      if (pOutIV)
        memcpy(pOutIV, (udCrypto::BYTE*)pCipherText + cipherTextLen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
      if (!udCrypto::aes_decrypt_cbc((udCrypto::BYTE*)pCipherText, cipherTextLen, (udCrypto::BYTE*)pPlainText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV))
        UD_ERROR_SET(udR_Failure_);
      break;
    case udCCM_CTR:
      UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE || pOutIV != nullptr, udR_InvalidParameter_);
      udCrypto::aes_encrypt_ctr((udCrypto::BYTE*)pCipherText, cipherTextLen, (udCrypto::BYTE*)pPlainText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV);
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
    }
  }
  break;
  default:
    UD_ERROR_SET(udR_InvalidConfiguration);
  }

  if (pActualPlainTextLen)
    *pActualPlainTextLen = actualPlainTextLen;
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_DestroyCipher(udCryptoCipherContext **ppCtx)
{
  if (!ppCtx || !*ppCtx)
    return udR_InvalidParameter_;
  udFree(*ppCtx);
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_CreateHash(udCryptoHashContext **ppCtx, udCryptoHashes hash)
{
  udResult result;
  udCryptoHashContext *pCtx = nullptr;

  UD_ERROR_NULL(ppCtx, udR_InvalidParameter_);

  pCtx = udAllocType(udCryptoHashContext, 1, udAF_Zero);
  UD_ERROR_NULL(pCtx, udR_MemoryAllocationFailure);

  pCtx->hash = hash;
  switch (hash)
  {
    case udCH_SHA1:
      sha1_init(&pCtx->sha1);
      pCtx->hashLengthInBytes = SHA1_BLOCK_SIZE;
      break;
    case udCH_SHA256:
      sha256_init(&pCtx->sha256);
      pCtx->hashLengthInBytes = SHA256_BLOCK_SIZE;
      break;
    default:
      result = udR_InvalidParameter_;
      goto epilogue;
  }

  // Give ownership of the context to the caller
  *ppCtx = pCtx;
  pCtx = nullptr;
  result = udR_Success;

epilogue:
  udFree(pCtx);
  return result;

}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_Digest(udCryptoHashContext *pCtx, const void *pBytes, size_t length)
{
  if (!pCtx)
    return udR_InvalidParameter_;
  if (length && !pBytes)
    return udR_InvalidParameter_;

  switch (pCtx->hash)
  {
    case udCH_SHA1:
      sha1_update(&pCtx->sha1, (const uint8_t*)pBytes, length);
      break;
    case udCH_SHA256:
      sha256_update(&pCtx->sha256, (const uint8_t*)pBytes, length);
      break;
    default:
      return udR_InvalidParameter_;
  }
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_Finalise(udCryptoHashContext *pCtx, uint8_t *pHash, size_t length, size_t *pActualHashLength)
{
  if (!pCtx)
    return udR_InvalidParameter_;
  if (length < pCtx->hashLengthInBytes)
    return udR_BufferTooSmall;

  if (pActualHashLength)
    *pActualHashLength = pCtx->hashLengthInBytes;

  if (pHash)
  {
    if (length < pCtx->hashLengthInBytes)
      return udR_BufferTooSmall;

    switch (pCtx->hash)
    {
      case udCH_SHA1:
        sha1_final(&pCtx->sha1, pHash);
        break;
      case udCH_SHA256:
        sha256_final(&pCtx->sha256, pHash);
        break;
      default:
        return udR_InvalidParameter_;
    }
  }

  return udR_Success;

}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_DestroyHash(udCryptoHashContext **ppCtx)
{
  if (!ppCtx || !*ppCtx)
    return udR_InvalidParameter_;
  udFree(*ppCtx);
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCrypto_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, uint8_t *pHash, size_t hashLength,
                       size_t *pActualHashLength, const void *pMessage2, size_t message2Length)
{
  udResult result;
  udCryptoHashContext *pCtx = nullptr;

  UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, pMessage, messageLength));
  if (pMessage2 && message2Length)
  {
    // Include the optional 2nd part of the message.
    // Digesting 2 concatenated message parts is very common in various crypto functions
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, pMessage2, message2Length));
  }
  UD_ERROR_CHECK(udCrypto_Finalise(pCtx, pHash, hashLength, pActualHashLength));

epilogue:
  udCrypto_DestroyHash(&pCtx);
  return result;
}


// ***************************************************************************************
// Author: Dave Pevreal, September 2015
udResult udCrypto_KDF(const char *pPassword, uint8_t *pKey, int keyLen)
{
  // Derive a secure key from a password string, using the same algorithm used by the Windows APIs to maintain interoperability with Geoverse
  // See the remarks section of the MS documentation here: http://msdn.microsoft.com/en-us/library/aa379916(v=vs.85).aspx
  udResult result;
  udCryptoHashContext *pCtx = nullptr;
  uint8_t passPhraseDigest[32];
  size_t passPhraseDigestLen;
  unsigned char hashBuffer[64];
  unsigned char derivedKey[40];

  UD_ERROR_IF(!pPassword || !pKey, udR_InvalidParameter_);
  UD_ERROR_IF(keyLen > 40, udR_Unsupported); // Limit of 40 byte key length due to SHA1 use

                                             // Hash the pass phrase
  UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, "ud1971", 6)); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, pPassword, strlen(pPassword))); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCrypto_Finalise(pCtx, passPhraseDigest, sizeof(passPhraseDigest), &passPhraseDigestLen));
  UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));
  UD_ERROR_IF(passPhraseDigestLen != 20, udR_InternalError);

  // Create a buffer of constant 0x36 xor'd with pass phrase hash
  memset(hashBuffer, 0x36, 64);
  for (size_t i = 0; i < passPhraseDigestLen; i++)
    hashBuffer[i] ^= passPhraseDigest[i];

  // Hash the result again and this gives us the key
  UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
  UD_ERROR_CHECK(udCrypto_Finalise(pCtx, derivedKey, 20));
  UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

  // For keys greater than 20 bytes, do the same thing with a different constant for the next 20 bytes
  if (keyLen > 20)
  {
    memset(hashBuffer, 0x5C, 64);
    for (size_t i = 0; i < passPhraseDigestLen; i++)
      hashBuffer[i] ^= passPhraseDigest[i];

    // Hash the result again and this gives us the key
    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, udCH_SHA1));
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, derivedKey + 20, 20));
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));
  }

  memcpy(pKey, derivedKey, keyLen);
  result = udR_Success;

epilogue:
  udCrypto_DestroyHash(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2017
udResult udCrypto_RandomKey(uint8_t *pKey, int keyLen)
{
  udResult result;
  uint8_t hash[32];
  struct Entropy
  {
#if UDPLATFORM_WINDOWS
    DWORD processId, threadId, tickCount, computerNameLen, userNameLen;
    SYSTEMTIME sysTime;
    MEMORYSTATUS memStatus;
    POINT cursorPos;
    wchar_t computerName[128];
    wchar_t userName[128];
    LARGE_INTEGER perfCounter, frequency;
    DWORD diskInfo[4];
    SYSTEM_INFO sysInfo;
#else
    struct timespec ts1;
    int randFileHandle;
    uint8_t randomData[32];
#endif
    uint64_t uninitialisedData; // deliberately uninitialised
  } entropy;

  UD_ERROR_IF(!pKey || keyLen <= 0 || keyLen > 32, udR_InvalidParameter_);

#if UDPLATFORM_WINDOWS
  // For windows we gather some entropy so we don't need to depend on the Crypto libraries/DLLs and specific Windows versions
  entropy.processId = GetCurrentProcessId();
  entropy.threadId = GetCurrentThreadId();
  entropy.tickCount = GetTickCount();
  GetLocalTime(&entropy.sysTime);
  GlobalMemoryStatus(&entropy.memStatus);
  GetCursorPos(&entropy.cursorPos);
  entropy.computerNameLen = UDARRAYSIZE(entropy.computerName);
  GetComputerName(entropy.computerName, &entropy.computerNameLen);
  entropy.userNameLen = UDARRAYSIZE(entropy.userName);
  GetUserName(entropy.userName, &entropy.userNameLen);
  QueryPerformanceCounter(&entropy.perfCounter);
  QueryPerformanceFrequency(&entropy.frequency);
  GetDiskFreeSpace(L"c:\\", &entropy.diskInfo[0], &entropy.diskInfo[1], &entropy.diskInfo[2], &entropy.diskInfo[3]);
  GetSystemInfo(&entropy.sysInfo);
#else
  clock_gettime(CLOCK_MONOTONIC, &entropy.ts1);
  entropy.randFileHandle = open("/dev/random", O_RDONLY);
  read(entropy.randFileHandle, entropy.randomData, sizeof(entropy.randomData));
  close(entropy.randFileHandle);
#endif

  result = udCrypto_Hash(udCH_SHA256, &entropy, sizeof(entropy), hash, sizeof(hash));
  UD_ERROR_HANDLE();

  memcpy(pKey, hash, keyLen);
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCrypto_HMAC(udCryptoHashes hash, const uint8_t *pKey, size_t keyLen, const uint8_t *pMessage, size_t messageLength,
                       uint8_t *pHMAC, size_t hmacLength, size_t *pActualHMACLength)
{
  // See https://en.wikipedia.org/wiki/Hash-based_message_authentication_code
  udResult result;
  enum { blockSize = 64 };
  uint8_t key[blockSize];
  uint8_t opad[blockSize];
  uint8_t ipad[blockSize];
  uint8_t ipadHash[blockSize];
  size_t ipadHashLen;

  memset(key, 0, blockSize);
  if (keyLen > blockSize)
    udCrypto_Hash(hash, pKey, keyLen, key, blockSize);
  else
    memcpy(key, pKey, keyLen);

  for (int i = 0; i < blockSize; ++i)
  {
    opad[i] = 0x5c ^ key[i];
    ipad[i] = 0x36 ^ key[i];
  }

  // First hash the concat of ipad and the message
  UD_ERROR_CHECK(udCrypto_Hash(hash, ipad, blockSize, ipadHash, blockSize, &ipadHashLen, pMessage, messageLength));
  // Then hash the concat of the opad and the result of first hash
  UD_ERROR_CHECK(udCrypto_Hash(hash, opad, blockSize, pHMAC, hmacLength, pActualHMACLength, ipadHash, ipadHashLen));

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_TestCipher(udCryptoCiphers cipher)
{
  udResult result = udR_Failure_;
  udCryptoCipherContext *pCtx = nullptr;
  size_t actualCipherTextLen, actualPlainTextLen;

  uint8_t key[1][32] = {
    { 0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4 }
  };

  uint8_t plaintext[2][32] = {
    { 0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51 }
  };

  if (cipher != udCC_AES256) // Only have a test for one thing at the moment
    goto epilogue;

  // Test CBC mode
  {
    uint8_t ciphertext[2][32] = {
      { 0xf5,0x8c,0x4c,0x04,0xd6,0xe5,0xf1,0xba,0x77,0x9e,0xab,0xfb,0x5f,0x7b,0xfb,0xd6,0x9c,0xfc,0x4e,0x96,0x7e,0xdb,0x80,0x8d,0x67,0x9f,0x77,0x7b,0xc6,0x70,0x2c,0x7d }
    };
    uint8_t iv[1][16] = {
      { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f }
    };
    result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0], udCCM_CBC);
    UD_ERROR_HANDLE();
    result = udCrypto_Encrypt(pCtx, iv[0], sizeof(iv[0]), plaintext[0], sizeof(plaintext[0]), ciphertext[1], sizeof(ciphertext[1]), &actualCipherTextLen);
    UD_ERROR_HANDLE();
    result = udCrypto_DestroyCipher(&pCtx);
    UD_ERROR_HANDLE();
    if (memcmp(ciphertext[0], ciphertext[1], sizeof(ciphertext[0])) != 0)
    {
      udDebugPrintf("Encrypt error CBC mode: ciphertext didn't match");
      result = udR_InternalError;
    }


    result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0], udCCM_CBC);
    UD_ERROR_HANDLE();
    result = udCrypto_Decrypt(pCtx, iv[0], sizeof(iv[0]), ciphertext[1], sizeof(ciphertext[1]), plaintext[1], sizeof(plaintext[1]), &actualPlainTextLen);
    UD_ERROR_HANDLE();
    result = udCrypto_DestroyCipher(&pCtx);
    UD_ERROR_HANDLE();
    if (memcmp(plaintext[0], plaintext[1], sizeof(plaintext[0])) != 0)
    {
      udDebugPrintf("Decrypt error CBC mode: plaintext didn't match");
      result = udR_InternalError;
    }
  }

  // Test CTR mode
  {
    uint8_t ciphertext[2][32] = {
      { 0x60,0x1e,0xc3,0x13,0x77,0x57,0x89,0xa5,0xb7,0xa7,0xf5,0x04,0xbb,0xf3,0xd2,0x28,0xf4,0x43,0xe3,0xca,0x4d,0x62,0xb5,0x9a,0xca,0x84,0xe9,0x90,0xca,0xca,0xf5,0xc5 }
    };
    const uint8_t nonce[8] = { 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7 };
    const uint64_t counter = 0xf8f9fafbfcfdfeffULL;
    uint8_t iv[16];
    result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0], udCCM_CTR);
    UD_ERROR_HANDLE();
    result = udCrypto_SetNonce(pCtx, nonce, sizeof(nonce));
    UD_ERROR_HANDLE();
    result = udCrypto_CreateIVForCTRMode(pCtx, iv, sizeof(iv), counter);
    UD_ERROR_HANDLE();
    result = udCrypto_Encrypt(pCtx, iv, sizeof(iv), plaintext[0], sizeof(plaintext[0]), ciphertext[1], sizeof(ciphertext[1]), &actualCipherTextLen);
    UD_ERROR_HANDLE();
    result = udCrypto_DestroyCipher(&pCtx);
    UD_ERROR_HANDLE();
    if (memcmp(ciphertext[0], ciphertext[1], sizeof(ciphertext[0])) != 0)
    {
      udDebugPrintf("Encrypt error CTR mode: ciphertext didn't match");
      result = udR_InternalError;
    }


    result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0], udCCM_CTR);
    UD_ERROR_HANDLE();
    result = udCrypto_SetNonce(pCtx, nonce, sizeof(nonce));
    UD_ERROR_HANDLE();
    result = udCrypto_CreateIVForCTRMode(pCtx, iv, sizeof(iv), counter);
    UD_ERROR_HANDLE();
    result = udCrypto_Decrypt(pCtx, iv, sizeof(iv), ciphertext[1], sizeof(ciphertext[1]), plaintext[1], sizeof(plaintext[1]), &actualPlainTextLen);
    UD_ERROR_HANDLE();
    result = udCrypto_DestroyCipher(&pCtx);
    UD_ERROR_HANDLE();
    if (memcmp(plaintext[0], plaintext[1], sizeof(plaintext[0])) != 0)
    {
      udDebugPrintf("Decrypt error CTR mode: plaintext didn't match");
      result = udR_InternalError;
    }
  }

  result = udR_Success;

epilogue:
  udCrypto_DestroyCipher(&pCtx);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_TestHash(udCryptoHashes hash)
{
  udResult result = udR_Failure_;
  udCryptoHashContext *pCtx = nullptr;

  char text1[] = { "abc" };
  char text2[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };
  char text3[] = { "aaaaaaaaaa" };

  switch (hash)
  {
  case udCH_SHA1:
  {
    uint8_t hash1[SHA1_BLOCK_SIZE] = { 0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d };
    uint8_t hash2[SHA1_BLOCK_SIZE] = { 0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1 };
    uint8_t hash3[SHA1_BLOCK_SIZE] = { 0x34,0xaa,0x97,0x3c,0xd4,0xc4,0xda,0xa4,0xf6,0x1e,0xeb,0x2b,0xdb,0xad,0x27,0x31,0x65,0x34,0x01,0x6f };
    uint8_t buf[SHA1_BLOCK_SIZE];

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, text1, strlen(text1)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash1, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, text2, strlen(text2)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash2, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    for (int i = 0; i < 100000; ++i)
      UD_ERROR_CHECK(udCrypto_Digest(pCtx, text3, strlen(text3)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash3, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));
    result = udR_Success;
  }
  break;
  case udCH_SHA256:
  {
    uint8_t hash1[SHA256_BLOCK_SIZE] = { 0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
      0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad };
    uint8_t hash2[SHA256_BLOCK_SIZE] = { 0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
      0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1 };
    uint8_t hash3[SHA256_BLOCK_SIZE] = { 0xcd,0xc7,0x6e,0x5c,0x99,0x14,0xfb,0x92,0x81,0xa1,0xc7,0xe2,0x84,0xd7,0x3e,0x67,
      0xf1,0x80,0x9a,0x48,0xa4,0x97,0x20,0x0e,0x04,0x6d,0x39,0xcc,0xc7,0x11,0x2c,0xd0 };
    uint8_t buf[SHA256_BLOCK_SIZE];

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, text1, strlen(text1)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash1, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    UD_ERROR_CHECK(udCrypto_Digest(pCtx, text2, strlen(text2)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash2, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

    UD_ERROR_CHECK(udCrypto_CreateHash(&pCtx, hash));
    for (int i = 0; i < 100000; ++i)
      UD_ERROR_CHECK(udCrypto_Digest(pCtx, text3, strlen(text3)));
    UD_ERROR_CHECK(udCrypto_Finalise(pCtx, buf, sizeof(buf)));
    UD_ERROR_IF(memcmp(hash3, buf, pCtx->hashLengthInBytes) != 0, udR_Failure_);
    UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));
    result = udR_Success;
  }
  break;
  default:
    result = udR_Failure_;
  }

epilogue:
  udCrypto_DestroyHash(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCrypto_TestHMAC(udCryptoHashes hash)
{
  udResult result;
  const char *pKey = "key";
  const char *pMessage = "The quick brown fox jumps over the lazy dog";
  uint8_t hmac[32];
  size_t hmacLength;
  static const uint8_t sha1Result[] =
  {
    0xde,0x7c,0x9b,0x85,0xb8,0xb7,0x8a,0xa6,0xbc,0x8a,0x7a,0x36,0xf7,0x0a,0x90,0x70,0x1c,0x9d,0xb4,0xd9
  };
  static const uint8_t sha256Result[] =
  {
    0xf7,0xbc,0x83,0xf4,0x30,0x53,0x84,0x24,0xb1,0x32,0x98,0xe6,0xaa,0x6f,0xb1,0x43,0xef,0x4d,0x59,0xa1,0x49,0x46,0x17,0x59,0x97,0x47,0x9d,0xbc,0x2d,0x1a,0x3c,0xd8
  };

  result = udCrypto_HMAC(hash, (const uint8_t*)pKey, strlen(pKey), (const uint8_t*)pMessage, strlen(pMessage), hmac, sizeof(hmac), &hmacLength);
  UD_ERROR_HANDLE();
  switch (hash)
  {
    case udCH_SHA1:
      UD_ERROR_IF(hmacLength != sizeof(sha1Result), udR_Failure_);
      UD_ERROR_IF(memcmp(sha1Result, hmac, hmacLength) != 0, udR_Failure_);
      break;
    case udCH_SHA256:
      UD_ERROR_IF(hmacLength != sizeof(sha256Result), udR_Failure_);
      UD_ERROR_IF(memcmp(sha256Result, hmac, hmacLength) != 0, udR_Failure_);
      break;
    default:
      UD_ERROR_SET(udR_InvalidParameter_);
  }

epilogue:
  return result;
}

