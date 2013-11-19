#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>



// *********************************************************************
size_t udStrcpy(char *dest, size_t destLen, const char *src)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = "";
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
size_t udStrncpy(char *dest, size_t destLen, const char *src, size_t maxChars)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = "";
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
size_t udStrcat(char *dest, size_t destLen, const char *src)
{
  if (dest == NULL)
    return 0;
  if (src == NULL) // Special case, handle a NULL source as an empty string
    src = "";
  size_t destChars = strlen(dest); // Note: Not including terminator
  size_t srcChars = strlen(src);
  if ((destChars + srcChars + 1) > destLen)
    return 0;
  strcat(dest, src);
  return destChars + srcChars + 1;
}

// *********************************************************************
int udStrcmp(const char *s1, const char *s2)
{
  static const char *empty = "";
  if (!s1) s1 = empty;
  if (!s2) s2 = empty;

  int result;
  do
  {
    result = *s1 - *s2;
  } while (!result && *s1++ && *s2++);
  
  return result;
}

// *********************************************************************
int udAddToStringTable(char *&stringTable, uint32_t &stringTableLength, const char *addString)
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
  char *newCache = (char*)udRealloc(stringTable, newLength);
  if (!newCache)
    return -1; // A nasty case where memory allocation has failed
  strcpy(newCache + offset, addString);
  stringTable = newCache;
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
bool udFilename::SetFilenameWithExt(const char *filenameWithExtension)
{
  char newPath[MaxPath];

  ExtractFolder(newPath, sizeof(newPath));
  if (!udStrcat(newPath, sizeof(newPath), filenameWithExtension))
    return false;
  return SetFromFullPath(newPath);
}

// *********************************************************************
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
int udFilename::ExtractFolder(char *folder, int folderLen)
{
  int folderChars = m_filenameIndex;
  if (folder != NULL)
    udStrncpy(folder, folderLen, m_path, folderChars);
  return folderChars + 1;
}

// *********************************************************************
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
void udFilename::Debug()
{
  char folder[260];
  char name[50];

  ExtractFolder(folder, sizeof(folder));
  ExtractFilenameOnly(name, sizeof(name));
  udDebugPrintf("folder<%s> name<%s> ext<%s> filename<%s> -> %s\n", folder, name, GetExt(), GetFilenameWithExt(), m_path);
}
