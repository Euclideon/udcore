#include "gtest/gtest.h"
#include "udCrypto.h"
#include "udJSON.h"
#include "udStringUtil.h"

TEST(udCryptoTests, AES_CBC_MonteCarlo)
{
  // Do the first only monte carlo tests for CBC mode (400 tests in official monte carlo)
  static const unsigned char aes_test_cbc_dec[2][16] =
  {
    { 0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73, 0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86 },
    { 0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75, 0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13 }
  };

  static const unsigned char aes_test_cbc_enc[2][16] =
  {
    { 0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84, 0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D },
    { 0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5, 0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0 }
  };

  static const char *pZeroKeys[2] =
  {
    "AAAAAAAAAAAAAAAAAAAAAA==", // 16-bytes of zeros
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=" // 32-bytes of zeros
  };
  unsigned char buf[64];
  udCryptoIV iv;
  unsigned char prv[16];
  udCryptoCipherContext *pCtx = nullptr;

  for (int i = 0; i < 4; i++)
  {
    int test256 = i >> 1;
    int testEncrypt = i & 1;

    memset(&iv, 0, sizeof(iv));
    memset(prv, 0, sizeof(prv));
    memset(buf, 0, sizeof(buf));

    udResult result = udCryptoCipher_Create(&pCtx, test256 ? udCC_AES256 : udCC_AES128, udCPM_None, pZeroKeys[test256], udCCM_CBC);
    EXPECT_EQ(udR_Success, result);

    if (!testEncrypt)
    {
      for (int j = 0; j < 10000; j++)
        udCryptoCipher_Decrypt(pCtx, &iv, buf, 16, buf, sizeof(buf), nullptr, &iv); // Note: specifically decrypting exactly 16 bytes, not sizeof(buf)

      EXPECT_EQ(0, memcmp(buf, aes_test_cbc_dec[test256], 16));
    }
    else
    {
      for (int j = 0; j < 10000; j++)
      {
        udCryptoCipher_Encrypt(pCtx, &iv, buf, 16, buf, sizeof(buf), nullptr, &iv); // Note: specifically encrypting exactly 16 bytes, not sizeof(buf)
        unsigned char tmp[16];
        memcpy(tmp, prv, 16);
        memcpy(prv, buf, 16);
        memcpy(buf, tmp, 16);
      }

      EXPECT_EQ(0, memcmp(prv, aes_test_cbc_enc[test256], 16));
    }
    EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pCtx));
  }
}

TEST(udCryptoTests, AES_CTR_MonteCarlo)
{
  // Do the first only monte carlo tests for CTR mode (400 tests in official monte carlo)
  static const char *aes_test_ctr_key[3] =
  {
    "rmhS+BIQZ8xL96V2VXfzng==", "fiQGeBf64NdD1s4fMlORYw==", "dpG+A15QIKisbmGFKfmg3A=="
  };

  static const udCryptoIV aes_test_ctr_nonce_counter[3] =
  {
    { { 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } },
    { { 0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59, 0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01 } },
    { { 0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F, 0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01 } }
  };

  static const unsigned char aes_test_ctr_pt[3][48] =
  {
    { 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62, 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67 },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
      0x20, 0x21, 0x22, 0x23 }
  };

  static const unsigned char aes_test_ctr_ct[3][48] =
  {
    { 0xE4, 0x09, 0x5D, 0x4F, 0xB7, 0xA7, 0xB3, 0x79, 0x2D, 0x61, 0x75, 0xA3, 0x26, 0x13, 0x11, 0xB8 },

    { 0x51, 0x04, 0xA1, 0x06, 0x16, 0x8A, 0x72, 0xD9, 0x79, 0x0D, 0x41, 0xEE, 0x8E, 0xDA, 0xD3, 0x88,
      0xEB, 0x2E, 0x1E, 0xFC, 0x46, 0xDA, 0x57, 0xC8, 0xFC, 0xE6, 0x30, 0xDF, 0x91, 0x41, 0xBE, 0x28 },

    { 0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9, 0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
      0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36, 0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
      0x25, 0xB2, 0x07, 0x2F }
  };

  static const int aes_test_ctr_len[3] = { 16, 32, 36 };

  unsigned char buf[64];
  udCryptoCipherContext *pCtx = nullptr;

  for (int i = 0; i < 4; i++)
  {
    int testNumber = i >> 1;
    int testEncrypt = i & 1;

    memset(buf, 0, sizeof(buf));

    udResult result = udCryptoCipher_Create(&pCtx, udCC_AES128, udCPM_None, aes_test_ctr_key[testNumber], udCCM_CTR); // We only test 128-bit for CTR mode
    EXPECT_EQ(udR_Success, result);

    if (!testEncrypt)
    {
      int len = aes_test_ctr_len[testNumber];
      memcpy(buf, aes_test_ctr_ct[testNumber], len);
      udCryptoCipher_Decrypt(pCtx, &aes_test_ctr_nonce_counter[testNumber], buf, len, buf, len);
      EXPECT_EQ(0, memcmp(buf, aes_test_ctr_pt[testNumber], len));
    }
    else
    {
      int len = aes_test_ctr_len[testNumber];
      memcpy(buf, aes_test_ctr_pt[testNumber], len);
      udCryptoCipher_Decrypt(pCtx, &aes_test_ctr_nonce_counter[testNumber], buf, len, buf, len);
      EXPECT_EQ(0, memcmp(buf, aes_test_ctr_ct[testNumber], len));
    }
    EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pCtx));
  }
}

