#define NOMINMAX
#include "udUUID.h"

#include "udStringUtil.h"
#include "udCrypto.h"
#include "udPlatformUtil.h"
#include <algorithm>

// ***************************************************************************
// Author: Paul Fox, April 2018
void udUUID_Clear(udUUID *pUUID)
{
  memset(pUUID, 0, sizeof(udUUID));
}

// ***************************************************************************
// Author: Paul Fox, April 2018
udResult udUUID_SetFromString(udUUID *pUUID, const char *pStr)
{
  if (!udUUID_IsValid(pStr))
    return udR_InvalidParameter;

  memset(pUUID, 0, sizeof(udUUID));
  memcpy(pUUID->internal_bytes, pStr, std::min((size_t)udUUID::udUUID_Length, udStrlen(pStr)));

  return udR_Success;
}

// ***************************************************************************
// Author: Paul Fox, March 2019
const char* udUUID_GetAsString(const udUUID &UUID)
{
  return (const char*)UUID.internal_bytes;
}

// ***************************************************************************
// Author: Paul Fox, April 2018
const char* udUUID_GetAsString(const udUUID *pUUID)
{
  if (pUUID == nullptr)
    return nullptr;

  return (const char*)pUUID->internal_bytes;
}

// ***************************************************************************
// Author: Paul Fox, April 2018
bool udUUID_IsValid(const char *pUUIDStr)
{
  if (pUUIDStr == nullptr)
    return false;

  for (int i = 0; i < udUUID::udUUID_Length; ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (pUUIDStr[i] == '-')
        continue;
    }
    else if ((pUUIDStr[i] >= '0' && pUUIDStr[i] <= '9') || (pUUIDStr[i] >= 'a' && pUUIDStr[i] <= 'f') || (pUUIDStr[i] >= 'A' && pUUIDStr[i] <= 'F'))
    {
      continue;
    }

    return false;
  }

  return true;
}

// ***************************************************************************
// Author: Paul Fox, April 2019
udResult udUUID_GenerateFromRandom(udUUID *pUUID)
{
  udResult result = udR_Failure;
  uint8_t mem[16];
  int index = 0;

  UD_ERROR_NULL(pUUID, udR_InvalidParameter);
  UD_ERROR_CHECK(udCrypto_Init());
  UD_ERROR_CHECK(udCrypto_Random(mem, udLengthOf(mem)));
  udCrypto_Deinit();

  for (int i = 0; i < udUUID::udUUID_Length; ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23) // Hyphens
    {
      pUUID->internal_bytes[i] = '-';
    }
    else
    {
      uint8_t bytes = mem[index];

      if (i == 19)
        bytes = (bytes & 0x3F) | 0x80; //Variant bits

      udStrUtoa((char*)pUUID->internal_bytes + i, udUUID::udUUID_Length - i + 1, bytes, 16, 2);

      ++index;
      ++i; //This block writes 2 bytes
    }
  }

  pUUID->internal_bytes[14] = '4'; // Version number

  result = udR_Success;

epilogue:
  return result;
}

// ***************************************************************************
// Author: Paul Fox, April 2019
udResult udUUID_GenerateFromString(udUUID *pUUID, const char *pStr)
{
  udCryptoHashContext *pHashCtx = nullptr;
  udResult result = udR_Failure;
  const char *pOutB64 = nullptr;
  uint8_t mem[20]; //SHA 1 is 160bits (20bytes)
  int index = 0;

  const uint8_t base[] = { 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

  UD_ERROR_NULL(pUUID, udR_InvalidParameter);

  UD_ERROR_CHECK(udCryptoHash_Create(&pHashCtx, udCH_SHA1));
  UD_ERROR_CHECK(udCryptoHash_Digest(pHashCtx, base, udLengthOf(base)));
  UD_ERROR_CHECK(udCryptoHash_Digest(pHashCtx, pStr, udStrlen(pStr)));
  UD_ERROR_CHECK(udCryptoHash_Finalise(pHashCtx, &pOutB64)); //pOut is already hex

  UD_ERROR_CHECK(udBase64Decode(pOutB64, udStrlen(pOutB64), mem, udLengthOf(mem)));

  for (int i = 0; i < udUUID::udUUID_Length; ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23) // Hyphens
    {
      pUUID->internal_bytes[i] = '-';
    }
    else
    {
      uint8_t bytes = mem[index];

      if (i == 19)
        bytes = (bytes & 0x3F) | 0x80; //Variant bits

      udStrUtoa((char*)pUUID->internal_bytes + i, udUUID::udUUID_Length - i + 1, bytes, 16, 2);

      ++index;
      ++i; //This block writes 2 bytes
    }
  }

  pUUID->internal_bytes[14] = '5'; // Version number

  result = udR_Success;

epilogue:
  udFree(pOutB64);
  udCryptoHash_Destroy(&pHashCtx);

  return result;
}

// ***************************************************************************
// Author: Paul Fox, April 2019
udResult udUUID_GenerateFromInt(udUUID *pUUID, int64_t value)
{
  const char *pStr = nullptr;
  udResult result = udR_Failure;

  UD_ERROR_CHECK(udSprintf(&pStr, "%" PRId64, value));
  UD_ERROR_CHECK(udUUID_GenerateFromString(pUUID, pStr));

  result = udR_Success;

epilogue:
  udFree(pStr);
  return result;
}

// ***************************************************************************
// Author: Paul Fox, March 2019
bool udUUID_IsValid(const udUUID &UUID)
{
  return udUUID_IsValid((const char*)UUID.internal_bytes);
}

// ***************************************************************************
// Author: Paul Fox, April 2018
bool udUUID_IsValid(const udUUID *pUUID)
{
  return udUUID_IsValid((const char*)pUUID->internal_bytes);
}

// ***************************************************************************
// Author: Paul Fox, May 2018
uint64_t udUUID_ToNonce(const udUUID *pUUID)
{
  uint64_t accumA = udStrAtou64((const char *)&pUUID->internal_bytes[0], nullptr, 16);
  uint64_t accumB = udStrAtou64((const char *)&pUUID->internal_bytes[9], nullptr, 16);
  uint64_t accumD = udStrAtou64((const char *)&pUUID->internal_bytes[19], nullptr, 16);
  uint64_t accumE = udStrAtou64((const char *)&pUUID->internal_bytes[24], nullptr, 16);

  uint64_t partA = (accumA << 32) | (accumB << 16);
  uint64_t partB = (accumD << 48) | accumE;

  return (partA ^ partB);
}

// ***************************************************************************
// Author: Paul Fox, April 2018
bool operator ==(const udUUID a, const udUUID b)
{
  return (udStrncmpi((const char*)a.internal_bytes, (const char*)b.internal_bytes, udUUID::udUUID_Length) == 0);
}

// ***************************************************************************
// Author: Paul Fox, May 2018
bool operator !=(const udUUID a, const udUUID b)
{
  return !(a == b);
}
