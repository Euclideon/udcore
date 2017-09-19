#define MBEDTLS_AES_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C

#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_MD_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_OID_C
#define MBEDTLS_RIPEMD160_C
#define MBEDTLS_MD5_C
#define MBEDTLS_TIMING_C

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
// Disable some warnings in the MBED_TLS codebase
#pragma warning(disable:4244)
#pragma warning(disable:4245)
#endif
