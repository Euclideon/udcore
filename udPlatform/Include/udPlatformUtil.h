#ifndef UDPLATFORM_UTIL_H
#define UDPLATFORM_UTIL_H

#include "udPlatform.h"

void udDebugPrintf(const char *format, ...);

template<typename T>
bool udSizeArray(T *&ptr, uint32_t &currentLength, uint32_t requiredLength, int32_t allocationMultiples = 0)
{
  if (requiredLength > currentLength || allocationMultiples < 0)
  {
    uint32_t newLength;
    if (allocationMultiples > 1)
      newLength = ((requiredLength + allocationMultiples - 1) / allocationMultiples) * allocationMultiples;
    else
      newLength = requiredLength;
    void *resized = realloc(ptr, newLength * sizeof(T));
    if (!resized)
      return false;

    ptr = (T*)resized;
    currentLength = newLength;
  }
  return true;
}

/// \brief Count the number of bits set in a 8 bit number
inline uint32_t udCountBits(uint8_t a_number)
{
  static const uint8_t bits[256] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8 };
  return bits[a_number];
}


// Add a string to a dynamic table of unique strings. Initialise stringTable to NULL and stringTableLength to 0, and table will be reallocated as necessary
int AddToStringTable(char *&stringTable, uint32_t &stringTableLength, const char *addString);

#endif // UDPLATFORM_UTIL_H
