#ifndef UDCRYPTO_H
#define UDCRYPTO_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, December 2014
//
// Module for exposing common cryptographic functionality
//

#include "udPlatform.h"

// **** Utility functions ****

// Securely free the base-64 string
void udCrypto_FreeSecure(const char *&pBase64String);

// Reversibly obscure a base-64 string, each subsequent call reverses obscurity like an XOR operation
// This function takes a const char * but modifies the string, because all use cases involve a const char string
void udCrypto_Obscure(const char *pBase64String);

// Get some cryptographic random data
udResult udCrypto_Random(void *pMem, size_t len);

// **** System Init and cleanup ****

udResult udCrypto_Init();
void udCrypto_Deinit();

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
  udCPM_None,
  udCPM_PKCS7,      // PKCS5 is identical but was only defined for 8-byte block ciphers
};

enum udCryptoChainMode
{
  udCCM_None,   // Sentinal meaning no chaining mode has been set yet
  udCCM_CBC,    // Sequential access, requires IV unique to each call to encrypt
  udCCM_CTR,    // Random access, requires a nonce unique to file
};

struct udCryptoCipherContext;
struct udCryptoIV // All IV's are 16-bytes, however it is possible a future cipher/mode could change this
{
  uint8_t iv[16];
};

// Initialise a cipher
udResult udCryptoCipher_Create(udCryptoCipherContext **ppCtx, udCryptoCiphers cipher, udCryptoPaddingMode padMode, const char *pKeyBase64, udCryptoChainMode chainMode);

// Set the Nonce/counter (for CTR mode)
udResult udCrypto_CreateIVForCTRMode(udCryptoCipherContext *pCtx, udCryptoIV *pIV, uint64_t nonce, uint64_t counter);

// Encrypt/decrypt using current mode/iv/nonce. Optional pOutIV arameter only applicable to CBC mode
udResult udCryptoCipher_Encrypt(udCryptoCipherContext *pCtx, const udCryptoIV *pIV, const void *pPlainText, size_t plainTextLen, void *pCipherText, size_t cipherTextLen, size_t *pPaddedCipherTextLen = nullptr, udCryptoIV *pOutIV = nullptr);
udResult udCryptoCipher_Decrypt(udCryptoCipherContext *pCtx, const udCryptoIV *pIV, const void *pCipherText, size_t cipherTextLen, void *pPlainText, size_t plainTextLen, size_t *pActualPlainTextLen = nullptr, udCryptoIV *pOutIV = nullptr);

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
  udCH_MD5,
  udCH_Count
};

enum udCryptoHashLength
{
  udCHL_SHA1Length = 20,
  udCHL_SHA256Length = 32,
  udCHL_SHA512Length = 64,
  udCHL_MD5Length = 16,

  udCHL_MaxHashLength = udCHL_SHA512Length
};


struct udCryptoHashContext;

// Initialise a hash
udResult udCryptoHash_Create(udCryptoHashContext **ppCtx, udCryptoHashes hash);

// Digest some bytes
udResult udCryptoHash_Digest(udCryptoHashContext *pCtx, const void *pBytes, size_t length);

// Digest some bytes
udResult udCryptoHash_Finalise(udCryptoHashContext *pCtx, const char **ppHashBase64);

// Free resources
udResult udCryptoHash_Destroy(udCryptoHashContext **ppCtx);

// Helper to create/digest/finalise/destroy for a given block (or two) of data
udResult udCryptoHash_Hash(udCryptoHashes hash, const void *pMessage, size_t messageLength, const char **ppHashBase64, const void *pMessage2 = nullptr, size_t message2Length = 0);

// Generate a keyed hash
udResult udCryptoHash_HMAC(udCryptoHashes hash, const char *pKeyBase64, const void *pMessage, size_t messageLength, const char **ppHMACBase64);

// Internal self-test
udResult udCryptoHash_SelfTest(udCryptoHashes hash);


// **** Key derivation/exchange functions, generated keys are encoded into a string ****