TEST(udCryptoTests, CipherErrorCodes)
{
  udResult result;
  udCryptoCipherContext *pCtx = nullptr;
  unsigned char buf[64];
  udCryptoIV iv;
  static const char *pZeroKey = "AAAAAAAAAAAAAAAAAAAAAA=="; // 16-bytes of zeros

  memset(&iv, 0, sizeof(iv));
  memset(buf, 0, sizeof(buf));
  result = udCryptoCipher_Create(nullptr, udCC_AES128, udCPM_None, pZeroKey, udCCM_CTR);
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Create(&pCtx, udCC_AES128, udCPM_None, nullptr, udCCM_CTR);
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Create(&pCtx, udCC_AES128, udCPM_None, pZeroKey, udCCM_CTR);
  EXPECT_EQ(udR_Success, result);

  result = udCryptoCipher_Encrypt(nullptr, &iv, buf, sizeof(buf), buf, sizeof(buf)); // context
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Encrypt(pCtx, nullptr, buf, sizeof(buf), buf, sizeof(buf)); // iv
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Encrypt(pCtx, &iv, nullptr, sizeof(buf), buf, sizeof(buf)); // input null
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Encrypt(pCtx, &iv, buf, 1, buf, sizeof(buf)); // input alignment
  EXPECT_EQ(udR_AlignmentRequired, result);
  result = udCryptoCipher_Encrypt(pCtx, &iv, buf, sizeof(buf), nullptr, sizeof(buf)); // output null
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Encrypt(pCtx, &iv, buf, sizeof(buf), buf, sizeof(buf) - 1); // output size
  EXPECT_EQ(udR_BufferTooSmall, result);

  result = udCryptoCipher_Decrypt(nullptr, &iv, buf, sizeof(buf), buf, sizeof(buf)); // context
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Decrypt(pCtx, nullptr, buf, sizeof(buf), buf, sizeof(buf)); // iv
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Decrypt(pCtx, &iv, nullptr, sizeof(buf), buf, sizeof(buf)); // input null
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Decrypt(pCtx, &iv, buf, 1, buf, sizeof(buf)); // input alignment
  EXPECT_EQ(udR_AlignmentRequired, result);
  result = udCryptoCipher_Decrypt(pCtx, &iv, buf, sizeof(buf), nullptr, sizeof(buf)); // output null
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Decrypt(pCtx, &iv, buf, sizeof(buf), buf, sizeof(buf) - 1); // output size
  EXPECT_EQ(udR_BufferTooSmall, result);

  result = udCryptoCipher_Destroy(&pCtx);
  EXPECT_EQ(udR_Success, result);

  // Additional destructions of non-existent objects
  result = udCryptoCipher_Destroy(&pCtx);
  EXPECT_EQ(udR_InvalidParameter, result);
  result = udCryptoCipher_Destroy(nullptr);
  EXPECT_EQ(udR_InvalidParameter, result);
}


TEST(udCryptoTests, SHA)
{
  /*
  * FIPS-180-1 test vectors. See https://www.di-mgt.com.au/sha_testvectors.html
  */
  static const char* s_testMessages[3] =
  {
    "abc",
    "",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
  };

  static udCryptoHashes s_testHashes[] = { udCH_SHA1, udCH_SHA256, udCH_SHA512 };
  static const char *s_testHashResults[][3] =
  {
    { // SHA-1
      "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=",
      "2jmj7l5rSw0yVb/vlWAYkK/YBwk=",
      "hJg+RBw70m66rkqh+VEp5eVGcPE="
    },
    { // SHA-256
      "ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0=",
      "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=",
      "JI1qYdIGOLjlwCaTDD5gOaM85Flk/yFn9uzt1BnbBsE="
    },
    { // SHA-512
      "3a81oZNherrMQXNJriBBMRLm+k6JqX6iCp7u5ktV05ohkpkqJ0/BqDa6PCOj/uu9RU1EI2Q86A4qmslPpUyknw==",
      "z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg==",
      "IEqPxt2oLwoM7XvrjgikFlfBbvRosiioJ5vjMacDwzWW/RXBOxsH+aodO+pXeJygMa2Fx6cd1wNU7GMSOMo0RQ=="
    }
  };

  for (size_t shaType = 0; shaType < UDARRAYSIZE(s_testHashes); ++shaType)
  {
    for (size_t testNumber = 0; testNumber < UDARRAYSIZE(s_testMessages); ++testNumber)
    {
      const char *pResultHash = nullptr;
      size_t inputLength = udStrlen(s_testMessages[testNumber]);

      udResult result = udCryptoHash_Hash(s_testHashes[shaType], s_testMessages[testNumber], inputLength, &pResultHash);
      EXPECT_EQ(udR_Success, result);
      EXPECT_EQ(true, udStrEqual(pResultHash, s_testHashResults[shaType][testNumber]));
      udFree(pResultHash);

      // Do an additional test of digesting the string in two separate parts
      size_t length1 = inputLength / 2;
      size_t length2 = inputLength - length1;
      result = udCryptoHash_Hash(s_testHashes[shaType], s_testMessages[testNumber], length1, &pResultHash, s_testMessages[testNumber] + length1, length2);
      EXPECT_EQ(udR_Success, result);
      EXPECT_EQ(true, udStrEqual(pResultHash, s_testHashResults[shaType][testNumber]));
      udFree(pResultHash);
    }
  }
}

