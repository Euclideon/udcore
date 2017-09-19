// Include any system headers that any files that are also included by code inside the namespace wrap
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "udCrypto.h"
#include "udValue.h"

#if UDPLATFORM_WINDOWS
#pragma warning(disable: 4267 4244)
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#endif

#include "mbedtls/aes.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"

#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

extern "C" int mbedtls_snprintf(char * s, size_t n, const char * format, ...)
{
  va_list ap;
  va_start(ap, format);
  int len = udSprintfVA(s, n, format, ap);
  va_end(ap);
  return len;
}

enum
{
  AES_BLOCK_SIZE = 16
};

#define TYPESTRING_RSAPRIVATE "RSA-Private"
#define TYPESTRING_RSAPUBLIC "RSA-Public"

struct udCryptoCipherContext
{
  mbedtls_aes_context ctx;
  uint8_t key[32];
  uint8_t iv[AES_BLOCK_SIZE];
  uint8_t nonce[AES_BLOCK_SIZE-8];
  int blockSize;
  int keyLengthInBits;
  udCryptoCiphers cipher;
  udCryptoChainMode chainMode;
  udCryptoPaddingMode padMode;
  bool nonceSet;
  bool ctxInit;
};

struct udCryptoHashContext
{
  udCryptoHashes hash;
  size_t hashLengthInBytes;
  union
  {
    mbedtls_sha1_context sha1;
    mbedtls_sha256_context sha256;
    mbedtls_sha512_context sha512;
  };
};

struct udCryptoSigContext
{
  udCryptoSigType type;
  union
  {
    mbedtls_rsa_context rsa;
  };
};


