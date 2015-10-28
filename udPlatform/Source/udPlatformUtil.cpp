#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udFile.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#if UDPLATFORM_WINDOWS
#include <io.h>
#include <mmsystem.h>
#else
#include "dirent.h"
#include <time.h>
#include <sys/time.h>
static const uint64_t nsec_per_sec = 1000000000; // 1 billion nanoseconds in one second
static const uint64_t nsec_per_msec = 1000000;   // 1 million nanoseconds in one millisecond
//static const uint64_t usec_per_msec = 1000;      // 1 thousand microseconds in one millisecond
#endif

static char s_udStrEmptyString[] = "";

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udGetTimeMs()
{
#if UDPLATFORM_WINDOWS
  return timeGetTime();
#else
  //struct timeval now;
  //gettimeofday(&now, NULL);
  //return (uint32_t)(now.tv_usec/usec_per_msec);

  // Unfortunately the above code is unreliable
  // TODO: Check whether this code gives consistent timing values regardless of thread
  struct timespec ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts1);
  return (ts1.tv_sec * 1000) + (ts1.tv_nsec / nsec_per_msec);
#endif
}

// *********************************************************************
// Author: Dave Pevreal, June 2014
uint64_t udPerfCounterStart()
{
#if UDPLATFORM_WINDOWS
  LARGE_INTEGER p;
  QueryPerformanceCounter(&p);
  return p.QuadPart;
#else
  uint64_t nsec_count;
  struct timespec ts1;
  clock_gettime(CLOCK_MONOTONIC, &ts1);
  nsec_count = ts1.tv_nsec + ts1.tv_sec * nsec_per_sec;
  return nsec_count;
#endif
}