TEST(udCryptoTests, Self)
{
  EXPECT_EQ(udR_Success, udCryptoCipher_SelfTest(udCC_AES128));
  EXPECT_EQ(udR_Success, udCryptoCipher_SelfTest(udCC_AES256));
  EXPECT_EQ(udR_Success, udCryptoHash_SelfTest(udCH_SHA1));
  EXPECT_EQ(udR_Success, udCryptoHash_SelfTest(udCH_SHA256));
  EXPECT_EQ(udR_Success, udCryptoHash_SelfTest(udCH_SHA512));
  EXPECT_EQ(udR_Success, udCryptoHash_SelfTest(udCH_MD5));
}

TEST(udCryptoTests, RSACreateSig)
{
  static const char *pPrivateKeyText =
    "{\n"
    "  'Type': 'RSA',\n"
    "  'Private': true,\n"
    "  'Size': 2048,\n"
    "  'N': 'n0pMFfJ0/vkZc1DEm4e1T33XoQZYvJxzFskuQ6E7IvTrR/KlTCMAEsU6rm+vNJQBFt+vEiNbv9qrzTQaW5XHp6k9+hwvQfkLKd2moc+G+ru5inbyOBBNpRfMAUjf6VZkyLbDFYrIFp6SJZYXhx2Pf/HpUKxpte4ZDFJARGpTLvE4GtYPs2Gj9xu6C1yRan3nLSa8MJhndNDxNGh5IF92ThMw0I+Pk1YSzpiJ9ZEX8GLqrVzWgORVdeiaYot3YSSMbf5pcDhrsvCDNSFB5396L3fc7IcWy99e9wiZiVcPUvLlNE/ziInNapOwG5EgPqTUk1jtXyLzuHZQDNADl/N/Uw==',\n"
    "  'E': 'AQAB',\n"
    "  'D': 'A2GBUenuf8brul3Zfm+X8pL6M6m90msDqlUkzTyr06cdI07MIVyQ0NUs1Kz8LAKEL2caASmM9fp/MQDNGmqIbU+TSC629hCCIyZYNhEAjWvUmVLC+1ulOj7SDqjsT7iMtRHj/B4Q9yHweinAYBbJh+6rhBHUwI7IK1HHmWwkTde6GxKiNxp4hrsxP8AKgX2+lCvPH0YT0jwFdHafcEfFCg0Mk8lIrlZRuD/5XbDVqKdcgrss63AwJthU1cEZztHH19sQfTjp2bEUFGBGQETL/NcJIO+YKbkng1UVvkTDcbJtSZ+PZxrQvzv5EJ4K0YTp26RlNNgTau2feMfyP5o3CQ==',\n"
    "  'P': '9dJtUa+v2GIwqhCJqcvSR/FdG53rAYwPYu9lJPmKbPxuSbUhtBJh+OzdVGU9H+xxRrw8w95hgEyaPLdGXfZuaGy3mFRHzCI9lfsShIIk37yKbIYbFPpvMu6M9gcB5LwbpK2IBlYZcSMohKvADv0qRaReWWLoIeVpj3M6tTZLaRU=',\n"
    "  'Q': 'peKuzv6ZYtNcERg3hxWyyWTw9TdbTHRJnmA8lVHb+JqxHipcp9uvcZBxHJiSqYY9nmdWPrEHyQIzXvEIclKtjYboKBt/bNfAZ1iLCR7bkX9wHQj6/PmAI9iFvzvvoQbC05C/RZkX3Ug00p7pR8/5qSYk72jC9hRgasNIjlOVkMc=',\n"
    "  'DP': 'vTLVUt6+n/OK8wnBer9WPGsHt37G5qzvFr2cgmXR5eov1Gkl5JuVbmqYOyGkdxKbaM7uke5x6raKq5p//UfzWEn80LBlhjcAYZQZf4VPbiiF/dsFsxLBTVkPgziHe45QVGH/ZKkV8d8Wi25JZv/xbiKBP5kBgz04DuGoWNrOFbU=',\n"
    "  'DQ': 'ePmRpl9CGTIuqEDS7e7DDeBRYWNXb7A2qAti4zppgym9FVSrcbbigZ1nAAW8n2jIsyaFXP7ZwJucPxbkpArripTh5a34BbZqGHQYITShx7/6URJlh+ukqX+UOlxJa1N07blX5De7kaLA8wD0+2wOlG6+7OGnnLJLhlCYL0OBha0=',\n"
    "  'QP': 'YSPeV0tloK7H8XGZ95KCixJfNYkhT29Lcldbm2kUbtk/rChH5OjGwINMc8CewH8/mZDAD7ZpyU9UyWyfK6PpYUjLguvsdmCWuXGVsURRj6MNsv6rHWjyGpfgxLTe2dUKK/xgIOd1mATTUpM3S3q3HRjccy0IfyTh2HdFHILUorU='\n"
    "}\n";

  static const char *pPublicKeyText =
    "{\n"
    "  'Type': 'RSA',\n"
    "  'Size': 2048,\n"
    "  'N': 'n0pMFfJ0/vkZc1DEm4e1T33XoQZYvJxzFskuQ6E7IvTrR/KlTCMAEsU6rm+vNJQBFt+vEiNbv9qrzTQaW5XHp6k9+hwvQfkLKd2moc+G+ru5inbyOBBNpRfMAUjf6VZkyLbDFYrIFp6SJZYXhx2Pf/HpUKxpte4ZDFJARGpTLvE4GtYPs2Gj9xu6C1yRan3nLSa8MJhndNDxNGh5IF92ThMw0I+Pk1YSzpiJ9ZEX8GLqrVzWgORVdeiaYot3YSSMbf5pcDhrsvCDNSFB5396L3fc7IcWy99e9wiZiVcPUvLlNE/ziInNapOwG5EgPqTUk1jtXyLzuHZQDNADl/N/Uw==',\n"
    "  'E': 'AQAB',\n"
    "}\n";

  udCryptoSigContext *pPrivCtx = nullptr;
  udCryptoSigContext *pPubCtx = nullptr;
  static const char *pMessage = "No problem can be solved from the same level of consciousness that created it. -Einstein";
  static const char *pExpectedSignature = "jZzErtTWtEjEIDJsf0HRPJA/9z2MLu2tCvd/OgOojinZ9y+hbQSADgnFVN/cGV965Z6x6burvYVWPT8TjF00+9aXIGY3vFncrdRYhM2ynq+cisOxzIacC1AraDcuQgMvyxv9NKjCNWvLJL1GaP6PzKJmuDT5HDiEz9DqNhya5U43dIFoXu3AlTZlT8uuUuSwbxk4G+rkfG2Jm6cj6l8BhvpMhxA7GYmuiA9ByQ7tcU8/G/zi1xY/zIHTOlzGc7h+BfXXkpNH+NdyMOW0NnQg1jfy5VuK3eYeDfP2xSDlvOivvXwwFR9lQCizx4kzSuSpoIg7pW0HvmjgXXcMC29kqg==";
  const char *pHash = nullptr;
  const char *pSignature = nullptr;

  EXPECT_EQ(udR_Success, udCrypto_Init());
  EXPECT_EQ(udR_Success, udCryptoHash_Hash(udCH_SHA1, pMessage, udStrlen(pMessage), &pHash));

#if 0 // Enable to generate a new key
  EXPECT_EQ(udR_Success, udCryptoSig_CreateKeyPair(&pPrivCtx, udCST_RSA2048));
  EXPECT_EQ(udR_Success, udCryptoSig_ExportKeyPair(pPrivCtx, &pPrivateKeyText, true));
  udDebugPrintf("Private key:\n%s\n", pPrivateKeyText);
  EXPECT_EQ(udR_Success, udCryptoSig_ExportKeyPair(pPrivCtx, &pPublicKeyText, false));
  udDebugPrintf("Public key:\n%s\n", pPublicKeyText);
  EXPECT_EQ(udR_Success, udCryptoSig_Sign(pPrivCtx, pHash, &pExpectedSignature, udCH_SHA1));
  udDebugPrintf("Expected signature:\n%s\n", pExpectedSignature);
#else
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPrivCtx, pPrivateKeyText));
#endif
  // Import the public key only
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPubCtx, pPublicKeyText));

  // Sign a message using the private key
  EXPECT_EQ(udR_Success, udCryptoSig_Sign(pPrivCtx, pHash, &pSignature, udCH_SHA1));

  //udDebugPrintf("Current Sig: \n%s\n", pSignature);

  // Verify it's the expected signature (only works with PKCS_15)
  EXPECT_EQ(0, udStrcmp(pSignature, pExpectedSignature));

  // Verify using the private key
  EXPECT_EQ(udR_Success, udCryptoSig_Verify(pPrivCtx, pHash, pSignature, udCH_SHA1));

  // Verify the message using the public key
  EXPECT_EQ(udR_Success, udCryptoSig_Verify(pPubCtx, pHash, pSignature, udCH_SHA1));

  // Change the hash slightly to ensure the message isn't verified
  ((char*)pHash)[1] ^= 1;
  EXPECT_EQ(udR_SignatureMismatch, udCryptoSig_Verify(pPrivCtx, pHash, pSignature, udCH_SHA1));
  EXPECT_EQ(udR_SignatureMismatch, udCryptoSig_Verify(pPubCtx, pHash, pSignature, udCH_SHA1));

  udCryptoSig_Destroy(&pPrivCtx);
  udCryptoSig_Destroy(&pPubCtx);
  udFree(pHash);
  udFree(pSignature);

  udCrypto_Deinit();
}

