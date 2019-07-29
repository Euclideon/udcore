#ifndef UDUUID_H
#define UDUUID_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Paul Fox, April 2018
//
// UUID implementation
//

#include <stdint.h>
#include "udPlatform.h"

struct udUUID
{
  enum {
    udUUID_Length = 36
  };

  uint8_t internal_bytes[udUUID_Length+1]; //+1 for \0
};

void udUUID_Clear(udUUID *pUUID);

udResult udUUID_SetFromString(udUUID *pUUID, const char *pStr); //pStr can be freed after this
const char* udUUID_GetAsString(const udUUID &UUID); //Do not free, you do not own this
const char* udUUID_GetAsString(const udUUID *pUUID); //Do not free, you do not own this

// These functions generate UUIDs
udResult udUUID_GenerateFromRandom(udUUID *pUUID); // Fills out pUUID as a version 4 UUID
udResult udUUID_GenerateFromString(udUUID *pUUID, const char *pStr); // Fills out pUUID as a version 5 UUID
udResult udUUID_GenerateFromInt(udUUID *pUUID, int64_t value); // Fills out pUUID as a version 5 UUID

bool udUUID_IsValid(const udUUID &UUID);
bool udUUID_IsValid(const udUUID *pUUID);
bool udUUID_IsValid(const char *pUUIDStr);

uint64_t udUUID_ToNonce(const udUUID *pUUID);

bool operator ==(const udUUID a, const udUUID b);
bool operator !=(const udUUID a, const udUUID b);

#endif //UDUUID_H
