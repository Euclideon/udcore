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
#include "mbedtls/md5.h"

#include "mbedtls/dhm.h"

#include "mbedtls/rsa.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#if !defined(FULL_MBEDTLS)
extern "C" int mbedtls_snprintf(char * s, size_t n, const char * format, ...)
{
  va_list ap;
  va_start(ap, format);
  int len = udSprintfVA(s, n, format, ap);
  va_end(ap);
  return len;
}
#endif

enum
{
  AES_BLOCK_SIZE = 16
};

#define TYPESTRING_RSA "RSA"
#define TYPESTRING_ECDSA "ECDSA"

#define TYPESTRING_CURVE_BP384R1 "BP384R1"

const mbedtls_md_type_t udc_to_mbed_hashfunctions[] = { MBEDTLS_MD_SHA1, MBEDTLS_MD_SHA256, MBEDTLS_MD_SHA512, MBEDTLS_MD_MD5, MBEDTLS_MD_NONE };
UDCOMPILEASSERT(UDARRAYSIZE(udc_to_mbed_hashfunctions) == udCH_Count+1, "Hash methods array not updated as well!");

struct udCryptoCipherContext
{
  mbedtls_aes_context ctx;
  uint8_t key[32];
  uint8_t iv[AES_BLOCK_SIZE];
  size_t blockSize;
  int keyLengthInBits;
  udCryptoCiphers cipher;
  udCryptoChainMode chainMode;
  udCryptoPaddingMode padMode;
  bool ctxInit;
};

struct udCryptoHashContext
{
  udCryptoHashes hashMethod;
  size_t hashLengthInBytes;
  union
  {
    mbedtls_sha1_context sha1;
    mbedtls_sha256_context sha256;
    mbedtls_sha512_context sha512;
    mbedtls_md5_context md5;
  };
};

struct udCryptoSigContext
{
  udCryptoSigType type;
  union
  {
    mbedtls_rsa_context rsa;
    mbedtls_ecdsa_context ecdsa;
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
  udFreeSecure(pBuf, bufLen);
  udCrypto_FreeSecure(pBase64);
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
// Author: Dave Pevreal, March 2018
// Helper to parse little endian binary to an mpi, INCREMENTS the pBuf pointer for convenience
static udResult FromLittleEndianBinary(mbedtls_mpi *pBigNum, uint8_t *&pBuf, size_t &bufLen, int numLen)
{
  udResult result;
  uint8_t buf[4096/8];

  UD_ERROR_IF(numLen > (int)sizeof(buf), udR_InternalError);
  UD_ERROR_IF(numLen < 0 || (size_t)numLen > bufLen, udR_InternalError);
  for (int i = numLen - 1; i >= 0; --i)
    buf[i] = *pBuf++;
  bufLen -= numLen;
  UD_ERROR_IF(mbedtls_mpi_read_binary(pBigNum, buf, numLen) != 0, udR_InternalCryptoError);
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
void udCrypto_FreeSecure(const char *&pBase64String)
{
  udFreeSecure(pBase64String, udStrlen(pBase64String));
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
void udCrypto_Obscure(const char *pBase64String)
{
  size_t len = udStrlen(pBase64String);
  for (size_t i = 0; i < len; ++i)
    const_cast<char*>(pBase64String)[i] ^= 1; // For now simple xor obscure
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
udResult udCrypto_Random(void *pMem, size_t len)
{
  udResult result;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;

  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);
  // Seed the random number generator
  UD_ERROR_IF(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)__FUNCTION__, sizeof(__FUNCTION__)) != 0, udR_InternalCryptoError);
  // Generate some random data
  UD_ERROR_IF(mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char*)pMem, len) != 0, udR_InternalCryptoError);