TEST(udCryptoTests, ECDSADigiSig)
{
  udCryptoSigContext *pPrivCtx = nullptr;
  udCryptoSigContext *pPubCtx = nullptr;
  static const char *pMessage = "No problem can be solved from the same level of consciousness that created it. -Einstein";
  static const char *pExpectedSignature = "MGQCMBUQ3UEJCsTpWveYHKtNj1TQcAh1i3uiIYdUUvVrVR1+ZgW7RvIoaPDLPj0NP0+OpwIwLGR5Tc9Qp8XEV3iemWsxhcXG5L1dS6Us8doQ4CSdrLLj2zWgbz1tXXv2sBmxT6bh";
  const char *pHash = nullptr;
  const char *pSignature = nullptr;

  static const char *pPrivateKeyText = R"key({
  "Type": "ECDSA",
  "Curve": "BP384R1",
  "Private": true,
  "X": "CRsaWnhJdlED7s82LjUUvomBxU5frr2FcoEiKCbmuKlnjToyKMcGLtuhliIqsIaO",
  "Y": "el3DpHAOA7bC7O/3TjN81STeOpOBOlQ7WIjPGHJxLbIhDlMm63GCMUPczB5S1IJi",
  "Z": "AQ==",
  "D": "LP+e429tIN/RzkeGyrYW6kmJ7WyAOHUOd8cnfb+z/ciPelesB+85qQyVeggOHWfX"
})key";

  static const char *pPublicKeyText = R"key({
  "Type": "ECDSA",
  "Curve": "BP384R1",
  "X": "CRsaWnhJdlED7s82LjUUvomBxU5frr2FcoEiKCbmuKlnjToyKMcGLtuhliIqsIaO",
  "Y": "el3DpHAOA7bC7O/3TjN81STeOpOBOlQ7WIjPGHJxLbIhDlMm63GCMUPczB5S1IJi",
  "Z": "AQ=="
})key";

  EXPECT_EQ(udR_Success, udCrypto_Init());
  EXPECT_EQ(udR_Success, udCryptoHash_Hash(udCH_SHA1, pMessage, udStrlen(pMessage), &pHash));

