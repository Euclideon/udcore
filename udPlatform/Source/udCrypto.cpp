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

#include "mbedtls/dhm.h"

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
// Helper to convert write an mpi to a udValue key as a base64 string
static udResult ToValue(const mbedtls_mpi &bigNum, udValue *pValue, const char *pKey)
{
  udResult result;
  unsigned char *pBuf = nullptr;
  const char *pBase64 = nullptr;
  size_t bufLen = mbedtls_mpi_size(&bigNum);

  pBuf = udAllocType(unsigned char, bufLen, udAF_None);
  UD_ERROR_NULL(pBuf, udR_MemoryAllocationFailure);

  UD_ERROR_IF(mbedtls_mpi_write_binary(&bigNum, pBuf, bufLen) != 0, udR_InternalCryptoError);
  result = udBase64Encode(&pBase64, pBuf, bufLen);
  UD_ERROR_HANDLE();
  result = pValue->Set("%s = '%s'", pKey, pBase64);

epilogue:
  udFree(pBuf);
  udFree(pBase64);
  return result;
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
udResult udCryptoCipher_Create(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey, udCryptoChainMode chainMode)
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
udResult udCryptoCipher_Encrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen, uint8_t *pOutIV)
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
udResult udCryptoCipher_Decrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen, uint8_t *pOutIV)
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
udResult udCryptoCipher_Destroy(udCryptoCipherContext **ppCtx)
{
  if (!ppCtx || !*ppCtx)
    return udR_InvalidParameter_;
  mbedtls_aes_free(&(*ppCtx)->ctx);
  udFree(*ppCtx);
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_SelfTest(udCryptoCiphers cipher)
{
  udResult result = udR_Failure_;
  udCryptoCipherContext *pCtx = nullptr;
  uint8_t key[udCCKL_MaxKeyLength];
  static const size_t keyLengths[udCC_Count] = { udCCKL_AES128KeyLength, udCCKL_AES256KeyLength };
  static const char *pPlainText = "There are two great days in every person's life; the day we are born and the day we discover why"; // Multiple of 16 characters
  static const uint8_t iv[16] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };
  static const uint8_t nonce[8] = { 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7 };
  static const uint64_t counter = 0xf8f9fafbfcfdfeffULL;
  static const uint8_t cipherTextCBC[udCC_Count][96] =
  {
    // AES128-CBC
    {
      0xe6,0x57,0x9a,0x39,0x2a,0x37,0xc7,0xed,0x29,0x27,0x25,0xfb,0xff,0xae,0x2d,0x58,
      0xcd,0xef,0x10,0xc2,0x56,0x95,0xe8,0x22,0xa0,0xc4,0x05,0xd8,0x2c,0x26,0x30,0x83,
      0xec,0x31,0xca,0x81,0x2a,0xe8,0xde,0x51,0x16,0xce,0xeb,0x48,0x78,0x3e,0x69,0x2c,
      0x3c,0xa2,0x00,0x7c,0x65,0x64,0x05,0x12,0x13,0xde,0xa9,0x69,0x6c,0x17,0x16,0x1e,
      0xf7,0xe6,0x62,0x50,0x96,0x87,0x4b,0x6f,0x80,0xb8,0x97,0x22,0x77,0xec,0xfd,0xa7,
      0x7a,0xaa,0x6a,0x6e,0x3b,0xbf,0xf2,0x1f,0x70,0x19,0x99,0x0b,0x1a,0x32,0xf9,0xa8
    },
    // AES256-CBC
    {
      0xb8,0x89,0xea,0xb4,0x22,0x19,0x93,0x4e,0x0c,0x4c,0xa3,0xf9,0x86,0x9a,0xe4,0x9e,
      0x82,0xf7,0x58,0x64,0xcf,0x90,0x14,0x77,0xc1,0xde,0x44,0x49,0xa4,0x4a,0xbd,0x31,
      0x63,0xd2,0x80,0x26,0x99,0x0a,0xf3,0xca,0xd9,0x92,0x5f,0x18,0x30,0x6e,0x51,0x02,
      0xc6,0x7c,0x60,0x55,0x5b,0xc4,0x25,0x1b,0xed,0xf8,0x55,0x5c,0x1f,0x8c,0x6a,0xbc,
      0x5c,0xdc,0x94,0xf7,0x83,0x2d,0x7e,0x1f,0xef,0x31,0xd4,0xb2,0xf1,0xfa,0x1e,0xf1,
      0x87,0x8d,0xf1,0xe4,0xee,0xde,0x0e,0x8b,0x6e,0xf0,0xe9,0x8a,0x32,0x02,0x0f,0xad
    }
  };
  static const uint8_t cipherTextCTR[udCC_Count][96] =
  {
    // AES128-CTR
    {
      0x44,0x90,0x81,0x06,0x60,0xf7,0x81,0x74,0x3c,0x1f,0x63,0x77,0x6b,0x7e,0xe5,0xd5,
      0x2c,0x73,0x60,0x99,0x4a,0xbe,0xa8,0xc5,0xa2,0xe4,0xb5,0xc7,0x61,0x43,0x6d,0x86,
      0x6e,0x07,0xa0,0x98,0xfa,0xa9,0x51,0x09,0xf7,0x21,0x78,0x35,0xb6,0x6b,0x7c,0x02,
      0xac,0x70,0x76,0x82,0xdc,0x39,0x1a,0x49,0x72,0xfb,0x0c,0x40,0x76,0x2d,0xac,0xeb,
      0xf9,0x27,0x99,0x9a,0x9c,0x9f,0x51,0xa2,0x3e,0x94,0x53,0xb5,0xf9,0x8d,0xa4,0x87,
      0x0d,0x5d,0x8b,0x67,0xe4,0x81,0xc4,0xed,0x53,0xa1,0x5c,0x92,0x0a,0x46,0xce,0x1d
    },
    // AES256-CTR
    {
      0x29,0xd6,0xac,0x22,0x6c,0x2d,0xba,0x7f,0x6a,0xd3,0x94,0x45,0x7a,0x24,0xe9,0x17,
      0x0d,0x4a,0x3d,0xf9,0x5f,0xd9,0xe0,0xd3,0x3a,0x36,0x77,0xb8,0xe2,0x3d,0x84,0xf0,
      0x8a,0x89,0x7c,0x53,0x7b,0x05,0x80,0xf7,0x6f,0x0c,0xbb,0x52,0x9d,0x14,0xba,0x69,
      0xa9,0xf4,0xc2,0xd4,0x72,0x96,0x4a,0x04,0x91,0x35,0x5f,0xa0,0xd6,0x6a,0xcd,0x5c,
      0xf9,0x82,0xb7,0xa4,0x6b,0x02,0xc1,0xbf,0x52,0xb9,0x68,0xe1,0x3f,0xae,0x81,0xac,
      0x36,0x2e,0xc0,0x0e,0x62,0x04,0xe4,0x0e,0x3b,0x1a,0x5f,0x06,0x7b,0x28,0xa4,0x99
    }
  };
  uint8_t ctrIV[16];
  uint8_t cipherText[128];
  uint8_t decryptedText[128];
  size_t actualCipherTextLen, actualDecryptedTextLen;

  UD_ERROR_IF(cipher < 0 || cipher >= udCC_Count, udR_InvalidParameter_);
  // Use a KDF to populate the key
  UD_ERROR_CHECK(udCryptoKey_DeriveFromPassword("password", key, keyLengths[cipher]));

  memset(cipherText, 0, sizeof(cipherText));
  memset(decryptedText, 0, sizeof(decryptedText));
  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, key, udCCM_CBC));
  UD_ERROR_CHECK(udCryptoCipher_Encrypt(pCtx, iv, sizeof(iv), pPlainText, udStrlen(pPlainText), cipherText, sizeof(cipherText), &actualCipherTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualCipherTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(cipherText, cipherTextCBC[cipher], actualCipherTextLen) != 0, udR_InternalCryptoError);

  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, key, udCCM_CBC));
  UD_ERROR_CHECK(udCryptoCipher_Decrypt(pCtx, iv, sizeof(iv), cipherText, actualCipherTextLen, decryptedText, sizeof(decryptedText), &actualDecryptedTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualDecryptedTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(pPlainText, decryptedText, actualDecryptedTextLen) != 0, udR_InternalCryptoError);

  actualCipherTextLen = actualDecryptedTextLen = 0;
  memset(cipherText, 0, sizeof(cipherText));
  memset(decryptedText, 0, sizeof(decryptedText));
  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, key, udCCM_CTR));
  UD_ERROR_CHECK(udCrypto_SetNonce(pCtx, nonce, sizeof(nonce)));
  UD_ERROR_CHECK(udCrypto_CreateIVForCTRMode(pCtx, ctrIV, sizeof(ctrIV), counter));
  UD_ERROR_CHECK(udCryptoCipher_Encrypt(pCtx, ctrIV, sizeof(ctrIV), pPlainText, udStrlen(pPlainText), cipherText, sizeof(cipherText), &actualCipherTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualCipherTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(cipherText, cipherTextCTR[cipher], actualCipherTextLen) != 0, udR_InternalCryptoError);

  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, key, udCCM_CTR));
  UD_ERROR_CHECK(udCrypto_SetNonce(pCtx, nonce, sizeof(nonce)));
  UD_ERROR_CHECK(udCrypto_CreateIVForCTRMode(pCtx, ctrIV, sizeof(ctrIV), counter));
  UD_ERROR_CHECK(udCryptoCipher_Decrypt(pCtx, ctrIV, sizeof(ctrIV), cipherText, actualCipherTextLen, decryptedText, sizeof(decryptedText), &actualDecryptedTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualDecryptedTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(pPlainText, decryptedText, actualDecryptedTextLen) != 0, udR_InternalCryptoError);

epilogue:
  udCryptoCipher_Destroy(&pCtx); // In case anything failed

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_Create(udCryptoHashContext **ppCtx, udCryptoHashes hash)
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
udResult udCryptoHash_Digest(udCryptoHashContext *pCtx, const void *pBytes, size_t length)
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
udResult udCryptoHash_Finalise(udCryptoHashContext *pCtx, uint8_t *pHash, size_t length, size_t *pActualHashLength)
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
udResult udCryptoHash_Destroy(udCryptoHashContext **ppCtx)
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
udResult udCryptoHash_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, uint8_t *pHash, size_t hashLength,
                       size_t *pActualHashLength, const void *pMessage2, size_t message2Length)
{
  udResult result;
  udCryptoHashContext *pCtx = nullptr;

  UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, hash));
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, pMessage, messageLength));
  if (pMessage2 && message2Length)
  {
    // Include the optional 2nd part of the message.
    // Digesting 2 concatenated message parts is very common in various crypto functions
    UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, pMessage2, message2Length));
  }
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, pHash, hashLength, pActualHashLength));