struct udCryptoDHMContext;
struct udCryptoECDHContext;

// Generate a key from a plain-text password (max 40 bytes, compatible with CryptDeriveKey KDF)
udResult udCryptoKey_DeriveFromPassword(const char **ppKeyBase64, size_t keyLen, const char *pPassword);

// Generate key data from supplied data (max 64 bytes)
udResult udCryptoKey_DeriveFromData(const char **ppKeyBase64, size_t keyLen, const void *pData, size_t dataLen);

// Generate a random key using system entropy (max 64 bytes)
udResult udCryptoKey_DeriveFromRandom(const char **ppKeyBase64, size_t keyLen);

// Create a Diffie-Hellman-Merkle key exchange context, generating a string of public values for the other party. Party A calls this.
udResult udCryptoKey_CreateDHM(udCryptoDHMContext **ppDHMCtx, const char **ppPublicValueA, size_t keyLen);

// Generate the shared secret key using parameters from party A, also generating the public value to send back to party A. Party B calls this.
udResult udCryptoKey_DeriveFromPartyA(const char *pPublicValueA, const char **ppPublicValueB, const char **ppKeyBase64);

// Generate the shared secret using the public value from the other party. Party A calls this.
udResult udCryptoKey_DeriveFromPartyB(udCryptoDHMContext *pDHMCtx, const char *pPublicValueB, const char **ppKeyBase64);

// Destroy the DHM context
void udCryptoKey_DestroyDHM(udCryptoDHMContext **ppDHMCtx);

// **** ECDH Key exchange functions, generated keys are encoded into a string ****

// Create a ECDH context
udResult udCryptoKeyECDH_CreateContextPartyA(udCryptoECDHContext **ppECDHCtx, const char **ppPublicValueA);

// Get PartB from PartA (and shared key)
udResult udCryptoKeyECDH_DeriveFromPartyA(const char *pPublicValueA, const char **ppPublicValueB, const char **ppKey);

// Get Shared Key from PartB
udResult udCryptoKeyECDH_DeriveFromPartyB(udCryptoECDHContext *ppECDHCtx, const char *pPublicValueB, const char **ppKey);

// Tidy Up for PartA
void udCryptoKeyECDH_Destroy(udCryptoECDHContext **ppECDHCtx);

// **** Digital signature functions ****

struct udCryptoSigContext;

enum udCryptoSigType
{
  udCST_RSA1024 = 1024,
  udCST_RSA2048 = 2048,
  udCST_RSA4096 = 4096,
  udCST_ECPBP384 = 384
};

enum udCryptoSigPadScheme
{
  udCSPS_Deterministic   // A deterministic signature that doesn't require entropy. For RSA PKCS #1.5
};

// Generate a random key-pair
udResult udCryptoSig_CreateKeyPair(udCryptoSigContext **ppSigCtx, udCryptoSigType type);

// Import public/keypair
udResult udCryptoSig_ImportKeyPair(udCryptoSigContext **pSigCtx, const char *pKeyText);
udResult udCryptoSig_ImportMSBlob(udCryptoSigContext **ppSigCtx, void *pBlob, size_t blobLen);

// Export a public/keypair as JSON
udResult udCryptoSig_ExportKeyPair(udCryptoSigContext *pSigCtx, const char **ppKeyText, bool exportPrivate = false);

// Sign a hash, be sure to udFree the result signature string when finished
udResult udCryptoSig_Sign(udCryptoSigContext *pSigCtx, const char *pHashBase64, const char **ppSignatureBase64, udCryptoHashes hashMethod, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Verify a signed hash
udResult udCryptoSig_Verify(udCryptoSigContext *pSigCtx, const char *pHashBase64, const char *pSignatureBase64, udCryptoHashes hashMethod, udCryptoSigPadScheme pad = udCSPS_Deterministic);

// Destroy a signature context
void udCryptoSig_Destroy(udCryptoSigContext **pSigCtx);

#endif // UDCRYPTO_H