#if 0 // Enable to generate a new key
  EXPECT_EQ(udR_Success, udCryptoSig_CreateKeyPair(&pPrivCtx, udCST_ECPBP384));
  EXPECT_EQ(udR_Success, udCryptoSig_ExportKeyPair(pPrivCtx, &pPrivateKeyText, true));
  udDebugPrintf("Private key:\n%s\n", pPrivateKeyText);
  EXPECT_EQ(udR_Success, udCryptoSig_ExportKeyPair(pPrivCtx, &pPublicKeyText, false));
  udDebugPrintf("Public key:\n%s\n", pPublicKeyText);
  EXPECT_EQ(udR_Success, udCryptoSig_Sign(pPrivCtx, pHash, &pSignature, udCH_SHA1));
  udDebugPrintf("Actual signature:\n%s\n", pSignature);
  udDebugPrintf("Expected signature:\n%s\n", pExpectedSignature);
#else
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPrivCtx, pPrivateKeyText));
#endif
  // Import the public key only
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPubCtx, pPublicKeyText));

  // Sign a message using the private key
  EXPECT_EQ(udR_Success, udCryptoSig_Sign(pPrivCtx, pHash, &pSignature, udCH_SHA1));

  // Verify it's the expected signature (only works with PKCS_15)
  EXPECT_TRUE(udStrEqual(pSignature, pExpectedSignature));

  // Verify using the private key
  EXPECT_EQ(udR_Success, udCryptoSig_Verify(pPrivCtx, pHash, pSignature, udCH_SHA1));

  // Verify the message using the public key
  EXPECT_EQ(udR_Success, udCryptoSig_Verify(pPubCtx, pHash, pSignature, udCH_SHA1));

  // Change the hash slightly to ensure the message isn't verified
  ((char*)pHash)[1] ^= 1;
  EXPECT_EQ(udR_SignatureMismatch, udCryptoSig_Verify(pPrivCtx, pHash, pSignature, udCH_SHA1));
  EXPECT_EQ(udR_SignatureMismatch, udCryptoSig_Verify(pPubCtx, pHash, pSignature, udCH_SHA1));

  udCryptoSig_Destroy(&pPrivCtx);
  udCryptoSig_Destroy(&pPubCtx);
  // Additional destructions of non-existent objects
  udCryptoSig_Destroy(&pPubCtx);
  udCryptoSig_Destroy(nullptr);
  udFree(pHash);
  udFree(pSignature);
  udCrypto_Deinit();
}