// ***************************************************************************************
// Author: Dave Pevreal, September 2017
// Helper to convert an mpi to base64
static const char *ToString(const mbedtls_mpi &bigNum)
{
  unsigned char *pBuf = nullptr;
  const char *pBase64 = nullptr;
  size_t bufLen = mbedtls_mpi_size(&bigNum);

  pBuf = udAllocType(unsigned char, bufLen, udAF_None);
  if (pBuf)
  {
    if (mbedtls_mpi_write_binary(&bigNum, pBuf, bufLen) == 0)
    {
      udBase64Encode(&pBase64, pBuf, bufLen);
    }
  }
  udFree(pBuf);
  return pBase64;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
// Helper to parse base64 to an mpi
static udResult FromString(mbedtls_mpi *pBigNum, const char *pBase64)
{
  udResult result;
  uint8_t *pBuf = nullptr;
  size_t bufLen;

  result = udBase64Decode(&pBuf, &bufLen, pBase64);
  UD_ERROR_HANDLE();
  UD_ERROR_IF(mbedtls_mpi_read_binary(pBigNum, pBuf, bufLen) != 0, udR_InternalCryptoError);
  result = udR_Success;

epilogue:
  udFree(pBuf);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_CreateCipher(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey, udCryptoChainMode chainMode)
{
  udResult result;
  udCryptoCipherContext *pCtx = nullptr;

  UD_ERROR_IF(ppCtx == nullptr || pKey == nullptr, udR_InvalidParameter_);

  pCtx = udAllocType(udCryptoCipherContext, 1, udAF_Zero);
  UD_ERROR_NULL(pCtx, udR_MemoryAllocationFailure);

  mbedtls_aes_init(&pCtx->ctx);
  pCtx->cipher = cipher;
  pCtx->padMode = padMode;
  pCtx->chainMode = chainMode;
  pCtx->blockSize = AES_BLOCK_SIZE;

  if (cipher == udCC_AES128)
  {
    pCtx->keyLengthInBits = 128;
  }
  else if (cipher == udCC_AES256)
  {
    pCtx->keyLengthInBits = 256;
  }
  else
  {
    UD_ERROR_SET(udR_InvalidParameter_);
  }
  memcpy(pCtx->key, pKey, pCtx->keyLengthInBits / 8);
  pCtx->ctxInit = false;

  // Give ownership of the context to the caller
  *ppCtx = pCtx;
  pCtx = nullptr;
  result = udR_Success;

epilogue:
  if (pCtx)
  {
    mbedtls_aes_free(&pCtx->ctx);
    udFree(pCtx);
  }
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
udResult udCrypto_Encrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen, uint8_t *pOutIV)
{
  udResult result;
  size_t paddedCliperTextLen;

  UD_ERROR_IF(!pCtx || !pPlainText || !pCipherText, udR_InvalidParameter_);

  if (!pCtx->ctxInit)
  {
    mbedtls_aes_setkey_enc(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits);
    pCtx->ctxInit = true;
  }

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

  UD_ERROR_IF(paddedCliperTextLen > cipherTextLen, udR_BufferTooSmall);

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      {
        switch (pCtx->chainMode)
        {
          case udCCM_CBC:
            UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE, udR_InvalidParameter_);
            memcpy(pCtx->iv, pIV, ivLen);
            if (mbedtls_aes_crypt_cbc(&pCtx->ctx, MBEDTLS_AES_ENCRYPT, plainTextLen, pCtx->iv, (const unsigned char*)pPlainText, (unsigned char*)pCipherText) != 0)
            {
              UD_ERROR_SET(udR_Failure_);
            }
            // For CBC, the output IV is the last encrypted block
            if (pOutIV)
              memcpy(pOutIV, (char*)pCipherText + paddedCliperTextLen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            break;

          case udCCM_CTR:
            UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE || pOutIV != nullptr, udR_InvalidParameter_); // Don't allow output IV in CTR mode (yet)
            memcpy(pCtx->iv, pIV, ivLen);
            {
              size_t ncoff = 0;
              unsigned char stream_block[16];
              if (mbedtls_aes_crypt_ctr(&pCtx->ctx, plainTextLen, &ncoff, pCtx->iv, stream_block, (const unsigned char*)pPlainText, (unsigned char *)pCipherText) != 0)
              {
                UD_ERROR_SET(udR_Failure_);
              }
            }
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
udResult udCrypto_Decrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen, uint8_t *pOutIV)
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

  UD_ERROR_IF(actualPlainTextLen > plainTextLen, udR_BufferTooSmall);

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
    {
      switch (pCtx->chainMode)
      {
        case udCCM_CBC:
          UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE, udR_InvalidParameter_);
          if (!pCtx->ctxInit)
          {
            mbedtls_aes_setkey_dec(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits);
            pCtx->ctxInit = true;
          }
          memcpy(pCtx->iv, pIV, ivLen);
          if (mbedtls_aes_crypt_cbc(&pCtx->ctx, MBEDTLS_AES_DECRYPT, cipherTextLen, pCtx->iv, (const unsigned char*)pCipherText, (unsigned char *)pPlainText) != 0)
          {
            UD_ERROR_SET(udR_Failure_);
          }
          if (pOutIV)
            memcpy(pOutIV, pCtx->iv, AES_BLOCK_SIZE);
          break;

        case udCCM_CTR:
          UD_ERROR_IF(pIV == nullptr || ivLen != AES_BLOCK_SIZE || pOutIV != nullptr, udR_InvalidParameter_); // Don't allow output IV in CTR mode (yet)
          if (!pCtx->ctxInit)
          {
            mbedtls_aes_setkey_enc(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits); // NOTE: Using ENCRYPT key schedule for CTR mode
            pCtx->ctxInit = true;
          }
          memcpy(pCtx->iv, pIV, ivLen);
          {
            size_t ncoff = 0;
            unsigned char stream_block[16];
            if (mbedtls_aes_crypt_ctr(&pCtx->ctx, cipherTextLen, &ncoff, pCtx->iv, stream_block, (const unsigned char*)pCipherText, (unsigned char *)pPlainText) != 0)
            {
              UD_ERROR_SET(udR_Failure_);
            }
          }
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
  mbedtls_aes_free(&(*ppCtx)->ctx);
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
      mbedtls_sha1_init(&pCtx->sha1);
      mbedtls_sha1_starts(&pCtx->sha1);
      pCtx->hashLengthInBytes = udCHL_SHA1Length;
      break;
    case udCH_SHA256:
      mbedtls_sha256_init(&pCtx->sha256);
      mbedtls_sha256_starts(&pCtx->sha256, 0);
      pCtx->hashLengthInBytes = udCHL_SHA256Length;
      break;
    case udCH_SHA512:
      mbedtls_sha512_init(&pCtx->sha512);
      mbedtls_sha512_starts(&pCtx->sha512, 0);
      pCtx->hashLengthInBytes = udCHL_SHA512Length;
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
      mbedtls_sha1_update(&pCtx->sha1, (const uint8_t*)pBytes, length);
      break;
    case udCH_SHA256:
      mbedtls_sha256_update(&pCtx->sha256, (const uint8_t*)pBytes, length);
      break;
    case udCH_SHA512:
      mbedtls_sha512_update(&pCtx->sha512, (const uint8_t*)pBytes, length);
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
        mbedtls_sha1_finish(&pCtx->sha1, pHash);
        break;
      case udCH_SHA256:
        mbedtls_sha256_finish(&pCtx->sha256, pHash);
        break;
      case udCH_SHA512:
        mbedtls_sha512_finish(&pCtx->sha512, pHash);
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
  switch ((*ppCtx)->hash)
  {
    case udCH_SHA1:
      mbedtls_sha1_free(&(*ppCtx)->sha1);
      break;
    case udCH_SHA256:
      mbedtls_sha256_free(&(*ppCtx)->sha256);
      break;
    case udCH_SHA512:
      mbedtls_sha512_free(&(*ppCtx)->sha512);
      break;
    default:
      break;
  }
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
  UD_ERROR_IF(passPhraseDigestLen != 20, udR_InternalCryptoError);

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
  // TEMPORARY. TODO: Use mbedTLS entropy
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
      result = udR_InternalCryptoError;
      UD_ERROR_HANDLE();
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
      UD_ERROR_SET(udR_InternalCryptoError);
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
      UD_ERROR_SET(udR_InternalCryptoError);
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
      UD_ERROR_SET(udR_InternalCryptoError);
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
    uint8_t hash1[udCHL_SHA1Length] = { 0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d };
    uint8_t hash2[udCHL_SHA1Length] = { 0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1 };
    uint8_t hash3[udCHL_SHA1Length] = { 0x34,0xaa,0x97,0x3c,0xd4,0xc4,0xda,0xa4,0xf6,0x1e,0xeb,0x2b,0xdb,0xad,0x27,0x31,0x65,0x34,0x01,0x6f };
    uint8_t buf[udCHL_SHA1Length];

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
    uint8_t hash1[udCHL_SHA256Length] = { 0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
      0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad };
    uint8_t hash2[udCHL_SHA256Length] = { 0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
      0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1 };
    uint8_t hash3[udCHL_SHA256Length] = { 0xcd,0xc7,0x6e,0x5c,0x99,0x14,0xfb,0x92,0x81,0xa1,0xc7,0xe2,0x84,0xd7,0x3e,0x67,
      0xf1,0x80,0x9a,0x48,0xa4,0x97,0x20,0x0e,0x04,0x6d,0x39,0xcc,0xc7,0x11,0x2c,0xd0 };
    uint8_t buf[udCHL_SHA256Length];

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

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCrypto_CreateSigKey(udCryptoSigContext **ppSigCtx, udCryptoSigType type)
{
  udResult result = udR_Failure_;
  int mbErr;
  udCryptoSigContext *pSigCtx = nullptr;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;

  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);
  pSigCtx = udAllocType(udCryptoSigContext, 1, udAF_Zero);
  UD_ERROR_NULL(pSigCtx, udR_MemoryAllocationFailure);
  pSigCtx->type = type;

  // Seed the DRGB and add this function name as an optional personalisation string
  mbErr = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)__FUNCTION__, sizeof(__FUNCTION__));
  UD_ERROR_IF(mbErr, udR_InternalCryptoError);

  switch (type)
  {
    case udCST_RSA2048:
    case udCST_RSA4096:
      mbedtls_rsa_init(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      mbErr = mbedtls_rsa_gen_key(&pSigCtx->rsa, mbedtls_ctr_drbg_random, &ctr_drbg, type, 65537);
      UD_ERROR_IF(mbErr, udR_InternalCryptoError);
      break;
    default:
      UD_ERROR_SET(udR_InvalidParameter_);
  }

  *ppSigCtx = pSigCtx;
  pSigCtx = nullptr;
  result = udR_Success;

epilogue:
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  udCrypto_DestroySig(&pSigCtx);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCrypto_ExportSigKey(udCryptoSigContext *pSigCtx, const char **ppKeyText, bool exportPrivate)
{
  udResult result;
  udValue v;
  const char *p;

  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(ppKeyText, udR_InvalidParameter_);

  switch (pSigCtx->type)
  {
  case udCST_RSA2048:
  case udCST_RSA4096:
    v.Set("Type = '%s'", exportPrivate ? TYPESTRING_RSAPRIVATE : TYPESTRING_RSAPUBLIC);
    v.Set("Size = %d", pSigCtx->type);
    p = ToString(pSigCtx->rsa.N); v.Set("N = '%s'", p); udFree(p);
    p = ToString(pSigCtx->rsa.E); v.Set("E = '%s'", p); udFree(p);
    if (exportPrivate)
    {
      p = ToString(pSigCtx->rsa.D); v.Set("D = '%s'", p); udFree(p);
      p = ToString(pSigCtx->rsa.P); v.Set("P = '%s'", p); udFree(p);
      p = ToString(pSigCtx->rsa.Q); v.Set("Q = '%s'", p); udFree(p);
      p = ToString(pSigCtx->rsa.DP); v.Set("DP = '%s'", p); udFree(p);
      p = ToString(pSigCtx->rsa.DQ); v.Set("DQ = '%s'", p); udFree(p);
      p = ToString(pSigCtx->rsa.QP); v.Set("QP = '%s'", p); udFree(p);
    }
    break;
  default:
    UD_ERROR_SET(udR_InvalidConfiguration);
  }

  result = v.Export(ppKeyText);

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCrypto_ImportSigKey(udCryptoSigContext **ppSigCtx, const char *pKeyText)
{
  udResult result;
  udValue v;
  udCryptoSigContext *pSigCtx = nullptr;

  result = v.Parse(pKeyText);
  UD_ERROR_HANDLE();
  pSigCtx = udAllocType(udCryptoSigContext, 1, udAF_Zero);
  UD_ERROR_NULL(pSigCtx, udR_MemoryAllocationFailure);
  pSigCtx->type = (udCryptoSigType)v.Get("Size").AsInt();
  switch (pSigCtx->type)
  {
    case udCST_RSA2048:
    case udCST_RSA4096:
      mbedtls_rsa_init(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      pSigCtx->rsa.len = pSigCtx->type / 8; // Size of the key
      UD_ERROR_CHECK(FromString(&pSigCtx->rsa.N, v.Get("N").AsString()));
      UD_ERROR_CHECK(FromString(&pSigCtx->rsa.E, v.Get("E").AsString()));
      UD_ERROR_IF(mbedtls_rsa_check_pubkey(&pSigCtx->rsa) != 0, udR_InternalCryptoError);
      if (udStrEqual(v.Get("Type").AsString(), TYPESTRING_RSAPRIVATE))
      {
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.D, v.Get("D").AsString()));
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.P, v.Get("P").AsString()));
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.Q, v.Get("Q").AsString()));
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.DP, v.Get("DP").AsString()));
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.DQ, v.Get("DQ").AsString()));
        UD_ERROR_CHECK(FromString(&pSigCtx->rsa.QP, v.Get("QP").AsString()));
        UD_ERROR_IF(mbedtls_rsa_check_privkey(&pSigCtx->rsa) != 0, udR_InternalCryptoError);
      }
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

  *ppSigCtx = pSigCtx;
  pSigCtx = nullptr;
  result = udR_Success;