  result = udR_Success;

epilogue:
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_Create(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const char *pKey, udCryptoChainMode chainMode)
{
  udResult result;
  udCryptoCipherContext *pCtx = nullptr;
  size_t keyLen;

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
  UD_ERROR_CHECK(udBase64Decode(pKey, 0, pCtx->key, sizeof(pCtx->key), &keyLen));
  UD_ERROR_IF((int)keyLen != pCtx->keyLengthInBits / 8, udR_InvalidConfiguration);
  pCtx->ctxInit = false;

  // Give ownership of the context to the caller
  *ppCtx = pCtx;
  pCtx = nullptr;
  result = udR_Success;

epilogue:
  if (pCtx)
  {
    mbedtls_aes_free(&pCtx->ctx);
    udFreeSecure(pCtx, sizeof(*pCtx));
  }
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, July 2016
udResult udCrypto_CreateIVForCTRMode(udCryptoCipherContext *pCtx, udCryptoIV *pIV, uint64_t nonce, uint64_t counter)
{
  if (pCtx == nullptr || pIV == nullptr)
    return udR_InvalidParameter_;
  if (pCtx->chainMode != udCCM_CTR)
    return udR_InvalidConfiguration;
  // Nonce is written little-endian as it is usually just data and every known platform we have is little endian
  for (int i = 0; i < 8; ++i)
    pIV->iv[i] = (uint8_t)(nonce >> (i * 8));
  // Very important for counter to be assigned big endian, this is because standards were made on stupid-endian computers
  for (int i = 0; i < 8; ++i)
    pIV->iv[8 + i] = (uint8_t)(counter >> ((7 - i) * 8));
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_Encrypt(udCryptoCipherContext *pCtx, const udCryptoIV *pIV, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen, udCryptoIV *pOutIV)
{
  udResult result;
  size_t paddedCliperTextLen = 0;
  const void *pPaddedPlainText = nullptr;

  UD_ERROR_IF(!pCtx || !pPlainText || !pCipherText, udR_InvalidParameter_);

  if (!pCtx->ctxInit)
  {
    mbedtls_aes_setkey_enc(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits);
    pCtx->ctxInit = true;
  }

  if (pCtx->padMode == udCPM_None)
    paddedCliperTextLen = plainTextLen;
  else
    paddedCliperTextLen = (plainTextLen + pCtx->blockSize) & ~(pCtx->blockSize - 1);

  UD_ERROR_IF((paddedCliperTextLen  & (pCtx->blockSize - 1)) != 0, udR_AlignmentRequirement);
  UD_ERROR_IF(paddedCliperTextLen > cipherTextLen, udR_BufferTooSmall);

  // Trading efficiency for simplicity, just duplicate the source and add the padding bytes
  if (paddedCliperTextLen != plainTextLen)
  {
    size_t padBytes = paddedCliperTextLen - plainTextLen;
    pPaddedPlainText = udMemDup(pPlainText, plainTextLen, padBytes, udAF_None);
    memset(udAddBytes((void*)pPaddedPlainText, plainTextLen), (int)padBytes, padBytes);
  }
  else
  {
    pPaddedPlainText = pPlainText;
  }

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      {
        switch (pCtx->chainMode)
        {
          case udCCM_CBC:
            UD_ERROR_NULL(pIV, udR_InvalidParameter_);
            memcpy(pCtx->iv, pIV, sizeof(pCtx->iv));
            UD_ERROR_IF(mbedtls_aes_crypt_cbc(&pCtx->ctx, MBEDTLS_AES_ENCRYPT, paddedCliperTextLen, pCtx->iv, (const unsigned char*)pPaddedPlainText, (unsigned char*)pCipherText) != 0, udR_InternalCryptoError);

            // For CBC, the output IV is the last encrypted block
            if (pOutIV)
              memcpy(pOutIV, (char*)pCipherText + paddedCliperTextLen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            break;

          case udCCM_CTR:
            UD_ERROR_IF(pIV == nullptr || pOutIV != nullptr, udR_InvalidParameter_); // Don't allow output IV in CTR mode (yet)
            memcpy(pCtx->iv, pIV, sizeof(pCtx->iv));
            {
              size_t ncoff = 0;
              unsigned char stream_block[16];
              UD_ERROR_IF(mbedtls_aes_crypt_ctr(&pCtx->ctx, paddedCliperTextLen, &ncoff, pCtx->iv, stream_block, (const unsigned char*)pPaddedPlainText, (unsigned char *)pCipherText) != 0, udR_InternalCryptoError);
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
  if (pPaddedPlainText != pPlainText)
    udFreeSecure(pPaddedPlainText, paddedCliperTextLen);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_Decrypt(udCryptoCipherContext *pCtx, const udCryptoIV *pIV, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen, udCryptoIV *pOutIV)
{
  udResult result;
  size_t actualPlainTextLen;
  void *pPaddedPlainText = nullptr;

  UD_ERROR_IF(!pCtx || !pPlainText || !pCipherText, udR_InvalidParameter_);
  UD_ERROR_IF((cipherTextLen % pCtx->blockSize) != 0, udR_AlignmentRequirement);

  if (cipherTextLen > plainTextLen)
  {
    UD_ERROR_IF(pCtx->padMode == udCPM_None, udR_BufferTooSmall);
    pPaddedPlainText = udAlloc(cipherTextLen);
  }
  else
  {
    pPaddedPlainText = pPlainText;
  }

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
    {
      switch (pCtx->chainMode)
      {
        case udCCM_CBC:
          UD_ERROR_NULL(pIV, udR_InvalidParameter_);
          if (!pCtx->ctxInit)
          {
            mbedtls_aes_setkey_dec(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits);
            pCtx->ctxInit = true;
          }
          memcpy(pCtx->iv, pIV, sizeof(pCtx->iv));
          UD_ERROR_IF(mbedtls_aes_crypt_cbc(&pCtx->ctx, MBEDTLS_AES_DECRYPT, cipherTextLen, pCtx->iv, (const unsigned char*)pCipherText, (unsigned char *)pPaddedPlainText) != 0, udR_InternalCryptoError);
          if (pOutIV)
            memcpy(pOutIV, pCtx->iv, sizeof(pCtx->iv));
          break;

        case udCCM_CTR:
          UD_ERROR_IF(pIV == nullptr || pOutIV != nullptr, udR_InvalidParameter_); // Don't allow output IV in CTR mode (yet)
          if (!pCtx->ctxInit)
          {
            mbedtls_aes_setkey_enc(&pCtx->ctx, pCtx->key, pCtx->keyLengthInBits); // NOTE: Using ENCRYPT key schedule for CTR mode
            pCtx->ctxInit = true;
          }
          memcpy(pCtx->iv, pIV, sizeof(pCtx->iv));
          {
            size_t ncoff = 0;
            unsigned char stream_block[16];
            if (mbedtls_aes_crypt_ctr(&pCtx->ctx, cipherTextLen, &ncoff, pCtx->iv, stream_block, (const unsigned char*)pCipherText, (unsigned char *)pPaddedPlainText) != 0)
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
  if (pCtx->padMode == udCPM_None)
  {
    actualPlainTextLen = cipherTextLen;
  }
  else
  {
    uint8_t padBytes = ((uint8_t*)pPaddedPlainText)[cipherTextLen - 1];
    actualPlainTextLen = cipherTextLen - padBytes;
    // Part of PKCS#7 is the padding must be verified
    for (uint8_t i = 0; i < padBytes; ++i)
      UD_ERROR_IF(((uint8_t*)pPaddedPlainText)[actualPlainTextLen + i] != padBytes, udR_CorruptData);
    if (pPaddedPlainText != pPlainText)
      memcpy(pPlainText, pPaddedPlainText, actualPlainTextLen);
  }

  if (pActualPlainTextLen)
    *pActualPlainTextLen = actualPlainTextLen;
  result = udR_Success;

epilogue:
  if (pPaddedPlainText != pPlainText)
    udFreeSecure(pPaddedPlainText, cipherTextLen);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_Destroy(udCryptoCipherContext **ppCtx)
{
  if (!ppCtx || !*ppCtx)
    return udR_InvalidParameter_;
  mbedtls_aes_free(&(*ppCtx)->ctx);
  udFreeSecure(*ppCtx, sizeof(**ppCtx));
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoCipher_SelfTest(udCryptoCiphers cipher)
{
  udResult result = udR_Failure_;
  udCryptoCipherContext *pCtx = nullptr;
  const char *pKey = nullptr;
  static const size_t keyLengths[udCC_Count] = { udCCKL_AES128KeyLength, udCCKL_AES256KeyLength };
  static const char *pPlainText = "There are two great days in every person's life; the day we are born and the day we discover why"; // Multiple of 16 characters
  static const udCryptoIV iv = { { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f } };
  static const uint64_t nonce = 0xf7f6f5f4f3f2f1f0ULL; // Note the nonce is little endian for udCrypto
  static const uint64_t counter = 0xf8f9fafbfcfdfeffULL; // The counter is big endian, because standards dictate that it is
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
  udCryptoIV ctrIV;
  uint8_t cipherText[128];
  uint8_t decryptedText[128];
  size_t actualCipherTextLen, actualDecryptedTextLen;

  UD_ERROR_IF(cipher < 0 || cipher >= udCC_Count, udR_InvalidParameter_);
  // Use a KDF to populate the key
  UD_ERROR_CHECK(udCryptoKey_DeriveFromPassword(&pKey, keyLengths[cipher], "password"));

  memset(cipherText, 0, sizeof(cipherText));
  memset(decryptedText, 0, sizeof(decryptedText));
  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, pKey, udCCM_CBC));
  UD_ERROR_CHECK(udCryptoCipher_Encrypt(pCtx, &iv, pPlainText, udStrlen(pPlainText), cipherText, sizeof(cipherText), &actualCipherTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualCipherTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(cipherText, cipherTextCBC[cipher], actualCipherTextLen) != 0, udR_InternalCryptoError);

  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, pKey, udCCM_CBC));
  UD_ERROR_CHECK(udCryptoCipher_Decrypt(pCtx, &iv, cipherText, actualCipherTextLen, decryptedText, sizeof(decryptedText), &actualDecryptedTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualDecryptedTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(pPlainText, decryptedText, actualDecryptedTextLen) != 0, udR_InternalCryptoError);

  actualCipherTextLen = actualDecryptedTextLen = 0;
  memset(cipherText, 0, sizeof(cipherText));
  memset(decryptedText, 0, sizeof(decryptedText));
  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, pKey, udCCM_CTR));
  UD_ERROR_CHECK(udCrypto_CreateIVForCTRMode(pCtx, &ctrIV, nonce, counter));
  UD_ERROR_CHECK(udCryptoCipher_Encrypt(pCtx, &ctrIV, pPlainText, udStrlen(pPlainText), cipherText, sizeof(cipherText), &actualCipherTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualCipherTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(cipherText, cipherTextCTR[cipher], actualCipherTextLen) != 0, udR_InternalCryptoError);

  UD_ERROR_CHECK(udCryptoCipher_Create(&pCtx, cipher, udCPM_None, pKey, udCCM_CTR));
  UD_ERROR_CHECK(udCrypto_CreateIVForCTRMode(pCtx, &ctrIV, nonce, counter));
  UD_ERROR_CHECK(udCryptoCipher_Decrypt(pCtx, &ctrIV, cipherText, actualCipherTextLen, decryptedText, sizeof(decryptedText), &actualDecryptedTextLen));
  UD_ERROR_CHECK(udCryptoCipher_Destroy(&pCtx));
  UD_ERROR_IF(actualDecryptedTextLen != 96, udR_InternalCryptoError);
  UD_ERROR_IF(memcmp(pPlainText, decryptedText, actualDecryptedTextLen) != 0, udR_InternalCryptoError);

epilogue:
  udCryptoCipher_Destroy(&pCtx); // In case anything failed
  udFree(pKey);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_Create(udCryptoHashContext **ppCtx, udCryptoHashes hashMethod)
{
  udResult result;
  udCryptoHashContext *pCtx = nullptr;

  UD_ERROR_IF(hashMethod >= udCH_Count, udR_InvalidParameter_);
  UD_ERROR_NULL(ppCtx, udR_InvalidParameter_);

  pCtx = udAllocType(udCryptoHashContext, 1, udAF_Zero);
  UD_ERROR_NULL(pCtx, udR_MemoryAllocationFailure);

  pCtx->hashMethod = hashMethod;
  switch (hashMethod)
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
    case udCH_MD5:
      mbedtls_md5_init(&pCtx->md5);
      mbedtls_md5_starts(&pCtx->md5);
      pCtx->hashLengthInBytes = udCHL_MD5Length;
      break;
    default:
      UD_ERROR_SET(udR_InvalidParameter_);
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

  switch (pCtx->hashMethod)
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
    case udCH_MD5:
      mbedtls_md5_update(&pCtx->md5, (const uint8_t*)pBytes, length);
      break;
    default:
      return udR_InvalidParameter_;
  }
  return udR_Success;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_Finalise(udCryptoHashContext *pCtx, const char **ppHashBase64)
{
  uint8_t hash[udCHL_MaxHashLength];
  if (!pCtx || !ppHashBase64)
    return udR_InvalidParameter_;

  switch (pCtx->hashMethod)
  {
    case udCH_SHA1:
      mbedtls_sha1_finish(&pCtx->sha1, hash);
      break;
    case udCH_SHA256:
      mbedtls_sha256_finish(&pCtx->sha256, hash);
      break;
    case udCH_SHA512:
      mbedtls_sha512_finish(&pCtx->sha512, hash);
      break;
    case udCH_MD5:
      mbedtls_md5_finish(&pCtx->md5, hash);
      break;
    default:
      return udR_InvalidConfiguration;
  }

  return udBase64Encode(ppHashBase64, hash, pCtx->hashLengthInBytes);
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_Destroy(udCryptoHashContext **ppCtx)
{
  if (!ppCtx || !*ppCtx)
    return udR_InvalidParameter_;
  switch ((*ppCtx)->hashMethod)
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
udResult udCryptoHash_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, const char **ppHashBase64,
                           const void *pMessage2, size_t message2Length)
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
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, ppHashBase64));

epilogue:
  udCryptoHash_Destroy(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCryptoHash_HMAC(udCryptoHashes hash, const char *pKeyBase64, const void *pMessage, size_t messageLength, const char **ppHMACBase64)
{
  // See https://en.wikipedia.org/wiki/Hash-based_message_authentication_code
  udResult result;
  uint8_t *pLongKeyData = nullptr;
  const char *pLongKeyHashBase64 = nullptr;
  const char *pIpadHashBase64 = nullptr;
  enum { blockSize = 64 };
  uint8_t key[blockSize];
  uint8_t opad[blockSize];
  uint8_t ipad[blockSize];
  uint8_t ipadHash[blockSize];
  size_t ipadHashLen;

  memset(key, 0, blockSize);
  if (udBase64Decode(pKeyBase64, 0, key, sizeof(key)) != udR_Success)
  {
    // Key is longer than a block, so we need to hash it down
    size_t longKeyDataLen = 0;
    UD_ERROR_CHECK(udBase64Decode(&pLongKeyData, &longKeyDataLen, pKeyBase64));
    UD_ERROR_CHECK(udCryptoHash_Hash(hash, pLongKeyData, longKeyDataLen, &pLongKeyHashBase64));
    UD_ERROR_CHECK(udBase64Decode(pLongKeyHashBase64, 0, key, sizeof(key)));
  }

  for (int i = 0; i < blockSize; ++i)
  {
    opad[i] = 0x5c ^ key[i];
    ipad[i] = 0x36 ^ key[i];
  }

  // First hash the concat of ipad and the message
  UD_ERROR_CHECK(udCryptoHash_Hash(hash, ipad, blockSize, &pIpadHashBase64, pMessage, messageLength));
  // Need to decode it from base64
  UD_ERROR_CHECK(udBase64Decode(pIpadHashBase64, 0, ipadHash, sizeof(ipadHash), &ipadHashLen));
  // Then hash the concat of the opad and the result of first hash
  UD_ERROR_CHECK(udCryptoHash_Hash(hash, opad, blockSize, ppHMACBase64, ipadHash, ipadHashLen));

epilogue:
  memset(key, 0, sizeof(key));
  udFree(pIpadHashBase64);
  udFree(pLongKeyHashBase64);
  udFree(pLongKeyData);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCryptoHash_SelfTest(udCryptoHashes hash)
{
  udResult result = udR_Failure_;

  static const char *pHmacKeyBase64 = "SmVmZQ=="; // "Jefe"
  static const char testText[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };
  static const char *hashResults[] =
  {
    "hJg+RBw70m66rkqh+VEp5eVGcPE=", // udCH_SHA1
    "JI1qYdIGOLjlwCaTDD5gOaM85Flk/yFn9uzt1BnbBsE=", // udCH_SHA256
    "IEqPxt2oLwoM7XvrjgikFlfBbvRosiioJ5vjMacDwzWW/RXBOxsH+aodO+pXeJygMa2Fx6cd1wNU7GMSOMo0RQ==", // udCH_SHA512
    "ghXvB5aiC8qq4RbTh2xmSg==" // udCH_MD5
  };
  static const char *hmacResults[] =
  {
    "gNSITHDFNAbgpwcaLiE1yXkDv0k=", // udCH_SHA1
    "3jREzWMffTaJrx7MExnld3wD5ZrpsNXd3QrAWJZkunc=", // udCH_SHA256
    "GPopoTHOqaFlvz6rC15hyA7z+VfH6v4ok3R2G0ORm5dyctUPGkvxD6Aakbta9JodqZHFKtc4NBIDFvEW+NMkFg==", // udCH_SHA512
    "5NqJxWSrcHlNi4YwIo5+Qg==" // udCH_MD5
  };
  const char *pResultBase64 = nullptr;

  UDCOMPILEASSERT(UDARRAYSIZE(hashResults) == udCH_Count, "Updated hash list without updating tests!");
  UDCOMPILEASSERT(UDARRAYSIZE(hmacResults) == udCH_Count, "Updated hash list without updating tests!");

  UD_ERROR_CHECK(udCryptoHash_Hash(hash, testText, strlen(testText), &pResultBase64));
  UD_ERROR_IF(!udStrEqual(hashResults[hash], pResultBase64), udR_Failure_);
  udFree(pResultBase64);
  UD_ERROR_CHECK(udCryptoHash_HMAC(hash, pHmacKeyBase64, testText, strlen(testText), &pResultBase64));
  UD_ERROR_IF(!udStrEqual(hmacResults[hash], pResultBase64), udR_Failure_);
  udFree(pResultBase64); // Freed here for consistency in case more tests added

epilogue:
  udFree(pResultBase64);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, May 2017
udResult udCrypto_TestHMAC(udCryptoHashes hash)
{
  udResult result;
  const char *pKeyBase64 = "a2V5";
  const char *pMessage = "The quick brown fox jumps over the lazy dog";
  const char *pHMACBase64 = nullptr;
  static const char *pHMACResults[udCH_Count] =
  {
    "3nybhbi3iqa8ino29wqQcBydtNk=", // udCH_SHA1
    "97yD9DBThCSxMpjmqm+xQ+9NWaFJRhdZl0edvC0aPNg=", // udCH_SHA256
    nullptr // TODO: Include SHA512
  };

  UD_ERROR_IF(hash < udCH_SHA1 || hash > udCH_SHA256, udR_InvalidParameter_); // TODO: Include SHA512
  UD_ERROR_CHECK(udCryptoHash_HMAC(hash, pKeyBase64, pMessage, strlen(pMessage), &pHMACBase64));
  UD_ERROR_IF(!udStrEqual(pHMACBase64, pHMACResults[hash]), udR_Failure_);

epilogue:
  udFree(pHMACBase64);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, September 2015
udResult udCryptoKey_DeriveFromPassword(const char **ppKey, size_t keyLen, const char *pPassword)
{
  // Derive a secure key from a password string, using the same algorithm used by the Windows APIs to maintain interoperability with Geoverse
  // See the remarks section of the MS documentation here: http://msdn.microsoft.com/en-us/library/aa379916(v=vs.85).aspx
  udResult result;
  udCryptoHashContext *pCtx = nullptr;
  const char *pHashBase64 = nullptr;
  uint8_t passPhraseDigest[32];
  size_t passPhraseDigestLen;
  unsigned char hashBuffer[64];
  unsigned char derivedKey[40];

  UD_ERROR_IF(!pPassword || !ppKey, udR_InvalidParameter_);

                                             // Hash the pass phrase
  UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, "ud1971", 6)); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, pPassword, strlen(pPassword))); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, &pHashBase64));
  UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));
  UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, passPhraseDigest, sizeof(passPhraseDigest), &passPhraseDigestLen));
  udFree(pHashBase64);
  UD_ERROR_IF(passPhraseDigestLen != 20, udR_InternalCryptoError);

  // Create a buffer of constant 0x36 xor'd with pass phrase hash
  memset(hashBuffer, 0x36, 64);
  for (size_t i = 0; i < passPhraseDigestLen; i++)
    hashBuffer[i] ^= passPhraseDigest[i];

  // Hash the result again and this gives us the key
  UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
  UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, &pHashBase64));
  UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));
  UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, derivedKey, 20));
  udFree(pHashBase64);

  // For keys greater than 20 bytes, do the same thing with a different constant for the next 20 bytes
  if (keyLen > 20)
  {
    memset(hashBuffer, 0x5C, 64);
    for (size_t i = 0; i < passPhraseDigestLen; i++)
      hashBuffer[i] ^= passPhraseDigest[i];

    // Hash the result again and this gives us the key
    UD_ERROR_CHECK(udCryptoHash_Create(&pCtx, udCH_SHA1));
    UD_ERROR_CHECK(udCryptoHash_Digest(pCtx, hashBuffer, sizeof(hashBuffer)));
    UD_ERROR_CHECK(udCryptoHash_Finalise(pCtx, &pHashBase64));
    UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, derivedKey + 20, 20));
    udFree(pHashBase64);
    UD_ERROR_CHECK(udCryptoHash_Destroy(&pCtx));
  }

  result = udBase64Encode(ppKey, derivedKey, keyLen);