TEST(udCryptoTests, ECDSADigiSigFromudCloud)
{
  udCryptoSigContext *pPubCtx = nullptr;
  static const char *pMessage = "No problem can be solved from the same level of consciousness that created it. -Einstein";
  const char *pExpectedSignature = "MGQCMCnZx1A9ZVXLL0rHTDOwCtIRI8ml+YQEJ+DtpJAP1FS+b45OAiVUETtLqzfWGd0MiAIwblWTglFDHYPtVMfN6WNtlNd6JB3Kcg3OEDej4pNDZOibTixCyZaBaGRazit4tKWV";
  const char *pHash = nullptr;

  static const char *pPublicKeyText = R"key({
    "Type": "ECDSA",
    "Curve": "BP384R1",
    "X": "CRsaWnhJdlED7s82LjUUvomBxU5frr2FcoEiKCbmuKlnjToyKMcGLtuhliIqsIaO",
    "Y": "el3DpHAOA7bC7O/3TjN81STeOpOBOlQ7WIjPGHJxLbIhDlMm63GCMUPczB5S1IJi",
    "Z": "AQ=="
  })key";

  EXPECT_EQ(udR_Success, udCrypto_Init());
  EXPECT_EQ(udR_Success, udCryptoHash_Hash(udCH_SHA256, pMessage, udStrlen(pMessage), &pHash));

  // Import the public key only
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPubCtx, pPublicKeyText));

  // Verify the message
  EXPECT_EQ(udR_Success, udCryptoSig_Verify(pPubCtx, pHash, pExpectedSignature, udCH_SHA256));

  // Change the hash slightly to ensure the message isn't verified
  ((char*)pHash)[1] ^= 1;
  EXPECT_EQ(udR_SignatureMismatch, udCryptoSig_Verify(pPubCtx, pHash, pExpectedSignature, udCH_SHA1));

  udCryptoSig_Destroy(&pPubCtx);
  udFree(pHash);
  udCrypto_Deinit();
}

TEST(udCryptoTests, DHM)
{
  udResult result;
  udJSON publicA, publicB;
  udCryptoDHMContext *pDHM = nullptr;
  const char *pPublicValueA = nullptr;
  const char *pPublicValueB = nullptr;
  const char *pSecretA;
  const char *pSecretB;
  const size_t keyLen = 64; // Maximum length secret

  EXPECT_EQ(udR_Success, udCrypto_Init());

  result = udCryptoKey_CreateDHM(&pDHM, &pPublicValueA, keyLen);
  EXPECT_EQ(udR_Success, result);

  result = udCryptoKey_DeriveFromPartyA(pPublicValueA, &pPublicValueB, &pSecretB);
  EXPECT_EQ(udR_Success, result);

  result = udCryptoKey_DeriveFromPartyB(pDHM, pPublicValueB, &pSecretA);
  EXPECT_EQ(udR_Success, result);

  EXPECT_EQ(true, udStrEqual(pSecretA, pSecretB));

  EXPECT_EQ(udR_Success, publicA.Parse(pPublicValueA));
  EXPECT_EQ(udR_Success, publicB.Parse(pPublicValueB));
  EXPECT_EQ(2, publicA.MemberCount());
  EXPECT_EQ(publicA.Get("keyLen").AsInt(), (int)keyLen);
  EXPECT_EQ(1, publicB.MemberCount());
  EXPECT_NE(true, udStrEqual(publicA.Get("PublicValue").AsString(), publicB.Get("PublicValue").AsString()));

  udFree(pPublicValueA);
  udFree(pPublicValueB);
  udCryptoKey_DestroyDHM(&pDHM);

  // Finally, generate another secret (just to secretB) and make sure it's different from the previous one
  result = udCryptoKey_CreateDHM(&pDHM, &pPublicValueA, keyLen);
  EXPECT_EQ(udR_Success, result);
  udFree(pSecretB);
  result = udCryptoKey_DeriveFromPartyA(pPublicValueA, &pPublicValueB, &pSecretB);
  EXPECT_EQ(udR_Success, result);
  udFree(pPublicValueA);
  udFree(pPublicValueB);
  udCryptoKey_DestroyDHM(&pDHM);
  EXPECT_NE(true, udStrEqual(pSecretA, pSecretB));
  udFree(pSecretA);
  udFree(pSecretB);

  // Additional destructions of non-existent objects
  udCryptoKey_DestroyDHM(&pDHM);
  udCryptoKey_DestroyDHM(nullptr);

  udCrypto_Deinit();
}

TEST(udCryptoTests, Utilities)
{
  static const char *pZeros = "AAAAAAAAAAAAAAAAAAAAAA=="; // 16-bytes of zeros
  uint64_t rand1, rand2;
  const char *pTestStr = udStrdup(pZeros);

  udCrypto_Init();
  EXPECT_EQ(udR_Success, udCrypto_Random(&rand1, sizeof(rand1)));
  EXPECT_EQ(udR_Success, udCrypto_Random(&rand2, sizeof(rand2)));
  EXPECT_TRUE(rand1 != rand2);
  udCrypto_Deinit();

  EXPECT_NE(nullptr, pTestStr);
  if (pTestStr)
  {
    EXPECT_TRUE(udStrEqual(pZeros, pTestStr));
    udCrypto_Obscure(pTestStr);
    EXPECT_FALSE(udStrEqual(pZeros, pTestStr));
    udCrypto_Obscure(pTestStr);
    EXPECT_TRUE(udStrEqual(pZeros, pTestStr));

    const char *pOldPointer = pTestStr;
    udCrypto_FreeSecure(pTestStr);
    EXPECT_EQ(nullptr, pTestStr);
#if UDPLATFORM_WINDOWS
    EXPECT_NE(0, memcmp(pOldPointer, pZeros, udStrlen(pZeros) + 1));
#else
    udUnused(pOldPointer);
#endif
  }
}