epilogue:
  udCrypto_DestroySig(&pSigCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCrypto_Sign(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char **ppSignatureString, udCryptoSigPadScheme pad)
{
  udResult result = udR_Failure_;
  unsigned char signature[udCST_RSA4096/8];

  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pHash, udR_InvalidParameter_);
  UD_ERROR_NULL(ppSignatureString, udR_InvalidParameter_);

  switch (pSigCtx->type)
  {
    case udCST_RSA2048:
    case udCST_RSA4096:
      if (pad == udCSPS_Deterministic)
        mbedtls_rsa_set_padding(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      UD_ERROR_IF(sizeof(signature) < (pSigCtx->type / 8), udR_InternalCryptoError);
      UD_ERROR_IF(mbedtls_rsa_rsassa_pkcs1_v15_sign(&pSigCtx->rsa, NULL, NULL, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_NONE, hashLen, (const unsigned char*)pHash, signature) != 0, udR_InternalCryptoError);
      result = udBase64Encode(ppSignatureString, signature, pSigCtx->type / 8);
      UD_ERROR_HANDLE();
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCrypto_VerifySig(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char *pSignatureString, udCryptoSigPadScheme pad)
{
  udResult result = udR_Failure_;
  unsigned char signature[udCST_RSA4096 / 8];
  size_t sigLen;

  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pHash, udR_InvalidParameter_);
  UD_ERROR_NULL(pSignatureString, udR_InvalidParameter_);

  switch (pSigCtx->type)
  {
    case udCST_RSA2048:
    case udCST_RSA4096:
      if (pad == udCSPS_Deterministic)
        mbedtls_rsa_set_padding(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      UD_ERROR_IF(sizeof(signature) < (pSigCtx->type / 8), udR_InternalCryptoError);
      UD_ERROR_CHECK(udBase64Decode(pSignatureString, 0, signature, sizeof(signature), &sigLen));
      UD_ERROR_IF(mbedtls_rsa_rsassa_pkcs1_v15_verify(&pSigCtx->rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, hashLen, (const unsigned char*)pHash, signature) != 0, udR_SignatureMismatch);
      UD_ERROR_IF(sigLen != (pSigCtx->type / 8), udR_InternalCryptoError);
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
void udCrypto_DestroySig(udCryptoSigContext **ppSigCtx)
{
  if (ppSigCtx)
  {
    udCryptoSigContext *pSigCtx = *ppSigCtx;
    if (pSigCtx)
    {
      *ppSigCtx = nullptr;
      mbedtls_rsa_free(&pSigCtx->rsa);
      memset(pSigCtx, 0, sizeof(*pSigCtx)); // zero to ensure no remnants
      udFree(pSigCtx);
    }
  }
}