epilogue:
  udCryptoHash_Destroy(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCryptoHash_HMAC(udCryptoHashes hash, const void *pKey, size_t keyLen, const void *pMessage, size_t messageLength,
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
    udCryptoHash_Hash(hash, pKey, keyLen, key, blockSize);
  else
    memcpy(key, pKey, keyLen);

  for (int i = 0; i < blockSize; ++i)
  {
    opad[i] = 0x5c ^ key[i];
    ipad[i] = 0x36 ^ key[i];
  }

  // First hash the concat of ipad and the message
  UD_ERROR_CHECK(udCryptoHash_Hash(hash, ipad, blockSize, ipadHash, blockSize, &ipadHashLen, pMessage, messageLength));
  // Then hash the concat of the opad and the result of first hash
  UD_ERROR_CHECK(udCryptoHash_Hash(hash, opad, blockSize, pHMAC, hmacLength, pActualHMACLength, ipadHash, ipadHashLen));

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_SelfTest(udCryptoHashes hash)
{
  udResult result = udR_Failure_;

  static const char hmacKey[] = { "Jefe" };
  static const char testText[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };
  uint8_t resultBuf[udCHL_MaxHashLength];
  size_t resultLen;
  static const uint8_t hashResults[udCH_Count][udCHL_MaxHashLength] =
  {
    // udCH_SHA1
    { 0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1 },
    // udCH_SHA256
    { 0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
      0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1 },
    // udCH_SHA512
    { 0x20,0x4a,0x8f,0xc6,0xdd,0xa8,0x2f,0x0a,0x0c,0xed,0x7b,0xeb,0x8e,0x08,0xa4,0x16,
      0x57,0xc1,0x6e,0xf4,0x68,0xb2,0x28,0xa8,0x27,0x9b,0xe3,0x31,0xa7,0x03,0xc3,0x35,
      0x96,0xfd,0x15,0xc1,0x3b,0x1b,0x07,0xf9,0xaa,0x1d,0x3b,0xea,0x57,0x78,0x9c,0xa0,
      0x31,0xad,0x85,0xc7,0xa7,0x1d,0xd7,0x03,0x54,0xec,0x63,0x12,0x38,0xca,0x34,0x45 }
  };
  static const uint8_t hmacResults[udCH_Count][udCHL_MaxHashLength] =
  {
    // udCH_SHA1
    { 0x80,0xd4,0x88,0x4c,0x70,0xc5,0x34,0x06,0xe0,0xa7,0x07,0x1a,0x2e,0x21,0x35,0xc9,0x79,0x03,0xbf,0x49 },
    // udCH_SHA256
    { 0xde,0x34,0x44,0xcd,0x63,0x1f,0x7d,0x36,0x89,0xaf,0x1e,0xcc,0x13,0x19,0xe5,0x77,
      0x7c,0x03,0xe5,0x9a,0xe9,0xb0,0xd5,0xdd,0xdd,0x0a,0xc0,0x58,0x96,0x64,0xba,0x77 },
    // udCH_SHA512
    { 0x18,0xfa,0x29,0xa1,0x31,0xce,0xa9,0xa1,0x65,0xbf,0x3e,0xab,0x0b,0x5e,0x61,0xc8,
      0x0e,0xf3,0xf9,0x57,0xc7,0xea,0xfe,0x28,0x93,0x74,0x76,0x1b,0x43,0x91,0x9b,0x97,
      0x72,0x72,0xd5,0x0f,0x1a,0x4b,0xf1,0x0f,0xa0,0x1a,0x91,0xbb,0x5a,0xf4,0x9a,0x1d,
      0xa9,0x91,0xc5,0x2a,0xd7,0x38,0x34,0x12,0x03,0x16,0xf1,0x16,0xf8,0xd3,0x24,0x16 }
  };

  UD_ERROR_CHECK(udCryptoHash_Hash(hash, testText, strlen(testText), resultBuf, sizeof(resultBuf), &resultLen));
  UD_ERROR_IF(memcmp(hashResults[hash], resultBuf, resultLen) != 0, udR_Failure_);
  UD_ERROR_CHECK(udCryptoHash_HMAC(hash, hmacKey, sizeof(hmacKey), testText, strlen(testText), resultBuf, sizeof(resultBuf), &resultLen));
  UD_ERROR_IF(memcmp(hmacResults[hash], resultBuf, resultLen) != 0, udR_Failure_);

epilogue:
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

  result = udCryptoHash_HMAC(hash, pKey, strlen(pKey), pMessage, strlen(pMessage), hmac, sizeof(hmac), &hmacLength);
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
// Author: Dave Pevreal, September 2015
udResult udCryptoKey_DeriveFromPassword(const char *pPassword, void *pKey, size_t keyLen)
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
  UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, "ud1971", 6)); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, pPassword, strlen(pPassword))); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, passPhraseDigest, sizeof(passPhraseDigest), &passPhraseDigestLen));
  UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));
  UD_ERROR_IF(passPhraseDigestLen != 20, udR_InternalCryptoError);

  // Create a buffer of constant 0x36 xor'd with pass phrase hash
  memset(hashBuffer, 0x36, 64);
  for (size_t i = 0; i < passPhraseDigestLen; i++)
    hashBuffer[i] ^= passPhraseDigest[i];

  // Hash the result again and this gives us the key
  UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, derivedKey, 20));
  UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));

  // For keys greater than 20 bytes, do the same thing with a different constant for the next 20 bytes
  if (keyLen > 20)
  {
    memset(hashBuffer, 0x5C, 64);
    for (size_t i = 0; i < passPhraseDigestLen; i++)
      hashBuffer[i] ^= passPhraseDigest[i];

    // Hash the result again and this gives us the key
    UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
    UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
    UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, derivedKey + 20, 20));
    UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));
  }

  memcpy(pKey, derivedKey, keyLen);
  result = udR_Success;