epilogue:
  memset(derivedKey, 0, sizeof(derivedKey));
  udFree(pHashBase64);
  udCryptoHash_Destroy(&pCtx);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2017
udResult udCryptoKey_DeriveFromData(const char **ppKey, size_t keyLen, const void *pData, size_t dataLen)
{
  udResult result;
  uint8_t hashBuffer[udCHL_SHA512Length];
  const char *pHashBase64 = nullptr;
  size_t i;

  UD_ERROR_IF(keyLen > sizeof(hashBuffer), udR_CountExceeded);
  UD_ERROR_CHECK(udCryptoHash_Hash((keyLen > udCHL_SHA256Length) ? udCH_SHA512 : udCH_SHA256, pData, dataLen, &pHashBase64, &i));
  UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, hashBuffer, sizeof(hashBuffer)));
  result = udBase64Encode(ppKey, hashBuffer, keyLen);

epilogue:
  udFree(pHashBase64);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2017
udResult udCryptoKey_DeriveFromRandom(const char **ppKey, size_t keyLen)
{
  udResult result;
  uint8_t randomData[64];

  result = udCrypto_Random(randomData, sizeof(randomData));
  UD_ERROR_HANDLE();
  result = udCryptoKey_DeriveFromData(ppKey, keyLen, randomData, sizeof(randomData));

epilogue:
  memset(randomData, 0, sizeof(randomData)); // Don't leave the source for the key sitting on the stack
  return result;
}