TEST(udCryptoTests, PKCS7)
{
  static const udCryptoIV iv = { { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f } };
  static const char *pPlainText = "There are two great days in every person's life; the day we are born and the day we discover why"; // Multiple of 16 characters
  char encryptedText[96 + 16];
  char decryptedText[96]; // Importantly don't add 16 here so that codepaths dealing with pad overrun are tested
  size_t encryptedTextLen, decryptedTextLen;
  udCryptoCipherContext *pEncCtx = nullptr;
  udCryptoCipherContext *pDecCtx = nullptr;
  const char *pKeyBase64 = nullptr;

  EXPECT_EQ(udR_Success, udCryptoKey_DeriveFromPassword(&pKeyBase64, udCCKL_AES128KeyLength, "password"));
  for (udCryptoChainMode mode = udCCM_CTR; mode != udCCM_None; mode = (udCryptoChainMode)(mode - 1))
  {
    EXPECT_EQ(udR_Success, udCryptoCipher_Create(&pEncCtx, udCC_AES128, udCPM_PKCS7, pKeyBase64, mode));
    EXPECT_EQ(udR_Success, udCryptoCipher_Create(&pDecCtx, udCC_AES128, udCPM_PKCS7, pKeyBase64, mode));
    for (size_t i = 0; i <= udStrlen(pPlainText); ++i)
    {
      memset(encryptedText, 0, udLengthOf(encryptedText));
      memset(decryptedText, 0, udLengthOf(decryptedText));
      EXPECT_EQ(udR_Success, udCryptoCipher_Encrypt(pEncCtx, &iv, pPlainText, i, encryptedText, udLengthOf(encryptedText), &encryptedTextLen));
      EXPECT_EQ((i + 16) & ~15, encryptedTextLen);
      EXPECT_EQ(udR_Success, udCryptoCipher_Decrypt(pDecCtx, &iv, encryptedText, encryptedTextLen, decryptedText, udLengthOf(decryptedText), &decryptedTextLen));
      EXPECT_EQ(i, decryptedTextLen);
      EXPECT_EQ(0, memcmp(decryptedText, pPlainText, i));
    }
    EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pEncCtx));
    EXPECT_EQ(udR_Success, udCryptoCipher_Destroy(&pDecCtx));
  }
  udFree(pKeyBase64);
}

TEST(udCryptoTests, Obscure)
{
  static const char *pAllBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=ABCDEFGH";

  // Loop so that each character is xor'd with each combination of index
  for (size_t i = 0; i < 8; ++i)
  {
    const char *pStr = udStrdup(pAllBase64 + i);
    size_t len = udStrlen(pStr);
    udCrypto_Obscure(pStr);
    EXPECT_EQ(len, udStrlen(pStr));
    EXPECT_STRCASENE(pAllBase64 + i, pStr);
    udCrypto_Obscure(pStr);
    EXPECT_EQ(len, udStrlen(pStr));
    EXPECT_STRCASEEQ(pAllBase64 + i, pStr);
    udFree(pStr);
  }
}

