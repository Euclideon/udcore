#include "udPlatform.h"


// **** Symmetric cipher algorithms ****

enum udCryptoCiphers
{
  udCC_AES128,
  udCC_AES256
};

enum udCryptoCipherKeyLength
{
  udCCKL_AES128KeyLength = 16,
  udCCKL_AES256KeyLength = 32,

  udCCKL_MaxKeyLength = udCCKL_AES256KeyLength
};

enum udCryptoPaddingMode
{
  udCPM_None
};

enum udCryptoChainMode
{
  udCCM_None,   // Sentinal meaning no chaining mode has been set yet
  udCCM_CBC,    // Sequential access, requires IV unique to each call to encrypt
  udCCM_CTR,    // Random access, requires a nonce unique to file
};

struct udCryptoCipherContext;

// Initialise a cipher
udResult udCrypto_CreateCipher(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey, udCryptoChainMode chainMode);

// Set the nonce (for CTR mode)
udResult udCrypto_SetNonce(udCryptoCipherContext *pCtx, const uint8_t *pNonce, int nonceLen);

// Set the Nonce/counter (for CTR mode)
udResult udCrypto_CreateIVForCTRMode(udCryptoCipherContext *pCtx, uint8_t *pIV, int ivLen, uint64_t counter);

// Encrypt/decrypt using current mode/iv/nonce. Optional pOutIV arameter only applicable to CBC mode
udResult udCrypto_Encrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen = nullptr, uint8_t *pOutIV = nullptr);
udResult udCrypto_Decrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen = nullptr, uint8_t *pOutIV = nullptr);

// Free resources
udResult udCrypto_DestroyCipher(udCryptoCipherContext **ppCtx);


// **** Hash algorithms ****

enum udCryptoHashes
{
  udCH_SHA1,
  udCH_SHA256,
  udCH_SHA512,
};

enum udCryptoHashLength
{
  udCHL_SHA1Length = 20,
  udCHL_SHA256Length = 32,
  udCHL_SHA512Length = 64,

  udCHL_MaxHashLength = udCHL_SHA512Length
};


struct udCryptoHashContext;

// Initialise a hash
udResult udCrypto_CreateHash(udCryptoHashContext **ppCtx, udCryptoHashes hash);

// Digest some bytes
udResult udCrypto_Digest(udCryptoHashContext *pCtx, const void *pBytes, size_t length);

// Digest some bytes
udResult udCrypto_Finalise(udCryptoHashContext *pCtx, uint8_t *pHash, size_t length, size_t *pActualHashLength = nullptr);

// Free resources
udResult udCrypto_DestroyHash(udCryptoHashContext **ppCtx);

// Helper to create/digest/finalise/destroy for a given block (or two) of data
udResult udCrypto_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, uint8_t *pHash, size_t hashLength,
                       size_t *pActualHashLength = nullptr, const void *pMessage2 = nullptr, size_t message2Length = 0);

// **** Key derivation functions ****

// Generate a key (max 40 bytes) from a plain-text password (compatible with CryptDeriveKey KDF)
udResult udCrypto_KDF(const char *pPassword, uint8_t *pKey, int keyLen);

// Generate a random key (max 32 bytes) using system entropy
udResult udCrypto_RandomKey(uint8_t *pKey, int keyLen);

// **** HMAC functions ****
udResult udCrypto_HMAC(udCryptoHashes hash, const uint8_t *pKey, size_t keyLen, const uint8_t *pMessage, size_t messageLength,
                       uint8_t *pHMAC, size_t hmacLength, size_t *pActualHMACLength = nullptr);

// **** Digital signature functions ****

struct udCryptoSigContext;
enum udCryptoSigType
{
  udCST_RSA2048 = 2048,
  udCST_RSA4096 = 4096,
};

enum udCryptoSigPadScheme
{
  udCSPS_Deterministic   // A deterministic signature that doesn't require entropy. For RSA PKCS #1.5
};

// Generate a random key-pair
udResult udCrypto_CreateSigKey(udCryptoSigContext **ppSigCtx, udCryptoSigType type);

// Import public/keypair
udResult udCrypto_ImportSigKey(udCryptoSigContext **pSigCtx, const char *pKeyText);

// Export a public/keypair as JSON
udResult udCrypto_ExportSigKey(udCryptoSigContext *pSigCtx, const char **ppKeyText, bool exportPrivate = false);

// Sign a hash, be sure to udFree the result signature string when finished
udResult udCrypto_Sign(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char **ppSignatureString, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Verify a signed hash
udResult udCrypto_VerifySig(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char *pSignatureString, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Destroy a signature context
void udCrypto_DestroySig(udCryptoSigContext **pSigCtx);


// Internal test of algorithms
udResult udCrypto_TestCipher(udCryptoCiphers cipher);
udResult udCrypto_TestHash(udCryptoHashes hash);
udResult udCrypto_TestHMAC(udCryptoHashes hash);