struct udCryptoDHMContext
{
  mbedtls_dhm_context dhm;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  size_t keyLen;

  udResult Init(size_t keyLen);
  udResult GenerateKey(const char **ppKey);
  void Destroy();
};

// ---------------------------------------------------------------------------------------
// Author: Dave Pevreal, October 2017
udResult udCryptoDHMContext::Init(size_t _keyLen)
{
  udResult result;
  int mbErr;
  unsigned char tempBuf[2048 / 8 * 3 + 16]; // A temporary buffer to export params, I don't use it but there's no choice
  size_t tempoutlen;

  keyLen = _keyLen;
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
udResult udCryptoDHMContext::GenerateKey(const char **ppKey)
{
  udResult result;
  unsigned char output[2048 / 8];
  size_t outputLen;

  UD_ERROR_NULL(ppKey, udR_InvalidParameter_);
  UD_ERROR_IF(keyLen < 1, udR_InvalidParameter_);
  if (mbedtls_dhm_calc_secret(&dhm, output, sizeof(output), &outputLen, mbedtls_ctr_drbg_random, &ctr_drbg))
  {
    UD_ERROR_SET(udR_InternalCryptoError);
  }

  result = udCryptoKey_DeriveFromData(ppKey, keyLen, output, outputLen);

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
  UD_ERROR_CHECK(pCtx->Init(keyLen));

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
udResult udCryptoKey_DeriveFromPartyA(const char *pPublicValueA, const char **ppPublicValueB, const char **ppKey)
{
  udResult result;
  udValue v;
  udCryptoDHMContext ctx;
  bool ctxInit = false;

  UD_ERROR_NULL(pPublicValueA, udR_InvalidParameter_);
  UD_ERROR_NULL(ppPublicValueB, udR_InvalidParameter_);
  UD_ERROR_NULL(ppKey, udR_InvalidParameter_);

  UD_ERROR_CHECK(v.Parse(pPublicValueA));
  UD_ERROR_IF(ctx.Init(v.Get("keyLen").AsInt()) != 0, udR_InternalCryptoError);
  ctxInit = true;
  UD_ERROR_CHECK(FromString(&ctx.dhm.GY, v.Get("PublicValue").AsString())); // Read other party's GX into our GY
  UD_ERROR_CHECK(ctx.GenerateKey(ppKey));
  v.Destroy();
  UD_ERROR_CHECK(ToValue(ctx.dhm.GX, &v, "PublicValue"));
  UD_ERROR_CHECK(v.Export(ppPublicValueB, udVEO_JSON | udVEO_StripWhiteSpace));

epilogue:
  if (ctxInit)
    ctx.Destroy();
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, October 2017
udResult udCryptoKey_DeriveFromPartyB(udCryptoDHMContext *pCtx, const char *pPublicValueB, const char **ppKey)
{
  udResult result;
  udValue v;

  UD_ERROR_NULL(pCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pPublicValueB, udR_InvalidParameter_);
  UD_ERROR_NULL(ppKey, udR_InvalidParameter_);

  UD_ERROR_CHECK(v.Parse(pPublicValueB));
  UD_ERROR_CHECK(FromString(&pCtx->dhm.GY, v.Get("PublicValue").AsString())); // Read other party's GX into our GY
  UD_ERROR_CHECK(pCtx->GenerateKey(ppKey));

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
    udFreeSecure((*ppDHMCtx), sizeof(**ppDHMCtx));
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
    case udCST_RSA1024:
    case udCST_RSA2048:
    case udCST_RSA4096:
      mbedtls_rsa_init(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      mbErr = mbedtls_rsa_gen_key(&pSigCtx->rsa, mbedtls_ctr_drbg_random, &ctr_drbg, type, 65537);
      UD_ERROR_IF(mbErr, udR_InternalCryptoError);
      break;
    case udCST_ECPBP384:
      mbedtls_ecdsa_init(&pSigCtx->ecdsa);
      mbErr = mbedtls_ecdsa_genkey(&pSigCtx->ecdsa, MBEDTLS_ECP_DP_BP384R1, mbedtls_ctr_drbg_random, &ctr_drbg);
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

  if (exportPrivate)
    v.Set("Private = true");

  switch (pSigCtx->type)
  {
    case udCST_RSA1024:
    case udCST_RSA2048:
    case udCST_RSA4096:
    v.Set("Type = '%s'", TYPESTRING_RSA);
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
  case udCST_ECPBP384:
    v.Set("Type = '%s'", TYPESTRING_ECDSA);
    v.Set("Curve = '%s'", TYPESTRING_CURVE_BP384R1);
    UD_ERROR_CHECK(ToValue(pSigCtx->ecdsa.Q.X, &v, "X"));
    UD_ERROR_CHECK(ToValue(pSigCtx->ecdsa.Q.Y, &v, "Y"));
    UD_ERROR_CHECK(ToValue(pSigCtx->ecdsa.Q.Z, &v, "Z"));
    if (exportPrivate)
      UD_ERROR_CHECK(ToValue(pSigCtx->ecdsa.d, &v, "D"));
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
  const char *pTypeString = nullptr;
  bool isPrivateKey = false;

  result = v.Parse(pKeyText);
  UD_ERROR_HANDLE();
  pSigCtx = udAllocType(udCryptoSigContext, 1, udAF_Zero);
  UD_ERROR_NULL(pSigCtx, udR_MemoryAllocationFailure);

  isPrivateKey = v.Get("Private").AsBool();
  pTypeString = v.Get("Type").AsString();
  if (udStrEqual(pTypeString, TYPESTRING_ECDSA))
  {
    if (udStrEqual(v.Get("Curve").AsString(), TYPESTRING_CURVE_BP384R1))
      pSigCtx->type = udCST_ECPBP384;
    else
      UD_ERROR_SET(udR_InternalCryptoError);
  }
  else if (udStrEqual(pTypeString, TYPESTRING_RSA))
  {
  pSigCtx->type = (udCryptoSigType)v.Get("Size").AsInt();
  }
  else
  {
    UD_ERROR_SET(udR_InternalCryptoError);
  }

  switch (pSigCtx->type)
  {
    case udCST_RSA1024:
    case udCST_RSA2048:
    case udCST_RSA4096:
      mbedtls_rsa_init(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      pSigCtx->rsa.len = pSigCtx->type / 8; // Size of the key
      UD_ERROR_CHECK(FromString(&pSigCtx->rsa.N, v.Get("N").AsString()));
      UD_ERROR_CHECK(FromString(&pSigCtx->rsa.E, v.Get("E").AsString()));
      UD_ERROR_IF(mbedtls_rsa_check_pubkey(&pSigCtx->rsa) != 0, udR_InternalCryptoError);
      if (isPrivateKey)
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
    case udCST_ECPBP384:
      mbedtls_ecdsa_init(&pSigCtx->ecdsa);
      mbedtls_ecp_group_load(&pSigCtx->ecdsa.grp, MBEDTLS_ECP_DP_BP384R1);
      UD_ERROR_CHECK(FromString(&pSigCtx->ecdsa.Q.X, v.Get("X").AsString()));
      UD_ERROR_CHECK(FromString(&pSigCtx->ecdsa.Q.Y, v.Get("Y").AsString()));
      UD_ERROR_CHECK(FromString(&pSigCtx->ecdsa.Q.Z, v.Get("Z").AsString()));
      UD_ERROR_IF(mbedtls_ecp_check_pubkey(&pSigCtx->ecdsa.grp, &pSigCtx->ecdsa.Q) != 0, udR_InternalCryptoError);
      if (isPrivateKey)
      {
        UD_ERROR_CHECK(FromString(&pSigCtx->ecdsa.d, v.Get("D").AsString()));
        UD_ERROR_IF(mbedtls_ecp_check_privkey(&pSigCtx->ecdsa.grp, &pSigCtx->ecdsa.d) != 0, udR_InternalCryptoError);
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
// Author: Dave Pevreal, March 2018
udResult udCryptoSig_ImportMSBlob(udCryptoSigContext **ppSigCtx, void *pBlob, size_t blobLen)
{
  // https://msdn.microsoft.com/en-us/library/cc250013.aspx
  struct MSBlob
  {
    uint8_t type, version;
    uint16_t res1;
    int32_t keyAlg;
    int32_t magic;
    int32_t bitLen;
  };
  MSBlob *pPriv = (MSBlob*)pBlob;
  uint8_t *p = (uint8_t*)(pPriv+1);

  udResult result;
  udValue v;
  udCryptoSigContext *pSigCtx = nullptr;

  UD_ERROR_IF(pPriv->type != 6 && pPriv->type != 7, udR_InvalidParameter_); // public/private key
  UD_ERROR_IF(pPriv->version != 2, udR_InvalidParameter_);
  UD_ERROR_IF(pPriv->keyAlg != 0x00002400, udR_InvalidParameter_);
  UD_ERROR_IF(pPriv->magic != 0x32415352 && pPriv->magic != 0x31415352, udR_InvalidParameter_);
  UD_ERROR_IF(pPriv->bitLen != 1024 && pPriv->bitLen != 2048 && pPriv->bitLen != 4096, udR_InvalidParameter_);
  pSigCtx = udAllocType(udCryptoSigContext, 1, udAF_Zero);
  UD_ERROR_NULL(pSigCtx, udR_MemoryAllocationFailure);

  pSigCtx->type = (udCryptoSigType)pPriv->bitLen;
  mbedtls_rsa_init(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
  pSigCtx->rsa.len = pSigCtx->type / 8; // Size of the key

  UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.E,  p, blobLen, 4));
  UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.N,  p, blobLen, pPriv->bitLen / 8));
  UD_ERROR_IF(mbedtls_rsa_check_pubkey(&pSigCtx->rsa) != 0, udR_InternalCryptoError);
  if (pPriv->type == 7)
  {
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.P,  p, blobLen, pPriv->bitLen / 16));
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.Q,  p, blobLen, pPriv->bitLen / 16));
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.DP, p, blobLen, pPriv->bitLen / 16));
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.DQ, p, blobLen, pPriv->bitLen / 16));
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.QP, p, blobLen, pPriv->bitLen / 16)); // Iq == inv(q % p)
    UD_ERROR_CHECK(FromLittleEndianBinary(&pSigCtx->rsa.D,  p, blobLen, pPriv->bitLen / 8));
    UD_ERROR_IF(mbedtls_rsa_check_privkey(&pSigCtx->rsa) != 0, udR_InternalCryptoError);
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
udResult udCryptoSig_Sign(udCryptoSigContext *pSigCtx, const char *pHashBase64, const char **ppSignatureBase64, udCryptoHashes hashMethod, udCryptoSigPadScheme pad)
{
  udResult result = udR_Failure_;
  unsigned char signature[udCST_RSA4096/8];
  unsigned char hash[udCHL_MaxHashLength];
  size_t hashLen;
  size_t sigLen = sizeof(signature);

  UD_ERROR_IF(hashMethod > udCH_Count, udR_InvalidParameter_);
  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pHashBase64, udR_InvalidParameter_);
  UD_ERROR_NULL(ppSignatureBase64, udR_InvalidParameter_);

  UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, hash, sizeof(hash), &hashLen));

  switch (pSigCtx->type)
  {
    case udCST_RSA1024:
    case udCST_RSA2048:
    case udCST_RSA4096:
      if (pad == udCSPS_Deterministic)
        mbedtls_rsa_set_padding(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      UD_ERROR_IF(sizeof(signature) < (size_t)(pSigCtx->type / 8), udR_InternalCryptoError);
      UD_ERROR_IF(mbedtls_rsa_rsassa_pkcs1_v15_sign(&pSigCtx->rsa, NULL, NULL, MBEDTLS_RSA_PRIVATE, udc_to_mbed_hashfunctions[hashMethod], hashLen, hash, signature) != 0, udR_InternalCryptoError);
      result = udBase64Encode(ppSignatureBase64, signature, pSigCtx->type / 8);
      UD_ERROR_HANDLE();
      break;
    case udCST_ECPBP384:
      UD_ERROR_IF(mbedtls_ecdsa_write_signature(&pSigCtx->ecdsa, udc_to_mbed_hashfunctions[hashMethod], hash, hashLen, signature, &sigLen, NULL, NULL) != 0, udR_InternalCryptoError);
      result = udBase64Encode(ppSignatureBase64, signature, sigLen);
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
udResult udCryptoSig_Verify(udCryptoSigContext *pSigCtx, const char *pHashBase64, const char *pSignatureBase64, udCryptoHashes hashMethod, udCryptoSigPadScheme pad)
{
  udResult result = udR_Failure_;
  unsigned char signature[udCST_RSA4096 / 8];
  unsigned char hash[udCHL_MaxHashLength];
  size_t hashLen;
  size_t sigLen;

  UD_ERROR_IF(hashMethod > udCH_Count, udR_InvalidParameter_);
  UD_ERROR_NULL(pSigCtx, udR_InvalidParameter_);
  UD_ERROR_NULL(pHashBase64, udR_InvalidParameter_);
  UD_ERROR_NULL(pSignatureBase64, udR_InvalidParameter_);

  UD_ERROR_CHECK(udBase64Decode(pHashBase64, 0, hash, sizeof(hash), &hashLen));

  switch (pSigCtx->type)
  {
    case udCST_RSA1024:
    case udCST_RSA2048:
    case udCST_RSA4096:
      if (pad == udCSPS_Deterministic)
        mbedtls_rsa_set_padding(&pSigCtx->rsa, MBEDTLS_RSA_PKCS_V15, 0);
      UD_ERROR_IF(sizeof(signature) < (size_t)(pSigCtx->type / 8), udR_InternalCryptoError);
      UD_ERROR_CHECK(udBase64Decode(pSignatureBase64, 0, signature, sizeof(signature), &sigLen));
      UD_ERROR_IF(mbedtls_rsa_rsassa_pkcs1_v15_verify(&pSigCtx->rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, udc_to_mbed_hashfunctions[hashMethod], hashLen, hash, signature) != 0, udR_SignatureMismatch);
      UD_ERROR_IF(sigLen != (size_t)(pSigCtx->type / 8), udR_InternalCryptoError);
      break;
    case udCST_ECPBP384:
      UD_ERROR_CHECK(udBase64Decode(pSignatureBase64, 0, signature, sizeof(signature), &sigLen));
      UD_ERROR_IF(mbedtls_ecdsa_read_signature(&pSigCtx->ecdsa, hash, hashLen, signature, sigLen) != 0, udR_SignatureMismatch);
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
      switch (pSigCtx->type)
      {
        case udCST_RSA1024:
        case udCST_RSA2048:
        case udCST_RSA4096:
          mbedtls_rsa_free(&pSigCtx->rsa);
          break;
        case udCST_ECPBP384:
          mbedtls_ecdsa_free(&pSigCtx->ecdsa);
          break;
        default:
          break;
      }
      udFreeSecure(pSigCtx, sizeof(*pSigCtx));
    }
  }
}