// *********************************************************************
// Author: Dave Pevreal, June 2014
float udPerfCounterMilliseconds(uint64_t startValue, uint64_t end)
{
#if UDPLATFORM_WINDOWS
  LARGE_INTEGER p, f;
  if (end)
    p.QuadPart = end;
  else
    QueryPerformanceCounter(&p);
  QueryPerformanceFrequency(&f);

  // TODO: Come back and tidy this up to be integer

  double delta = (double)(p.QuadPart - startValue);

  double ms = (delta) ? (1000.0 / (f.QuadPart / delta)) : 0.0;
  return (float)ms;
#else
  if (!end)
    end = udPerfCounterStart();
  double ms = (end - startValue) / (nsec_per_sec / 1000.0);
  return (float)ms;
#endif
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrcpy(char *dest, size_t destLen, const char *src)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = s_udStrEmptyString;
  size_t srcChars = strlen(src);
  if ((srcChars + 1) > destLen)
  {
    *dest = 0;
    return 0;
  }
  memcpy(dest, src, srcChars + 1);
  return srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrncpy(char *dest, size_t destLen, const char *src, size_t maxChars)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = s_udStrEmptyString;
  size_t srcChars = strlen(src);
  if (srcChars > maxChars)
    srcChars = maxChars;
  if ((srcChars + 1) > destLen)
  {
    *dest = 0;
    return 0;
  }
  memcpy(dest, src, srcChars); // Use crt strcpy as it's likely to be very fast
  dest[srcChars] = 0; // Nul terminate
  return srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrcat(char *dest, size_t destLen, const char *src)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = s_udStrEmptyString;
  size_t destChars = strlen(dest); // Note: Not including terminator
  size_t srcChars = strlen(src);
  if ((destChars + srcChars + 1) > destLen)
    return 0;
  strcat(dest, src);
  return destChars + srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmp(const char *s1, const char *s2)
{
  if (!s1) s1 = s_udStrEmptyString;
  if (!s2) s2 = s_udStrEmptyString;

  int result;
  do
  {
    result = *s1 - *s2;
  } while (!result && *s1++ && *s2++);

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmpi(const char *s1, const char *s2)
{
  if (!s1) s1 = s_udStrEmptyString;
  if (!s2) s2 = s_udStrEmptyString;

  int result;
  do
  {
    result = tolower(*s1) - tolower(*s2);
  } while (!result && *s1++ && *s2++);

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrlen(const char *str)
{
  if (!str) str = s_udStrEmptyString;

  size_t len = 0;
  while (*str++)
    ++len;

  return len;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udStrBeginsWith(const char *s, const char *prefix)
{
  if (!s) s = s_udStrEmptyString;
  if (!prefix) prefix = s_udStrEmptyString;

  while (*prefix)
  {
    if (*s++ != *prefix++)
      return false;
  }
  return true;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
char *udStrdup(const char *s, size_t additionalChars)
{
  if (!s) s = s_udStrEmptyString;

  size_t len = udStrlen(s) + 1;
  char *dup = udAllocType(char, len + additionalChars, udAF_None);
  memcpy(dup, s, len);
  return dup;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
const char *udStrchr(const char *s, const char *pCharList, size_t *pIndex)
{
  if (!s) s = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t index;
  size_t listLen = udStrlen(pCharList);

  for (index = 0; s[index]; ++index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (s[index] == pCharList[i])
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        return s + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = index;
  return nullptr;
}


// *********************************************************************
// Author: Samuel Surtees, May 2015
const char *udStrrchr(const char *s, const char *pCharList, size_t *pIndex)
{
  if (!s) s = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t sLen = udStrlen(s);
  size_t listLen = udStrlen(pCharList);

  for (ptrdiff_t index = sLen - 1; index >= 0; --index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (s[index] == pCharList[i])
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        return s + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = sLen;
  return nullptr;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
const char *udStrstr(const char *s, size_t sLen, const char *pSubString, size_t *pIndex)
{
  if (!s) s = s_udStrEmptyString;
  if (!pSubString) pSubString = s_udStrEmptyString;
  size_t i;
  size_t subStringIndex = 0;

  for (i = 0; s[i] && (!sLen || i < sLen); ++i)
  {
    if (s[i] == pSubString[subStringIndex])
    {
      if (pSubString[++subStringIndex] == 0)
      {
        // Substring found, move index to beginning of instance
        i = i + 1 - subStringIndex;
        if (pIndex)
          *pIndex = i;
        return s + i;
      }
    }
    else
    {
      subStringIndex = 0;
    }
  }

  // Substring not found
  if (pIndex)
    *pIndex = i;
  return nullptr;
}

#if UDPLATFORM_WINDOWS
// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const char *pString)
{
  size_t len = udStrlen(pString) + 1;
  m_pUTF8 = const_cast<char*>(pString);
  m_pWide = udAllocType(wchar_t, len, udAF_None);
  m_pAllocation = m_pWide;

  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString, -1, m_pWide, (int)len) == 0)
    *m_pWide = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const wchar_t *pString)
{
  size_t len = wcslen(pString) + 1;
  size_t allocSize = len * 4;
  m_pUTF8 = udAllocType(char, allocSize, udAF_None);
  m_pWide = const_cast<wchar_t*>(pString);
  m_pAllocation = m_pUTF8;

  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pString, -1, m_pUTF8, (int)allocSize, nullptr, nullptr) == 0)
    *m_pUTF8 = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::~udOSString()
{
  udFree(m_pAllocation);
}
#endif // UDPLATFORM_WINDOWS

// *********************************************************************
// Author: Dave Pevreal, August 2014
int udTokenSplit(char *pLine, const char *pDelimiters, char *pTokenArray[], int maxTokens)
{
  if (pLine == nullptr)
    return 0;

  int tokenCount = 0;
  while (*pLine && tokenCount < maxTokens)
  {
    size_t delimiterIndex;
    pTokenArray[tokenCount++] = pLine;                  // Assign token
    if (udStrchr(pLine, pDelimiters, &delimiterIndex))  // Get the index of the delimiter
    {
      pLine[delimiterIndex] = 0;                        // Null terminate the token
      pLine += delimiterIndex + 1;                      // Move pLine to 1st char after delimiter (possibly another delimiter)
    }
    else
    {
      break;
    }
  }

  // Assign remaining tokens to the end-of-line, this way caller's can
  // depend on all tokens in the array being initialised
  for (int i = tokenCount; i < maxTokens; ++i)
    pTokenArray[i] = pLine;

  return tokenCount;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int32_t udStrAtoi(const char *s, int *pCharCount, int radix)
{
  if (!s) s = s_udStrEmptyString;
  int32_t result = 0;
  int charCount = 0;
  bool negate = false;

  while (s[charCount] == ' ' || s[charCount] == '\t')
    ++charCount;
  if (s[charCount] == '+')
    ++charCount;
  if (s[charCount] == '-')
  {
    negate = true;
    ++charCount;
  }
  for (; s[charCount]; ++charCount)
  {
    int nextValue = radix; // A sentinal to force end of processing
    if (s[charCount] >= '0' && s[charCount] <= '9')
      nextValue = s[charCount] - '0';
    else if (s[charCount] >= 'a' && s[charCount] <= 'f')
      nextValue = 10 + (s[charCount] - 'a');
    else if (s[charCount] >= 'A' && s[charCount] <= 'F')
      nextValue = 10 + (s[charCount] - 'A');

    if (nextValue >= radix)
      break;

    result = result * radix + nextValue;
  };
  if (negate)
    result = -result;
  if (pCharCount)
    *pCharCount = charCount;
  return result;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
int64_t udStrAtoi64(const char *s, int *pCharCount, int radix)
{
  if (!s) s = s_udStrEmptyString;
  int64_t result = 0;
  int charCount = 0;
  bool negate = false;

  while (s[charCount] == ' ' || s[charCount] == '\t')
    ++charCount;
  if (s[charCount] == '+')
    ++charCount;
  if (s[charCount] == '-')
  {
    negate = true;
    ++charCount;
  }
  for (; s[charCount]; ++charCount)
  {
    int nextValue = radix; // A sentinal to force end of processing
    if (s[charCount] >= '0' && s[charCount] <= '9')
      nextValue = s[charCount] - '0';
    else if (s[charCount] >= 'a' && s[charCount] <= 'f')
      nextValue = 10 + (s[charCount] - 'a');
    else if (s[charCount] >= 'A' && s[charCount] <= 'F')
      nextValue = 10 + (s[charCount] - 'A');

    if (nextValue >= radix)
      break;

    result = result * radix + nextValue;
  };
  if (negate)
    result = -result;
  if (pCharCount)
    *pCharCount = charCount;
  return result;
}


// *********************************************************************
// Author: Dave Pevreal, August 2014
float udStrAtof(const char *s, int *pCharCount)
{
  if (!s) s = s_udStrEmptyString;
  int preCharCount = 0;
  int charCount = 0;
  int secondaryCharCount = 0;

  float negate = 1.0f;
  while (s[preCharCount] == ' ' || s[preCharCount] == '\t')
    ++preCharCount;

  // Process negation separately
  if (s[preCharCount] == '-')
  {
    negate = -1.0f;
    ++preCharCount;
  }
  s += preCharCount;

  float result = (float)udStrAtoi(s, &charCount);
  if (s[charCount] == '.')
  {
    ++charCount;
    int32_t fraction = udStrAtoi(s + charCount, &secondaryCharCount);
    if (result >= 0.f)
      result += fraction / powf(10.f, (float)secondaryCharCount);
    else
      result -= fraction / powf(10.f, (float)secondaryCharCount);
  }
  if (s[charCount] == 'e' || s[charCount] == 'E')
  {
    ++charCount;
    float e = (float)udStrAtoi(s + charCount, &secondaryCharCount);
    result *= powf(10, e);
  }
  if (pCharCount)
    *pCharCount = preCharCount + charCount + secondaryCharCount;
  return result * negate;
}


// *********************************************************************
// Author: Dave Pevreal, August 2014
double udStrAtof64(const char *s, int *pCharCount)
{
  if (!s) s = s_udStrEmptyString;
  int preCharCount = 0;
  int charCount = 0;
  int secondaryCharCount = 0;

  double negate = 1.0f;
  while (s[preCharCount] == ' ' || s[preCharCount] == '\t')
    ++preCharCount;

  // Process negation separately
  if (s[preCharCount] == '-')
  {
    negate = -1.0f;
    ++preCharCount;
  }
  s += preCharCount;

  double result = (double)udStrAtoi64(s, &charCount);
  if (s[charCount] == '.')
  {
    ++charCount;
    int32_t fraction = udStrAtoi(s + charCount, &secondaryCharCount);
    if (result >= 0.f)
      result += fraction / pow(10.0, secondaryCharCount);
    else
      result -= fraction / pow(10.0, secondaryCharCount);
  }
  if (s[charCount] == 'e' || s[charCount] == 'E')
  {
    ++charCount;
    double e = (double)udStrAtoi64(s + charCount, &secondaryCharCount);
    result *= pow(10, e);
  }
  if (pCharCount)
    *pCharCount = preCharCount + charCount + secondaryCharCount;
  return result * negate;
}


// *********************************************************************
// Author: Dave Pevreal, December 2014
size_t udDecodeBase64(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength)
{
  static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  uint32_t accum = 0; // Accumulator for incoming data (read 6 bits at a time)
  int accumBits = 0;
  size_t outputIndex = 0;
  for (size_t inputIndex = 0;inputIndex < length; )
  {
    char *c = strchr(b64, pString[inputIndex++]);
    if (c)
    {
      accum |= uint32_t(c - b64) << (16 - accumBits - 6); // Store in accumulator, starting at high byte
      if ((accumBits += 6) >= 8)
      {
        if (outputIndex < outputLength)
          pOutput[outputIndex] = uint8_t(accum >> 8);
        ++outputIndex;
        accum <<= 8;
        accumBits -= 8;
      }
    }
  }
  return outputIndex;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString)
{
  uint32_t offset = 0;
  int addStrLen = (int)udStrlen(addString); // Length of string to be added

  while (offset < *pStringTableLength)
  {
    int curStrLen = (int)udStrlen(pStringTable + offset);
    if (offset + curStrLen > *pStringTableLength)
      break; // A catch-all in case something's gone wrong
    if (curStrLen >= addStrLen && udStrcmp(pStringTable + offset + curStrLen - addStrLen, addString) == 0)
      return offset + curStrLen - addStrLen; // Found a match
    else
      offset += curStrLen + 1;
  }
  int newLength = offset + addStrLen + 1;
  char *newCache = (char*)udRealloc(pStringTable, newLength);
  if (!newCache)
    return -1; // A nasty case where memory allocation has failed
  strcpy(newCache + offset, addString);
  pStringTable = newCache;
  *pStringTableLength = offset + addStrLen + 1;
  return offset;
}


// *********************************************************************
int udGetHardwareThreadCount()
{
#if UDPLATFORM_WINDOWS
  DWORD_PTR processMask;
  DWORD_PTR systemMask;

  if (::GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
  {
    int hardwareThreadCount = 0;
    while (processMask)
    {
      ++hardwareThreadCount;
      processMask &= processMask - 1; // Clear LSB
    }
    return hardwareThreadCount;
  }
#elif UDPLATFORM_LINUX
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif

  return 1;
}

// *********************************************************************
bool udFilename::SetFromFullPath(const char *fullPath)
{
  *m_path = 0;
  m_filenameIndex = 0;
  m_extensionIndex = 0;
  if (fullPath)
  {
    if (!udStrcpy(m_path, sizeof(m_path), fullPath))
      return false;
    CalculateIndices();
  }
  return true;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFolder(const char *folder)
{
  char newPath[MaxPath];
  size_t i = udStrcpy(newPath, sizeof(m_path), folder);
  if (!i)
    return false;

  // If the m_path doesn't have a trailing seperator, look for one so we can
  // append one already being used. That is c:\m_path\ or c:/m_path/
  if (i > 2 && newPath[i-2] != '/' && newPath[i-2] != '\\' && newPath[i-2] != ':')
  {
    for (--i; i > 0; --i)
    {
      if (newPath[i-1] == '\\' || newPath[i-1] == ':')
      {
        udStrcat(newPath, sizeof(newPath), "\\");
        break;
      }
      if (newPath[i-1] == '/')
      {
        udStrcat(newPath, sizeof(newPath), "/");
        break;
      }
    }
    // Nothing was found so add a /. TODO: Get correct separator from system
    if (i == 0)
      udStrcat(newPath, sizeof(newPath), "/");
  }

  if (!udStrcat(newPath, sizeof(newPath), GetFilenameWithExt()))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameNoExt(const char *filenameOnlyComponent)
{
  char newPath[MaxPath];

  ExtractFolder(newPath, sizeof(newPath));
  if (!udStrcat(newPath, sizeof(newPath), filenameOnlyComponent))
    return false;
  if (!udStrcat(newPath, sizeof(newPath), GetExt()))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameWithExt(const char *filenameWithExtension)
{
  char newPath[MaxPath];

  ExtractFolder(newPath, sizeof(newPath));
  if (!udStrcat(newPath, sizeof(newPath), filenameWithExtension))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetExtension(const char *extComponent)
{
  char newPath[MaxPath];

  if (!udStrcpy(newPath, sizeof(newPath), m_path))
    return false;
  newPath[m_extensionIndex] = 0; // Truncate the extension
  if (!udStrcat(newPath, sizeof(newPath), extComponent))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFolder(char *folder, int folderLen)
{
  int folderChars = m_filenameIndex;
  if (folder != NULL)
    udStrncpy(folder, folderLen, m_path, folderChars);
  return folderChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFilenameOnly(char *filename, int filenameLen)
{
  int filenameChars = m_extensionIndex - m_filenameIndex;
  if (filename != NULL)
  {
    udStrncpy(filename, filenameLen, m_path + m_filenameIndex, filenameChars);
  }
  return filenameChars + 1;
}

// ---------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
void udFilename::CalculateIndices()
{
  int len = (int)strlen(m_path);
  // Set filename and extension indices to null terminator as a sentinal
  m_filenameIndex = -1;
  m_extensionIndex = len; // If no extension, point extension to nul terminator

  for (--len; len > 0 && (m_filenameIndex == -1 || m_extensionIndex == -1); --len)
  {
    if (m_path[m_extensionIndex] == 0 && m_path[len] == '.') // Last period
      m_extensionIndex = len;
    if (m_filenameIndex == -1 && (m_path[len] == '/' || m_path[len] == '\\' || m_path[len] == ':'))
      m_filenameIndex = len + 1;
  }
  // If no path indicators were found the filename is the beginning of the path
  if (m_filenameIndex == -1)
    m_filenameIndex = 0;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
void udFilename::Debug()
{
  char folder[260];
  char name[50];

  ExtractFolder(folder, sizeof(folder));
  ExtractFilenameOnly(name, sizeof(name));
  udDebugPrintf("folder<%s> name<%s> ext<%s> filename<%s> -> %s\n", folder, name, GetExt(), GetFilenameWithExt(), m_path);
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
udResult udURL::SetURL(const char *pURLText)
{
  udFree(m_pURLText);
  m_pScheme = s_udStrEmptyString;
  m_pDomain = s_udStrEmptyString;
  m_pPath = s_udStrEmptyString;

  if (pURLText)
  {
    // Take a copy of the entire string
    char *p = m_pURLText = udStrdup(pURLText, 1); // Add an extra char on the allocation to nul terminate the domain
    if (!pURLText)
      return udR_MemoryAllocationFailure;

    size_t i;

    // Isolate the scheme
    udStrchr(p, ":/", &i); // Find the colon that breaks the scheme, but don't search past a slash

    if (p[i] == ':') // Test in case of malformed url (potentially allowing a default scheme such as 'http'
    {
      m_pScheme = p;
      p[i] = 0; // null terminate the scheme
      p = p + i + 1;
      // Skip over the // at start of domain (if present)
      if (p[0] == '/' && p[1] == '/')
        p += 2;
    }

    // Isolate the domain - this is slightly painful because ipv6 has colons
    m_pDomain = p;
    udStrchr(p, p[0] == '[' ? "/]" : "/:", &i); // Find the character that breaks the domain, but don't search past a slash
    if (p[0] == '[' && p[i] == ']') ++i; // Skip over closing bracket of ipv6 address
    if (p[i] == ':')
    {
      // A colon is present, so decode the port number
      int portChars;
      m_port = udStrAtoi(&p[i+1], &portChars);
      p[i] = 0; // null terminate the domain
      i += portChars + 1;
    }
    else
    {
      // Otherwise let's assume port 80. TODO: Default port should be based on the scheme
      m_port = 80;
      // Because no colon character exists to terminate the domain, move it forward by 1 byte
      if (p[i] != 0)
      {
        memmove(p + i + 1, p + i, udStrlen(p + i) + 1); // Move the string one to the right to retain the separator (note: 1 byte was added to allocation when udStrdup called)
        p[i++] = 0; // null terminate the domain
      }
    }
    p += i;

    // Finally, the path is the last component (for now, unless the class is extended to support splitting the query string too)
    m_pPath = p;
  }

  return udR_Success; // TODO: Perhaps return an error if the url isn't formed properly
}


#pragma pack(push)
#pragma pack(2)
struct udBMPHeader
{
    uint16_t  bfType;            // must be 'BM'
    uint32_t  bfSize;            // size of the whole .bmp file
    uint16_t  bfReserved1;       // must be 0
    uint16_t  bfReserved2;       // must be 0
    uint32_t  bfOffBits;

    uint32_t  biSize;            // size of the structure
    int32_t   biWidth;           // image width
    int32_t   biHeight;          // image height
    uint16_t  biPlanes;          // bitplanes
    uint16_t  biBitCount;        // resolution
    uint32_t  biCompression;     // compression
    uint32_t  biSizeImage;       // size of the image
    int32_t   biXPelsPerMeter;   // pixels per meter X
    int32_t   biYPelsPerMeter;   // pixels per meter Y
    uint32_t  biClrUsed;         // colors used
    uint32_t  biClrImportant;    // important colors
};
#pragma pack(pop)

// ***************************************************************************************
// Author: Dave Pevreal, August 2014
udResult udSaveBMP(const char *pFilename, int width, int height, uint32_t *pColorData, int pitchInBytes)
{
  udBMPHeader header = { 0 };
  if (!pitchInBytes)
    pitchInBytes = width * 4;

  udFile *pFile = nullptr;
  int paddedLineSize = (width * 3 + 3) & ~3;
  udResult result = udR_MemoryAllocationFailure;
  uint8_t *pLine = udAllocType(uint8_t, paddedLineSize, udAF_Zero);
  if (!pLine)
    goto error;

  header.bfType = 0x4d42;       // 0x4d42 = 'BM'
  header.bfReserved1 = 0;
  header.bfReserved2 = 0;
  header.bfSize = sizeof(header) + paddedLineSize * height;
  header.bfOffBits = 0x36;
  header.biSize = 0x28; // sizeof(BITMAPINFOHEADER);
  header.biWidth = width;
  header.biHeight = height;
  header.biPlanes = 1;
  header.biBitCount = 24;
  header.biCompression = 0; /*BI_RGB*/
  header.biSizeImage = 0;
  header.biXPelsPerMeter = 0x0ec4;
  header.biYPelsPerMeter = 0x0ec4;
  header.biClrUsed = 0;
  header.biClrImportant = 0;

  result = udFile_Open(&pFile, pFilename, udFOF_Write | udFOF_Create);
  if (result != udR_Success)
    goto error;

  result = udFile_SeekWrite(pFile, &header, sizeof(header));
  if (result != udR_Success)
    goto error;

  for (int y = height - 1; y >= 0 ; --y)
  {
    for (int x = 0; x < width; ++x)
      memcpy(&pLine[x*3], &((uint8_t*)pColorData)[y * pitchInBytes + x * 4], 3);

    result = udFile_SeekWrite(pFile, pLine, paddedLineSize);
    if (result != udR_Success)
      goto error;
  }

error:
  udFree(pLine);
  udFile_Close(&pFile);
  return result;
}

// ***************************************************************************************
// Author: Dave Pevreal, August 2014
udResult udLoadBMP(const char *pFilename, int *pWidth, int *pHeight, uint32_t **ppColorData)
{
  if (!pFilename || !pWidth || !pHeight || !ppColorData)
    return udR_InvalidParameter_;
  udBMPHeader header = { 0 };
  udFile *pFile = nullptr;
  uint8_t *pColors = nullptr;
  uint8_t *pLine = nullptr;
  int paddedLineSize;
  udResult result;

  result = udFile_Open(&pFile, pFilename, udFOF_Read);
  if (result != udR_Success)
    goto epilogue;
  result = udFile_SeekRead(pFile, &header, sizeof(header));
  if (result != udR_Success)
    goto epilogue;

  *pWidth = header.biWidth;
  *pHeight = header.biHeight;
  paddedLineSize = (*pWidth * 3 + 3) & ~3;
  pColors = udAllocType(uint8_t, *pWidth * *pHeight * 4, udAF_None);
  pLine = udAllocType(uint8_t, paddedLineSize, udAF_None);
  if (!pColors || !pLine)
  {
    result = udR_MemoryAllocationFailure;
    goto epilogue;
  }

  for (int y = *pHeight - 1; y >= 0 ; --y)
  {
    result = udFile_SeekRead(pFile, pLine, paddedLineSize);
    if (result != udR_Success)
      goto epilogue;

    uint8_t *p = pColors + y * *pWidth * 4;
    for (int x = 0; x < *pWidth; ++x)
    {
      *p++ = pLine[x * 3 + 0];
      *p++ = pLine[x * 3 + 1];
      *p++ = pLine[x * 3 + 2];
      *p++ = 0xff;
    }
  }

  *ppColorData = (uint32_t*)pColors;
  pColors = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFile_Close(&pFile);
  udFree(pLine);
  udFree(pColors);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, October 2015
const char *udCommaInt(int64_t n)
{
  static char buffers[32][32]; // 32 cycling buffers of 32 characters, which is enough for biggest 64-bit integer
  static int bufferIndex = 0;
  char *pBuf = buffers[bufferIndex];
  bufferIndex = (bufferIndex + 1) % 32;
  uint64_t v = (uint64_t)n;

  int i = 0;
  if (n < 0)
  {
    pBuf[i++] = '-';
    v = (uint64_t)-n;
  }

  // Get the column variable to be the first column containing anything
  uint64_t col = 1;
  int digitCount = 1;
  while (v >= col * 10)
  {
    col = col * 10;
    ++digitCount;
  }
  do
  {
    uint64_t c = v / col;
    pBuf[i++] = (char)('0' + c);
    if ((--digitCount % 3) == 0 && digitCount)
      pBuf[i++] = ',';
    v -= c * col;
    col = col / 10;
  } while (digitCount);
  pBuf[i++] = 0;

  return pBuf;
}


struct udFindDirData : public udFindDir
{
#if UDPLATFORM_WINDOWS
  HANDLE hFind;
  WIN32_FIND_DATAW findFileData;
  char utf8Filename[2048];

  void SetMembers()
  {
    // Take a copy of the filename after translation from wide-char to utf8
    udStrcpy(utf8Filename, sizeof(utf8Filename), udOSString(findFileData.cFileName));
    pFilename = utf8Filename;
    isDirectory = !!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
#elif UDPLATFORM_LINUX
  DIR *pDir;
  struct dirent *pDirent;

  void SetMembers()
  {
    pFilename = pDirent->d_name;
    isDirectory = !!(pDirent->d_type & DT_DIR);
  }
#endif
};

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes)
{
#if UD_32BIT
  struct stat st = { 0 };
  if (stat(pFilename, &st) == 0)
#elif UDPLATFORM_WINDOWS
  struct _stat64 st = { 0 };
  if (_stat64(pFilename, &st) == 0)
#else
  struct stat64 st = { 0 };
  if (stat64(pFilename, &st) == 0)
#endif
  {
    if (pFileLengthInBytes)
      *pFileLengthInBytes = (int64_t)st.st_size;
    return udR_Success;
  }
  else
  {
    return udR_ObjectNotFound;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileDelete(const char *pFilename)
{
  return remove(pFilename) == -1 ? udR_Failure_ : udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder)
{
  udResult result;
  udFindDirData *pFindData = nullptr;

  pFindData = udAllocType(udFindDirData, 1, udAF_Zero);
  if (!pFindData)
  {
    result = udR_MemoryAllocationFailure;
    goto epilogue;
  }

#if UDPLATFORM_WINDOWS
  {
    udFilename fn;
    fn.SetFolder(pFolder);
    fn.SetFilenameWithExt("*.*");
    pFindData->hFind = FindFirstFileW(udOSString(fn.GetPath()), &pFindData->findFileData);
    if (pFindData->hFind == INVALID_HANDLE_VALUE)
    {
      result = udR_File_OpenFailure;
      goto epilogue;
    }
    pFindData->SetMembers();
  }
#elif UDPLATFORM_LINUX
  pFindData->pDir = opendir(pFolder);
  if (!pFindData->pDir)
  {
    result = udR_File_OpenFailure;
    goto epilogue;
  }
  pFindData->pDirent = readdir(pFindData->pDir);
  if (!pFindData->pDirent)
  {
    result = udR_ObjectNotFound;
    goto epilogue;
  }
  pFindData->SetMembers();
#elif UDPLATFORM_NACL
  // TODO: See if this implementation is required
  result = udR_ObjectNotFound;
  goto epilogue;
#endif

  result = udR_Success;
  *ppFindDir = pFindData;
  pFindData = nullptr;

epilogue:
  if (pFindData)
    udCloseDir((udFindDir**)&pFindData);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udReadDir(udFindDir *pFindDir)
{
  if (!pFindDir)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  if (!FindNextFileW(pFindData->hFind, &pFindData->findFileData))
  {
    return udR_ObjectNotFound;
  }
  pFindData->SetMembers();
#elif UDPLATFORM_LINUX
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  pFindData->pDirent = readdir(pFindData->pDir);
  if (!pFindData->pDirent)
  {
    return udR_ObjectNotFound;
  }
  pFindData->SetMembers();
#endif
  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udCloseDir(udFindDir **ppFindDir)
{
  if (!ppFindDir || !*ppFindDir)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->hFind != INVALID_HANDLE_VALUE)
    FindClose(pFindData->hFind);
#elif UDPLATFORM_LINUX
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->pDir)
    closedir(pFindData->pDir);
#endif

  udFree(*ppFindDir);
  return udR_Success;
}

// ****************************************************************************
// Author: Paul Fox, September 2015
int udSprintf(char *pDest, size_t destlength, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int length = vsnprintf(pDest, destlength, format, args);
  va_end(args);

  pDest[destlength - 1] = '\0';

  return length;
}
