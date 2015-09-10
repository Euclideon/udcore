// Include any system headers that any files that are also included by code inside the namespace wrap
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "udCrypto.h"

#if UDPLATFORM_WINDOWS
#pragma warning(disable: 4267 4244)
#endif

namespace udCrypto
{
  // Include public domain, license free source taken from https://github.com/B-Con/crypto-algorithms
#include "crypto/aes.c"
#include "crypto/sha1.c"
};

struct udCryptoCipherContext
{
  udCrypto::WORD keySchedule[60];
  int blockSize;
  int keyLengthInBits;
  udCryptoCiphers cipher;
  udCryptoPaddingMode padMode;
};

struct udCryptoHashContext
{
  udCryptoHashes hash;
  size_t hashLengthInBytes;
  union
  {
    udCrypto::SHA1_CTX sha1;
  };
};


// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_CreateCipher(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey)
{
  udResult result;
  udCryptoCipherContext *pCtx = nullptr;

  result = udR_InvalidParameter_;
  if (ppCtx == nullptr || pKey == nullptr)
    goto epilogue;

  result = udR_MemoryAllocationFailure;
  pCtx = udAllocType(udCryptoCipherContext, 1, udAF_Zero);
  if (pCtx == nullptr)
    goto epilogue;

  pCtx->cipher = cipher;
  pCtx->padMode = padMode;

  switch (cipher)
  {
    case udCC_AES128:
      udCrypto::aes_key_setup(pKey, pCtx->keySchedule, 128);
      pCtx->blockSize = 16;
      pCtx->keyLengthInBits = 128;
      break;
    case udCC_AES256:
      udCrypto::aes_key_setup(pKey, pCtx->keySchedule, 256);
      pCtx->blockSize = 16;
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
// Author: Dave Pevreal, December 2014
udResult udCrypto_EncryptECB(udCryptoCipherContext *pCtx, const uint8_t *pPlainText, size_t plainTextLen, uint8_t *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen)
{
  udResult result;
  size_t paddedCliperTextLen;

  result = udR_InvalidParameter_;
  if (!pCtx || !pPlainText || !pCipherText)
    goto epilogue;

  paddedCliperTextLen = plainTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((plainTextLen % pCtx->blockSize) != 0)
      {
        result = udR_BlockLimitExceeded; // TODO: Add better error code
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      result = udR_InvalidConfiguration;
      goto epilogue;
  }

  result = udR_BufferTooSmall;
  if (paddedCliperTextLen < cipherTextLen)
    goto epilogue;

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      for (size_t i = 0; i < plainTextLen; i += pCtx->blockSize)
        udCrypto::aes_encrypt(pPlainText + i, pCipherText + i, pCtx->keySchedule, pCtx->keyLengthInBits);
      break;
  }

  if (pPaddedCipherTextLen)
    *pPaddedCipherTextLen = paddedCliperTextLen;
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_DecryptECB(udCryptoCipherContext *pCtx, const uint8_t *pCipherText, size_t cipherTextLen, uint8_t *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen)
{
  udResult result;
  size_t actualPlainTextLen;

  result = udR_InvalidParameter_;
  if (!pCtx || !pPlainText || !pCipherText)
    goto epilogue;

  actualPlainTextLen = cipherTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((cipherTextLen % pCtx->blockSize) != 0)
      {
        result = udR_BlockLimitExceeded; // TODO: Add better error code
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      result = udR_InvalidConfiguration;
      goto epilogue;
  }

  result = udR_BufferTooSmall;
  if (actualPlainTextLen < plainTextLen)
    goto epilogue;

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      for (size_t i = 0; i < plainTextLen; i += pCtx->blockSize)
        udCrypto::aes_encrypt(pCipherText + i, pPlainText + i, pCtx->keySchedule, pCtx->keyLengthInBits);
      break;
  }

  if (pActualPlainTextLen)
    *pActualPlainTextLen = actualPlainTextLen;
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_EncryptCBC(udCryptoCipherContext *pCtx, const uint8_t *pIV, const uint8_t *pPlainText, size_t plainTextLen, uint8_t *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen)
{
  udResult result;
  size_t paddedCliperTextLen;

  result = udR_InvalidParameter_;
  if (!pCtx || !pPlainText || !pCipherText)
    goto epilogue;

  paddedCliperTextLen = plainTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((plainTextLen % pCtx->blockSize) != 0)
      {
        result = udR_BlockLimitExceeded; // TODO: Add better error code
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      result = udR_InvalidConfiguration;
      goto epilogue;
  }

  result = udR_BufferTooSmall;
  if (paddedCliperTextLen < cipherTextLen)
    goto epilogue;

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      if (!udCrypto::aes_encrypt_cbc(pPlainText, plainTextLen, pCipherText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV))
      {
        result = udR_Failure_;
        goto epilogue;
      }
      break;
  }

  if (pPaddedCipherTextLen)
    *pPaddedCipherTextLen = paddedCliperTextLen;
  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_DecryptCBC(udCryptoCipherContext *pCtx, const uint8_t *pIV, const uint8_t *pCipherText, size_t cipherTextLen, uint8_t *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen)
{
  udResult result;
  size_t actualPlainTextLen;

  result = udR_InvalidParameter_;
  if (!pCtx || !pPlainText || !pCipherText)
    goto epilogue;

  actualPlainTextLen = cipherTextLen;
  switch (pCtx->padMode)
  {
    case udCPM_None:
      if ((cipherTextLen % pCtx->blockSize) != 0)
      {
        result = udR_BlockLimitExceeded; // TODO: Add better error code
        goto epilogue;
      }
      break;
    // TODO: Add a padding mode
    default:
      result = udR_InvalidConfiguration;
      goto epilogue;
  }

  result = udR_BufferTooSmall;
  if (actualPlainTextLen > plainTextLen)
    goto epilogue;

  switch (pCtx->cipher)
  {
    case udCC_AES128:
    case udCC_AES256:
      if (!udCrypto::aes_decrypt_cbc(pCipherText, cipherTextLen, pPlainText, pCtx->keySchedule, pCtx->keyLengthInBits, pIV))
      {
        result = udR_Failure_;
        goto epilogue;
      }
      break;
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
udResult udCrypto_TestCipher(udCryptoCiphers cipher)
{
	uint8_t plaintext[2][32] = {
		{0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51}
	};
	uint8_t ciphertext[2][32] = {
		{0xf5,0x8c,0x4c,0x04,0xd6,0xe5,0xf1,0xba,0x77,0x9e,0xab,0xfb,0x5f,0x7b,0xfb,0xd6,0x9c,0xfc,0x4e,0x96,0x7e,0xdb,0x80,0x8d,0x67,0x9f,0x77,0x7b,0xc6,0x70,0x2c,0x7d}
	};
	uint8_t iv[1][16] = {
		{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}
	};
	uint8_t key[1][32] = {
		{0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4}
	};

  udResult result = udR_Failure_;
  udCryptoCipherContext *pCtx = nullptr;
  size_t actualCipherTextLen, actualPlainTextLen;

  if (cipher != udCC_AES256) // Only have a test for one thing at the moment
    goto epilogue;

  result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0]);
  if (result != udR_Success)
    goto epilogue;
  result = udCrypto_EncryptCBC(pCtx, iv[0], plaintext[0], sizeof(plaintext[0]), ciphertext[1], sizeof(ciphertext[1]), &actualCipherTextLen);
  if (result != udR_Success)
    goto epilogue;
  result = udCrypto_DestroyCipher(&pCtx);
  if (result != udR_Success)
    goto epilogue;
  if (memcmp(ciphertext[0], ciphertext[1], sizeof(ciphertext[0])) != 0)
  {
    udDebugPrintf("Encrypt error: ciphertext didn't match");
    result = udR_InternalError;
  }


  result = udCrypto_CreateCipher(&pCtx, udCC_AES256, udCPM_None, key[0]);
  if (result != udR_Success)
    goto epilogue;
  result = udCrypto_DecryptCBC(pCtx, iv[0], ciphertext[1], sizeof(ciphertext[1]), plaintext[1], sizeof(plaintext[1]), &actualPlainTextLen);
  if (result != udR_Success)
    goto epilogue;
  result = udCrypto_DestroyCipher(&pCtx);
  if (result != udR_Success)
    goto epilogue;
  if (memcmp(plaintext[0], plaintext[1], sizeof(plaintext[0])) != 0)
  {
    udDebugPrintf("Decrypt error: plaintext didn't match");
    result = udR_InternalError;
  }

  result = udR_Success;

epilogue:
  udCrypto_DestroyCipher(&pCtx);

  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, December 2014
udResult udCrypto_CreateHash(udCryptoHashContext **ppCtx, udCryptoHashes hash)
{
  udResult result;
  udCryptoHashContext *pCtx = nullptr;

  result = udR_InvalidParameter_;
  if (ppCtx == nullptr)
    goto epilogue;

  result = udR_MemoryAllocationFailure;
  pCtx = udAllocType(udCryptoHashContext, 1, udAF_Zero);
  if (pCtx == nullptr)
    goto epilogue;

  pCtx->hash = hash;
  switch (hash)
  {
    case udCH_SHA1:
      sha1_init(&pCtx->sha1);
      pCtx->hashLengthInBytes = SHA1_BLOCK_SIZE;
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

  switch (pCtx->hash)
  {
    case udCH_SHA1:
      sha1_update(&pCtx->sha1, (const uint8_t*)pBytes, length);
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

  switch (pCtx->hash)
  {
    case udCH_SHA1:
      sha1_final(&pCtx->sha1, pHash);
      break;
    default:
      return udR_InvalidParameter_;
  }

  if (pActualHashLength)
    *pActualHashLength = pCtx->hashLengthInBytes;

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
// Author: Dave Pevreal, December 2014
udResult udCrypto_TestHash(udCryptoHashes hash)
{
  udResult result = udR_Failure_;
  udCryptoHashContext *pCtx = nullptr;

  switch (hash)
  {
    case udCH_SHA1:
      {
	      char text1[] = {"abc"};
	      char text2[] = {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	      char text3[] = {"aaaaaaaaaa"};
	      uint8_t hash1[SHA1_BLOCK_SIZE] = {0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d};
	      uint8_t hash2[SHA1_BLOCK_SIZE] = {0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1};
	      uint8_t hash3[SHA1_BLOCK_SIZE] = {0x34,0xaa,0x97,0x3c,0xd4,0xc4,0xda,0xa4,0xf6,0x1e,0xeb,0x2b,0xdb,0xad,0x27,0x31,0x65,0x34,0x01,0x6f};
	      uint8_t buf[SHA1_BLOCK_SIZE];

        result = udCrypto_CreateHash(&pCtx, hash);
        if (result != udR_Success)
          goto epilogue;
	      result = udCrypto_Digest(pCtx, text1, strlen(text1));
        if (result != udR_Success)
          goto epilogue;
	      result = udCrypto_Finalise(pCtx, buf, sizeof(buf));
        if (result != udR_Success)
          goto epilogue;
        result = udCrypto_DestroyHash(&pCtx);
        if (result != udR_Success)
          goto epilogue;
        result = udR_Failure_;
	      if (memcmp(hash1, buf, SHA1_BLOCK_SIZE) != 0)
          goto epilogue;

        result = udCrypto_CreateHash(&pCtx, hash);
        if (result != udR_Success)
          goto epilogue;
	      result = udCrypto_Digest(pCtx, text2, strlen(text2));
        if (result != udR_Success)
          goto epilogue;
	      result = udCrypto_Finalise(pCtx, buf, sizeof(buf));
        if (result != udR_Success)
          goto epilogue;
        result = udCrypto_DestroyHash(&pCtx);
        if (result != udR_Success)
          goto epilogue;
        result = udR_Failure_;
	      if (memcmp(hash2, buf, SHA1_BLOCK_SIZE) != 0)
          goto epilogue;

        result = udCrypto_CreateHash(&pCtx, hash);
        if (result != udR_Success)
          goto epilogue;
	      for (int i = 0; i < 100000; ++i)
        {
	        result = udCrypto_Digest(pCtx, text3, strlen(text3));
          if (result != udR_Success)
            goto epilogue;
        }
	      result = udCrypto_Finalise(pCtx, buf, sizeof(buf));
        if (result != udR_Success)
          goto epilogue;
        result = udCrypto_DestroyHash(&pCtx);
        if (result != udR_Success)
          goto epilogue;
        result = udR_Failure_;
	      if (memcmp(hash3, buf, SHA1_BLOCK_SIZE) != 0)
          goto epilogue;

        result = udR_Success;
        break;
      }
    default:
      result = udR_Failure_;
  }

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
  unsigned char derivedKey[64];

  UD_ERROR_IF(!pPassword || !pKey, udR_InvalidParameter_);
  UD_ERROR_IF(keyLen > 20, udR_InvalidConfiguration); // Currently, only key lengths up to 20 bytes are supported, this can be extended if necessary

  // Hash the pass phrase
  UD_ERROR_CHECK(result = udCrypto_CreateHash(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, "ud1971", 6)); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, pPassword, strlen(pPassword))); // This is a special "salt" for our KDF to make it unique to UD
  UD_ERROR_CHECK(udCrypto_Finalise(pCtx, passPhraseDigest, sizeof(passPhraseDigest), &passPhraseDigestLen));
  UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));
  UD_ERROR_IF(passPhraseDigestLen != 20, udR_InternalError);

  // Create a buffer of constant 0x36 xor'd with pass phrase hash
  memset(derivedKey, 0x36, 64);  // We don't need to do the full algorithm as we only need the first 64 bytes
  for (int i = 0; i < passPhraseDigestLen; i++)   // The passPhraseDigestLen is 20 bytes (SHA1)
    derivedKey[i] ^= passPhraseDigest[i];

  // Hash the result again and this gives us the key
  UD_ERROR_CHECK(result = udCrypto_CreateHash(&pCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCrypto_Digest(pCtx, derivedKey, sizeof(derivedKey)));
  UD_ERROR_CHECK(udCrypto_Finalise(pCtx, derivedKey, sizeof(derivedKey)));
  UD_ERROR_CHECK(udCrypto_DestroyHash(&pCtx));

  memcpy(pKey, derivedKey, keyLen);
  result = udR_Success;

epilogue:
  udCrypto_DestroyHash(&pCtx);
  return result;
}
