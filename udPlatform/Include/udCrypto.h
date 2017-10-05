#include "udPlatform.h"


// **** Symmetric cipher algorithms ****

enum udCryptoCiphers
{
  udCC_AES128,
  udCC_AES256,
  udCC_Count
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
udResult udCryptoCipher_Create(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const uint8_t *pKey, udCryptoChainMode chainMode);

// Set the nonce (for CTR mode)
udResult udCrypto_SetNonce(udCryptoCipherContext *pCtx, const uint8_t *pNonce, int nonceLen);

// Set the Nonce/counter (for CTR mode)
udResult udCrypto_CreateIVForCTRMode(udCryptoCipherContext *pCtx, uint8_t *pIV, int ivLen, uint64_t counter);

// Encrypt/decrypt using current mode/iv/nonce. Optional pOutIV arameter only applicable to CBC mode
udResult udCryptoCipher_Encrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen = nullptr, uint8_t *pOutIV = nullptr);
udResult udCryptoCipher_Decrypt(udCryptoCipherContext *pCtx, const uint8_t *pIV, size_t ivLen, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen = nullptr, uint8_t *pOutIV = nullptr);

// Free resources
udResult udCryptoCipher_Destroy(udCryptoCipherContext **ppCtx);

// Internal self-test
udResult udCryptoCipher_SelfTest(udCryptoCiphers cipher);


// **** Hash algorithms ****

enum udCryptoHashes
{
  udCH_SHA1,
  udCH_SHA256,
  udCH_SHA512,
  udCH_Count
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
udResult udCryptoHash_Create(udCryptoHashContext **ppCtx, udCryptoHashes hash);

// Digest some bytes
udResult udCryptoHash_Digest(udCryptoHashContext *pCtx, const void *pBytes, size_t length);

// Digest some bytes
udResult udCryptoHash_Finalise(udCryptoHashContext *pCtx, uint8_t *pHash, size_t length, size_t *pActualHashLength = nullptr);

// Free resources
udResult udCryptoHash_Destroy(udCryptoHashContext **ppCtx);

// Helper to create/digest/finalise/destroy for a given block (or two) of data
udResult udCryptoHash_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, uint8_t *pHash, size_t hashLength,
                           size_t *pActualHashLength = nullptr, const void *pMessage2 = nullptr, size_t message2Length = 0);

// Generate a keyed hash
udResult udCryptoHash_HMAC(udCryptoHashes hash, const void *pKey, size_t keyLen, const void *pMessage, size_t messageLength,
                           uint8_t *pHMAC, size_t hmacLength, size_t *pActualHMACLength = nullptr);

// Internal self-test
udResult udCryptoHash_SelfTest(udCryptoHashes hash);


// **** Key derivation/exchange functions ****

struct udCryptoDHMContext;

// Generate a key (max 40 bytes) from a plain-text password (compatible with CryptDeriveKey KDF)
udResult udCryptoKey_DeriveFromPassword(const char *pPassword, void *pKey, size_t keyLen);

// Generate key data from supplied data (supplied data hashed and rehashed to required length)
udResult udCryptoKey_DeriveFromData(void *pKey, size_t keyLen, const void *pData, size_t dataLen);

// Generate a random key (max 32 bytes) using system entropy
udResult udCryptoKey_DeriveFromRandom(void *pKey, size_t keyLen);

// Create a Diffie-Hellman-Merkle key exchange context, generating a string of public values for the other party. Party A calls this.
udResult udCryptoKey_CreateDHM(udCryptoDHMContext **ppDHMCtx, const char **ppPublicValueA, size_t secretLen);

// Generate the shared secret key using parameters from party A, also generating the public value to send back to party A. Party B calls this.
udResult udCryptoKey_DeriveFromPartyA(const char *pPublicValueA, const char **ppPublicValueB, void *pKey, size_t keyLen);

// Generate the shared secret using the public value from the other party. Party A calls this.
udResult udCryptoKey_DeriveFromPartyB(udCryptoDHMContext *pDHMCtx, const char *pPublicValueB, void *pKey, size_t keyLen);

// Destroy the DHM context
void udCryptoKey_DestroyDHM(udCryptoDHMContext **ppDHMCtx);


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
udResult udCryptoSig_CreateKeyPair(udCryptoSigContext **ppSigCtx, udCryptoSigType type);

// Import public/keypair
udResult udCryptoSig_ImportKeyPair(udCryptoSigContext **pSigCtx, const char *pKeyText);

// Export a public/keypair as JSON
udResult udCryptoSig_ExportKeyPair(udCryptoSigContext *pSigCtx, const char **ppKeyText, bool exportPrivate = false);

// Sign a hash, be sure to udFree the result signature string when finished
udResult udCryptoSig_Sign(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char **ppSignatureString, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Verify a signed hash
udResult udCryptoSig_Verify(udCryptoSigContext *pSigCtx, const void *pHash, size_t hashLen, const char *pSignatureString, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Destroy a signature context
void udCryptoSig_Destroy(udCryptoSigContext **pSigCtx);