// Test the returns of functions having not called udCrypto_Init()
TEST(udCryptoTests, Uninitialised)
{
  const char *pHash = nullptr;
  const char *pTempString = nullptr;
  udCryptoDHMContext *pDHMCtx = nullptr;
  udCryptoSigContext *pPrivCtx = nullptr;
  udCryptoCipherContext *pCipherCtx = nullptr;
  static const char *pPrivateKeyText =
    "{\n"
    "  'Type': 'RSA',\n"
    "  'Private': true,\n"
    "  'Size': 2048,\n"
    "  'N': 'n0pMFfJ0/vkZc1DEm4e1T33XoQZYvJxzFskuQ6E7IvTrR/KlTCMAEsU6rm+vNJQBFt+vEiNbv9qrzTQaW5XHp6k9+hwvQfkLKd2moc+G+ru5inbyOBBNpRfMAUjf6VZkyLbDFYrIFp6SJZYXhx2Pf/HpUKxpte4ZDFJARGpTLvE4GtYPs2Gj9xu6C1yRan3nLSa8MJhndNDxNGh5IF92ThMw0I+Pk1YSzpiJ9ZEX8GLqrVzWgORVdeiaYot3YSSMbf5pcDhrsvCDNSFB5396L3fc7IcWy99e9wiZiVcPUvLlNE/ziInNapOwG5EgPqTUk1jtXyLzuHZQDNADl/N/Uw==',\n"
    "  'E': 'AQAB',\n"
    "  'D': 'A2GBUenuf8brul3Zfm+X8pL6M6m90msDqlUkzTyr06cdI07MIVyQ0NUs1Kz8LAKEL2caASmM9fp/MQDNGmqIbU+TSC629hCCIyZYNhEAjWvUmVLC+1ulOj7SDqjsT7iMtRHj/B4Q9yHweinAYBbJh+6rhBHUwI7IK1HHmWwkTde6GxKiNxp4hrsxP8AKgX2+lCvPH0YT0jwFdHafcEfFCg0Mk8lIrlZRuD/5XbDVqKdcgrss63AwJthU1cEZztHH19sQfTjp2bEUFGBGQETL/NcJIO+YKbkng1UVvkTDcbJtSZ+PZxrQvzv5EJ4K0YTp26RlNNgTau2feMfyP5o3CQ==',\n"
    "  'P': '9dJtUa+v2GIwqhCJqcvSR/FdG53rAYwPYu9lJPmKbPxuSbUhtBJh+OzdVGU9H+xxRrw8w95hgEyaPLdGXfZuaGy3mFRHzCI9lfsShIIk37yKbIYbFPpvMu6M9gcB5LwbpK2IBlYZcSMohKvADv0qRaReWWLoIeVpj3M6tTZLaRU=',\n"
    "  'Q': 'peKuzv6ZYtNcERg3hxWyyWTw9TdbTHRJnmA8lVHb+JqxHipcp9uvcZBxHJiSqYY9nmdWPrEHyQIzXvEIclKtjYboKBt/bNfAZ1iLCR7bkX9wHQj6/PmAI9iFvzvvoQbC05C/RZkX3Ug00p7pR8/5qSYk72jC9hRgasNIjlOVkMc=',\n"
    "  'DP': 'vTLVUt6+n/OK8wnBer9WPGsHt37G5qzvFr2cgmXR5eov1Gkl5JuVbmqYOyGkdxKbaM7uke5x6raKq5p//UfzWEn80LBlhjcAYZQZf4VPbiiF/dsFsxLBTVkPgziHe45QVGH/ZKkV8d8Wi25JZv/xbiKBP5kBgz04DuGoWNrOFbU=',\n"
    "  'DQ': 'ePmRpl9CGTIuqEDS7e7DDeBRYWNXb7A2qAti4zppgym9FVSrcbbigZ1nAAW8n2jIsyaFXP7ZwJucPxbkpArripTh5a34BbZqGHQYITShx7/6URJlh+ukqX+UOlxJa1N07blX5De7kaLA8wD0+2wOlG6+7OGnnLJLhlCYL0OBha0=',\n"
    "  'QP': 'YSPeV0tloK7H8XGZ95KCixJfNYkhT29Lcldbm2kUbtk/rChH5OjGwINMc8CewH8/mZDAD7ZpyU9UyWyfK6PpYUjLguvsdmCWuXGVsURRj6MNsv6rHWjyGpfgxLTe2dUKK/xgIOd1mATTUpM3S3q3HRjccy0IfyTh2HdFHILUorU='\n"
    "}\n";
  static const char *pExpectedSignature = "jZzErtTWtEjEIDJsf0HRPJA/9z2MLu2tCvd/OgOojinZ9y+hbQSADgnFVN/cGV965Z6x6burvYVWPT8TjF00+9aXIGY3vFncrdRYhM2ynq+cisOxzIacC1AraDcuQgMvyxv9NKjCNWvLJL1GaP6PzKJmuDT5HDiEz9DqNhya5U43dIFoXu3AlTZlT8uuUuSwbxk4G+rkfG2Jm6cj6l8BhvpMhxA7GYmuiA9ByQ7tcU8/G/zi1xY/zIHTOlzGc7h+BfXXkpNH+NdyMOW0NnQg1jfy5VuK3eYeDfP2xSDlvOivvXwwFR9lQCizx4kzSuSpoIg7pW0HvmjgXXcMC29kqg==";
  udCryptoIV iv;

  // Hash and cipher functions work uninitialised
  EXPECT_EQ(udR_Success, udCryptoHash_Hash(udCH_SHA1, "Message", 7, &pHash));
  EXPECT_EQ(udR_Success, udCryptoHash_HMAC(udCH_SHA1, "SmVmZQ==", "Message", 7, &pTempString));
  udFree(pTempString);
  EXPECT_EQ(udR_Success, udCryptoKey_DeriveFromPassword(&pTempString, 16, "Password123"));
  udFree(pTempString);

  memset(&iv, 0xaa, sizeof(iv));
  EXPECT_EQ(udR_Success, udCryptoCipher_Create(&pCipherCtx, udCC_AES128, udCPM_None, "AAAAAAAAAAAAAAAAAAAAAA==", udCCM_CTR));
  EXPECT_EQ(udR_Success, udCryptoCipher_Encrypt(pCipherCtx, &iv, iv.iv, sizeof(iv.iv), iv.iv, sizeof(iv.iv)));
  udCryptoCipher_Destroy(&pCipherCtx);

  // Random (and therefore) most signature functions don't work, other than importing and exporting
  EXPECT_EQ(udR_NotInitialized, udCrypto_Random(iv.iv, sizeof(iv.iv)));
  EXPECT_EQ(udR_NotInitialized, udCryptoSig_CreateKeyPair(&pPrivCtx, udCST_RSA2048));
  EXPECT_EQ(udR_Success, udCryptoSig_ImportKeyPair(&pPrivCtx, pPrivateKeyText));
  EXPECT_EQ(udR_NotInitialized, udCryptoSig_Sign(pPrivCtx, pHash, &pTempString, udCH_SHA1));
  EXPECT_EQ(udR_NotInitialized, udCryptoSig_Verify(pPrivCtx, pHash, pExpectedSignature, udCH_SHA1));
  EXPECT_EQ(udR_Success, udCryptoSig_ExportKeyPair(pPrivCtx, &pTempString, false));
  udFree(pTempString);
  udCryptoSig_Destroy(&pPrivCtx);

  // Diffie-Hellman requires initialisation
  EXPECT_EQ(udR_NotInitialized, udCryptoKey_CreateDHM(&pDHMCtx, &pTempString, 64));

  udFree(pHash);
}
