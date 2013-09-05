#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


void udDebugPrintf(const char *format, ...)
{
  va_list args;
  char buffer[300];

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
#ifdef _WIN32
  OutputDebugStringA(buffer);
#else
  printf("%s", buffer);
#endif
}


int AddToStringTable(char *&stringTable, uint32_t &stringTableLength, const char *addString)
{
  uint32_t offset = 0;
  int addStrLen = (int)strlen(addString); // Length of string to be added

  while (offset < stringTableLength)
  {
    int curStrLen = (int)strlen(stringTable + offset);
    if (offset + curStrLen > stringTableLength)
      break; // A catch-all in case something's gone wrong
    if (curStrLen >= addStrLen && strcmp(stringTable + offset + curStrLen - addStrLen, addString) == 0)
      return offset + curStrLen - addStrLen; // Found a match
    else
      offset += curStrLen + 1;
  }
  int newLength = offset + addStrLen + 1;
  char *newCache = (char*)realloc(stringTable, newLength);
  if (!newCache)
    return -1; // A nasty case where memory allocation has failed
  strcpy(newCache + offset, addString);
  stringTable = newCache;
  stringTableLength = offset + addStrLen + 1;
  return offset;
}

