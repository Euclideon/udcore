#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if UDPLATFORM_WINDOWS
#include <mmsystem.h>
#elif UDPLATFORM_LINUX
#include <sys/time.h>
#endif

static char s_udStrEmptyString[] = "";

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udGetTimeMs()
{
#if UDPLATFORM_WINDOWS
  return timeGetTime();
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_usec/1000;
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
  char *dup = udAllocType(char, len + additionalChars);
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

  //const char *result = strstr(s, pSubString);

  //if (pIndex)
  //  *pIndex = (result) ? result - s : strlen(s);

  //return result;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
int32_t udStrAtoi(const char *s, size_t *pCharCount, int radix)
{
  if (!s) s = s_udStrEmptyString;
  int32_t result = 0;
  int charCount = 0;
  bool negate = false;

  while (s[charCount] == ' ' || s[charCount] == '\t')
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
int64_t udStrAtoi64(const char *s, size_t *pCharCount, int radix)
{
  if (!s) s = s_udStrEmptyString;
  int64_t result = 0;
  int charCount = 0;
  bool negate = false;

  while (s[charCount] == ' ' || s[charCount] == '\t')
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
int udAddToStringTable(char *&pStringTable, uint32_t &stringTableLength, const char *addString)
{
  uint32_t offset = 0;
  int addStrLen = (int)udStrlen(addString); // Length of string to be added

  while (offset < stringTableLength)
  {
    int curStrLen = (int)udStrlen(pStringTable + offset);
    if (offset + curStrLen > stringTableLength)
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
  stringTableLength = offset + addStrLen + 1;
  return offset;
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
      size_t portChars;
      m_port = udStrAtoi(&p[i+1], &portChars);
      i += portChars + 1;
    }
    else
    {
      // Otherwise let's assume port 80. TODO: Default port should be based on the scheme
      m_port = 80;
    }
    if (p[i] != 0)
    {
      memmove(p + i + 1, p + i, udStrlen(p + i) + 1); // Move the string one to the right to retain the separator (note: 1 byte was added to allocation when udStrdup called)
      p[i++] = 0; // null terminate the domain
    }
    p += i;

    // Finally, the path is the last component (for now, unless the class is extended to support splitting the query string too)
    m_pPath = p;
  }

  return udR_Success; // TODO: Perhaps return an error if the url isn't formed properly
}