epilogue:
  udCryptoHash_Destroy(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2017
udResult udCryptoKey_DeriveFromData(void *pKey, size_t keyLen, const void *pData, size_t dataLen)
{
  udResult result;
  uint8_t hashBuffer[udCHL_SHA512Length];
  size_t i;

  UD_ERROR_CHECK(udCryptoHash_Hash(udCH_SHA512, pData, dataLen, hashBuffer, sizeof(hashBuffer), &i));
  i = udMin(i, keyLen); // In case key length required is less than the hash
  memcpy(pKey, hashBuffer, i);
  // If the key required is longer than the hash, then re-hash the hash buffer
  while (i < keyLen)
  {
    pKey = udAddBytes(pKey, i);
    keyLen -= i;
    UD_ERROR_CHECK(udCryptoHash_Hash(udCH_SHA512, hashBuffer, sizeof(hashBuffer), hashBuffer, sizeof(hashBuffer), &i));
    i = udMin(i, keyLen); // In case secret length required is less than the hash
    memcpy(pKey, hashBuffer, i);
  }

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2017
udResult udCryptoKey_DeriveFromRandom(void *pKey, size_t keyLen)
{
  udResult result;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  uint8_t randomData[32];

  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);
  // Seed the random number generator
  UD_ERROR_IF(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)__FUNCTION__, sizeof(__FUNCTION__)) != 0, udR_InternalCryptoError);
  // Generate some random data
  UD_ERROR_IF(mbedtls_ctr_drbg_random(&ctr_drbg, randomData, sizeof(randomData)) !=  0, udR_InternalCryptoError);

  result = udCryptoKey_DeriveFromData(pKey, keyLen,randomData, sizeof(randomData));

