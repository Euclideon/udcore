#include "udPlatform.h"


// **** Symmetric cipher algorithms ****

enum udCryptoCiphers
{
  udCC_AES128,  // 16-byte key
  udCC_AES256,  // 32-byte key
};

enum udCryptoPaddingMode
{
  udCPM_None
};

struct udCryptoCipherContext;

// Initialise a cipher
udResult udCrypto_CreateCipher(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey);

// Encrypt some data (depending on padding scheme, *pPaddedCipherTextLen may be greater than plainTextLen)
udResult udCrypto_EncryptCBC(udCryptoCipherContext *pCtx, const uint8_t *pIV, const uint8_t *pPlainText, size_t plainTextLen, uint8_t *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen = nullptr);

// Decrypt some data (depending on padding scheme, *pActualplainTextLen may be less than cipherTextLen)
udResult udCrypto_DecryptCBC(udCryptoCipherContext *pCtx, const uint8_t *pIV, const uint8_t *pCipherText, size_t cipherTextLen, uint8_t *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen = nullptr);

// Free resources
udResult udCrypto_DestroyCipher(udCryptoCipherContext **ppCtx);


// **** Hash algorithms ****

enum udCryptoHashes
{
  udCH_SHA1
};

struct udCryptoHashContext;

// Initialise a hash
udResult udCrypto_CreateHash(udCryptoHashContext **ppCtx, udCryptoHashes hash);

// Digest some bytes
udResult udCrypto_Digest(udCryptoHashContext *pCtx, const uint8_t *pBytes, size_t length);

// Digest some bytes
udResult udCrypto_Finalise(udCryptoHashContext *pCtx, uint8_t *pHash, size_t length, size_t *pActualHashLength = nullptr);

// Free resources
udResult udCrypto_DestroyHash(udCryptoHashContext **ppCtx);

// Internal test of algorithms
udResult udCrypto_TestCipher(udCryptoCiphers cipher);
udResult udCrypto_TestHash(udCryptoHashes hash);