epilogue:
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  return result;
}

struct udCryptoDHMContext
{
  mbedtls_dhm_context dhm;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;

  udResult Init();
  udResult GenerateKey(void *pKey, size_t keyLen);
  void Destroy();
};

// ---------------------------------------------------------------------------------------
// Author: Dave Pevreal, October 2017
udResult udCryptoDHMContext::Init()
{
  udResult result;
  int mbErr;
  unsigned char tempBuf[2048 / 8 * 3 + 16]; // A temporary buffer to export params, I don't use it but there's no choice
  size_t tempoutlen;

  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  mbedtls_dhm_init(&dhm);
  mbedtls_mpi_read_string(&dhm.P, 16, MBEDTLS_DHM_RFC5114_MODP_2048_P);
  mbedtls_mpi_read_string(&dhm.G, 16, MBEDTLS_DHM_RFC5114_MODP_2048_G);

  mbErr = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)__FUNCTION__, sizeof(__FUNCTION__));
  UD_ERROR_IF(mbErr, udR_InternalCryptoError);

  // Generate random parameters to suit the P and G already defined, these are output to tempBuf
  // but in a stupid format so we ignore that and we will save them out as JSON separately after generating the public value
  mbErr = mbedtls_dhm_make_params(&dhm, mbedtls_mpi_size(&dhm.P), tempBuf, &tempoutlen, mbedtls_ctr_drbg_random, &ctr_drbg);
  UD_ERROR_IF(mbErr, udR_InternalCryptoError);
  mbErr = mbedtls_dhm_make_public(&dhm, mbedtls_mpi_size(&dhm.P), tempBuf, mbedtls_mpi_size(&dhm.P), mbedtls_ctr_drbg_random, &ctr_drbg);
  UD_ERROR_IF(mbErr, udR_InternalCryptoError);

  result = udR_Success;

epilogue:
  return result;
}

// ---------------------------------------------------------------------------------------
// Author: Dave Pevreal, October 2017
udResult udCryptoDHMContext::GenerateKey(void *pKey, size_t keyLen)
{
  udResult result;
  unsigned char output[2048 / 8];
  size_t outputLen;

  UD_ERROR_NULL(pKey, udR_InvalidParameter_);
  UD_ERROR_IF(keyLen < 1, udR_InvalidParameter_);
  if (mbedtls_dhm_calc_secret(&dhm, output, sizeof(output), &outputLen, mbedtls_ctr_drbg_random, &ctr_drbg))
  {
    UD_ERROR_SET(udR_InternalCryptoError);
  }

  result = udCryptoKey_DeriveFromData(pKey, keyLen, output, outputLen);

epilogue:
  return result;
}

// ---------------------------------------------------------------------------------------
// Author: Dave Pevreal, October 2017
void udCryptoDHMContext::Destroy()
{
  mbedtls_dhm_free(&dhm);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  memset(this, 0, sizeof(*this));
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
udResult udCryptoKey_CreateDHM(udCryptoDHMContext **ppDHMCtx, const char **ppPublicValueA, size_t keyLen)
{
  udResult result;
  udValue v;
  udCryptoDHMContext *pCtx = nullptr;

  pCtx = udAllocType(udCryptoDHMContext, 1, udAF_Zero);
  UD_ERROR_NULL(pCtx, udR_MemoryAllocationFailure);
  UD_ERROR_CHECK(pCtx->Init());

  // Now export the important bits to JSON
  v.Set("keyLen = %d", keyLen);
  // Don't export P and G, we can assume they are from RFC5114
  UD_ERROR_CHECK(ToValue(pCtx->dhm.GX, &v, "PublicValue"));
  UD_ERROR_CHECK(v.Export(ppPublicValueA, udVEO_JSON | udVEO_StripWhiteSpace));

  // Success, so transfer ownership of the context to caller
  *ppDHMCtx = pCtx;
  pCtx = nullptr;
  result = udR_Success;

epilogue:
  udCryptoKey_DestroyDHM(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
udResult udCryptoKey_DeriveFromPartyA(const char *pPublicValueA, const char **ppPublicValueB, void *pKey, size_t keyLen)
{
  udResult result;
  udValue v;
  udCryptoDHMContext ctx;

  UD_ERROR_IF(ctx.Init() != 0, udR_InternalCryptoError);
  UD_ERROR_NULL(pPublicValueA, udR_InvalidParameter_);
  UD_ERROR_NULL(ppPublicValueB, udR_InvalidParameter_);
  UD_ERROR_NULL(pKey, udR_InvalidParameter_);

  UD_ERROR_CHECK(v.Parse(pPublicValueA));
  UD_ERROR_IF(v.Get("keyLen").AsInt() != (int)keyLen, udR_InvalidParameter_);
  UD_ERROR_CHECK(FromString(&ctx.dhm.GY, v.Get("PublicValue").AsString())); // Read other party's GX into our GY
  UD_ERROR_CHECK(ctx.GenerateKey(pKey, keyLen));
  v.Destroy();
  UD_ERROR_CHECK(ToValue(ctx.dhm.GX, &v, "PublicValue"));
  v.Export(ppPublicValueB, udVEO_JSON | udVEO_StripWhiteSpace);

epilogue:
  ctx.Destroy();
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
udResult udCryptoKey_DeriveFromPartyB(udCryptoDHMContext *pCtx, const char *pPublicValueB, void *pKey, size_t keyLen)
{
  udResult result;
  udValue v;

  UD_ERROR_NULL(pCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pPublicValueB, udR_InvalidParameter_);
  UD_ERROR_NULL(pKey, udR_InvalidParameter_);
  UD_ERROR_IF(keyLen < 1, udR_InvalidParameter_);

  UD_ERROR_CHECK(v.Parse(pPublicValueB));
  UD_ERROR_CHECK(FromString(&pCtx->dhm.GY, v.Get("PublicValue").AsString())); // Read other party's GX into our GY
  UD_ERROR_CHECK(pCtx->GenerateKey(pKey, keyLen));

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
void udCryptoKey_DestroyDHM(udCryptoDHMContext **ppDHMCtx)
{
  if (ppDHMCtx && *ppDHMCtx)
  {
    (*ppDHMCtx)->Destroy();
    udFree((*ppDHMCtx));
  }
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCryptoSig_CreateKeyPair(udCryptoSigContext **ppSigCtx, udCryptoSigType type)
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
  udCryptoSig_Destroy(&pSigCtx);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCryptoSig_ExportKeyPair(udCryptoSigContext *pSigCtx, const char **ppKeyText, bool exportPrivate)
{
  udResult result;
  udValue v;

  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(ppKeyText, udR_InvalidParameter_);

  switch (pSigCtx->type)
  {
  case udCST_RSA2048:
  case udCST_RSA4096:
    v.Set("Type = '%s'", exportPrivate ? TYPESTRING_RSAPRIVATE : TYPESTRING_RSAPUBLIC);
    v.Set("Size = %d", pSigCtx->type);
    UD_ERROR_CHECK(ToValue(pSigCtx->rsa.N, &v, "N"));
    UD_ERROR_CHECK(ToValue(pSigCtx->rsa.E, &v, "E"));
    if (exportPrivate)
    {
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.D, &v, "D"));
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.P, &v, "P"));
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.Q, &v, "Q"));
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.DP, &v, "DP"));
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.DQ, &v, "DQ"));
      UD_ERROR_CHECK(ToValue(pSigCtx->rsa.QP, &v, "QP"));
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
udResult udCryptoSig_ImportKeyPair(udCryptoSigContext **ppSigCtx, const char *pKeyText)
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
  udCryptoSig_Destroy(&pSigCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
udResult udCryptoSig_Sign(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char **ppSignatureString, udCryptoSigPadScheme pad)
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
udResult udCryptoSig_Verify(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char *pSignatureString, udCryptoSigPadScheme pad)
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
      UD_ERROR_IF(sigLen != (size_t)(pSigCtx->type / 8), udR_InternalCryptoError);
      break;
    default:
      UD_ERROR_SET(udR_InvalidConfiguration);
  }

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2017
void udCryptoSig_Destroy(udCryptoSigContext **ppSigCtx)
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

