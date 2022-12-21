#define _CRT_SECURE_NO_WARNINGS
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udFile.h"
#include "udMath.h"
#include "udJSON.h"

#include <sys/stat.h>
#include <time.h>
#include <chrono>

#if UDPLATFORM_WINDOWS
# include <io.h>
# include <mmsystem.h>
#else
# include <sys/types.h>
# include <pwd.h>
# include "dirent.h"
# include <time.h>
# include <sys/time.h>
# include <errno.h>
static const uint64_t nsec_per_sec = 1000000000; // 1 billion nanoseconds in one second
static const uint64_t nsec_per_msec = 1000000;   // 1 million nanoseconds in one millisecond
//static const uint64_t usec_per_msec = 1000;      // 1 thousand microseconds in one millisecond
#endif

static char s_udStrEmptyString[] = "";

// *********************************************************************
// Author: Dave Pevreal, October 2016
void udUpdateCamera(double camera[16], double yawRadians, double pitchRadians, double tx, double ty, double tz)
{
  udDouble4x4 rotation = udDouble4x4::create(camera);
  udDouble3 pos = rotation.axis.t.toVector3();
  rotation.axis.t = udDouble4::identity();

  if (yawRadians != 0.0)
    rotation = udDouble4x4::rotationZ(yawRadians) * rotation;   // Yaw on global axis
  if (pitchRadians != 0.0)
    rotation = rotation * udDouble4x4::rotationX(pitchRadians); // Pitch on local axis
  udDouble3 trans = udDouble3::zero();
  trans += rotation.axis.x.toVector3() * tx;
  trans += rotation.axis.y.toVector3() * ty;
  trans += rotation.axis.z.toVector3() * tz;
  rotation.axis.t = udDouble4::create(pos + trans, 1.0);

  memcpy(camera, rotation.a, sizeof(rotation));
}

// *********************************************************************
// Author: Dave Pevreal, October 2016
void udUpdateCamera(float camera[16], float yawRadians, float pitchRadians, float tx, float ty, float tz)
{
  udFloat4x4 rotation = udFloat4x4::create(camera);
  udFloat3 pos = rotation.axis.t.toVector3();
  rotation.axis.t = udFloat4::identity();

  rotation = udFloat4x4::rotationZ(yawRadians) * rotation;   // Yaw on global axis
  rotation = rotation * udFloat4x4::rotationX(pitchRadians); // Pitch on local axis
  pos += rotation.axis.x.toVector3() * tx;
  pos += rotation.axis.y.toVector3() * ty;
  pos += rotation.axis.z.toVector3() * tz;
  rotation.axis.t = udFloat4::create(pos, 1.0);

  memcpy(camera, rotation.a, sizeof(rotation));
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udGetTimeMs()
{
#if UDPLATFORM_UWP
  return GetTickCount();
#elif UDPLATFORM_WINDOWS
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
// Author: Dave Pevreal, April 2018
float udPerfCounterSeconds(uint64_t startValue, uint64_t end)
{
  return udPerfCounterMilliseconds(startValue, end) / 1000.f;
}

// *********************************************************************
// Author: Dave Pevreal, April 2018
int udDaysUntilExpired(int maxDays, const char **ppExpireDateStr)
{
  // Calculate the build year/month compile-time constants
  #define BUILDDATE __DATE__
  #define BUILDDATEYEAR (BUILDDATE[7]*1000 + BUILDDATE[8]*100 + BUILDDATE[9]*10 + BUILDDATE[10] - 1111*'0')
  #define BUILDDATEMONTH  (((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'n')) ?  1 : ( /* Jan */ \
                           ((BUILDDATE[0] == 'F') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'b')) ?  2 : ( /* Feb */ \
                           ((BUILDDATE[0] == 'M') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'r')) ?  3 : ( /* Mar */ \
                           ((BUILDDATE[0] == 'A') && (BUILDDATE[1] == 'p') && (BUILDDATE[2] == 'r')) ?  4 : ( /* Apr */ \
                           ((BUILDDATE[0] == 'M') && (BUILDDATE[1] == 'a') && (BUILDDATE[2] == 'y')) ?  5 : ( /* May */ \
                           ((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'n')) ?  6 : ( /* Jun */ \
                           ((BUILDDATE[0] == 'J') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'l')) ?  7 : ( /* Jul */ \
                           ((BUILDDATE[0] == 'A') && (BUILDDATE[1] == 'u') && (BUILDDATE[2] == 'g')) ?  8 : ( /* Aug */ \
                           ((BUILDDATE[0] == 'S') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'p')) ?  9 : ( /* Sep */ \
                           ((BUILDDATE[0] == 'O') && (BUILDDATE[1] == 'c') && (BUILDDATE[2] == 't')) ? 10 : ( /* Oct */ \
                           ((BUILDDATE[0] == 'N') && (BUILDDATE[1] == 'o') && (BUILDDATE[2] == 'v')) ? 11 : ( /* Nov */ \
                           ((BUILDDATE[0] == 'D') && (BUILDDATE[1] == 'e') && (BUILDDATE[2] == 'c')) ? 12 : ( /* Dec */ \
                            -1 )))))))))))))
  #define BUILDDATEDAY ((BUILDDATE[4] == ' ' ? '0' : BUILDDATE[4]) * 10 + BUILDDATE[5] - 11*'0')

  time_t nowMoment = time(0), testMoment;
  struct tm nowTm = *localtime(&nowMoment);
  struct tm buildTm = nowTm;
  struct tm testTm;
  buildTm.tm_year = BUILDDATEYEAR - 1900;
  buildTm.tm_mon = BUILDDATEMONTH - 1;
  buildTm.tm_mday = BUILDDATEDAY; // Only field that starts at 1 not zero.
  #undef BUILDDATE
  #undef BUILDDATEYEAR
  #undef BUILDDATEMONTH
  #undef BUILDDATEDAY

  int daysSince = -1;
  do
  {
    testTm = buildTm;
    testTm.tm_mday += ++daysSince;
    testMoment = mktime(&testTm);
  } while (testMoment < nowMoment && (daysSince < maxDays));

  if (ppExpireDateStr)
  {
    static char str[100];
    testTm = buildTm;
    testTm.tm_mday += maxDays;
    time_t expireTime = mktime(&testTm);
    testTm = *localtime(&expireTime);
    udSprintf(str, "%04d-%02d-%02d", testTm.tm_year + 1900, testTm.tm_mon + 1, testTm.tm_mday);
    *ppExpireDateStr = str;
  }

  return maxDays - daysSince;
}

// ****************************************************************************
// Author: Paul Fox, July 2019
int64_t udGetEpochSecsUTCd()
{
  return std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den;
}

// ****************************************************************************
// Author: Paul Fox, July 2019
double udGetEpochSecsUTCf()
{
  return 1.0 * std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den;
}

#if UDPLATFORM_WINDOWS
// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const char *pString)
{
  size_t len = udStrlen(pString) + 1;
  pUTF8 = const_cast<char*>(pString);
  pWide = udAllocType(wchar_t, len, udAF_None);
  pAllocation = pWide;

  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString, -1, pWide, (int)len) == 0)
    *pWide = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::udOSString(const wchar_t *pString)
{
  size_t len = wcslen(pString) + 1;
  size_t allocSize = len * 4;
  pUTF8 = udAllocType(char, allocSize, udAF_None);
  pWide = const_cast<wchar_t*>(pString);
  pAllocation = pUTF8;

  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pString, -1, pUTF8, (int)allocSize, nullptr, nullptr) == 0)
    *pUTF8 = 0;
}

// *********************************************************************
// Author: Dave Pevreal, June 2015
udOSString::~udOSString()
{
  udFree(pAllocation);
}
#endif // UDPLATFORM_WINDOWS

static char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// *********************************************************************
// Author: Dave Pevreal, December 2014
udResult udBase64Decode(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength, size_t *pOutputLengthWritten /*= nullptr*/)
{
  udResult result;
  uint32_t accum = 0; // Accumulator for incoming data (read 6 bits at a time)
  int accumBits = 0;
  size_t outputIndex = 0;

  if (!length && pString)
    length = udStrlen(pString);

  if (!pOutput && pOutputLengthWritten)
  {
    outputIndex = length / 4 * 3;
    UD_ERROR_SET(udR_Success);
  }

  UD_ERROR_NULL(pString, udR_InvalidParameter);
  UD_ERROR_NULL(pOutput, udR_InvalidParameter);

  for (size_t inputIndex = 0; inputIndex < length; )
  {
    char *c = strchr(b64, pString[inputIndex++]);
    if (c)
    {
      accum |= uint32_t(c - b64) << (16 - accumBits - 6); // Store in accumulator, starting at high byte
      if ((accumBits += 6) >= 8)
      {
        UD_ERROR_IF(outputIndex >= outputLength, udR_BufferTooSmall);
        pOutput[outputIndex++] = uint8_t(accum >> 8);
        accum <<= 8;
        accumBits -= 8;
      }
    }
  }
  result = udR_Success;

epilogue:
  if (pOutputLengthWritten)
    *pOutputLengthWritten = outputIndex;

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, September 2017
udResult udBase64Decode(uint8_t **ppOutput, size_t *pOutputLength, const char *pString)
{
  udResult result;
  uint8_t *pOutput = nullptr;

  UD_ERROR_NULL(ppOutput, udR_InvalidParameter);
  UD_ERROR_NULL(pOutputLength, udR_InvalidParameter);
  UD_ERROR_NULL(pString, udR_InvalidParameter);

  result = udBase64Decode(pString, 0, nullptr, 0, pOutputLength);
  UD_ERROR_HANDLE();
  pOutput = udAllocType(uint8_t, *pOutputLength, udAF_None);
  UD_ERROR_NULL(pOutput, udR_MemoryAllocationFailure);
  result = udBase64Decode(pString, 0, pOutput, *pOutputLength, pOutputLength);
  UD_ERROR_HANDLE();

  *ppOutput = pOutput;
  pOutput = nullptr;
  result = udR_Success;

epilogue:
  udFree(pOutput);
  return result;
}

// *********************************************************************
// Author: Paul Fox, March 2016
udResult udBase64Encode(const void *pBinary, size_t binaryLength, char *pString, size_t strLength, size_t *pOutputLengthWritten /*= nullptr*/)
{
  udResult result;
  uint32_t accum = 0; // Accumulator for data (read 8 bits at a time but only consume 6)
  int accumBits = 0;
  size_t outputIndex = 0;
  size_t expectedOutputLength = (binaryLength + 2) / 3 * 4 + 1; // +1 for nul terminator

  if (!pString && pOutputLengthWritten)
  {
    outputIndex = expectedOutputLength;
    UD_ERROR_SET(udR_Success);
  }

  UD_ERROR_NULL(pString, udR_InvalidParameter);
  UD_ERROR_IF(!pBinary && binaryLength, udR_InvalidParameter);

  for (size_t inputIndex = 0; inputIndex < binaryLength; ++inputIndex)
  {
    accum = (accum << 8) | ((uint8_t*)pBinary)[inputIndex];
    accumBits += 8;
    while (accumBits >= 6)
    {
      UD_ERROR_IF(outputIndex >= strLength, udR_BufferTooSmall);
      pString[outputIndex] = b64[((accum >> (accumBits - 6)) & 0x3F)];
      ++outputIndex;
      accumBits -= 6;
    }
  }

  if (accumBits == 2)
  {
    UD_ERROR_IF(outputIndex >= strLength + 3, udR_BufferTooSmall);
    pString[outputIndex] = b64[(accum & 0x3) << 4];
    pString[outputIndex+1] = '='; //Pad chars
    pString[outputIndex+2] = '=';
    outputIndex += 3;
  }
  else if (accumBits == 4)
  {
    UD_ERROR_IF(outputIndex + 2 >= strLength, udR_BufferTooSmall);
    pString[outputIndex] = b64[(accum & 0xF) << 2];
    pString[outputIndex+1] = '='; //Pad chars
    outputIndex += 2;
  }
  pString[outputIndex++] = 0; // Null terminate if room in the string

  UD_ERROR_IF(outputIndex != expectedOutputLength, udR_InternalError); // Okay, the horse may have bolted at this point.

  result = udR_Success;

epilogue:
  if (pOutputLengthWritten)
    *pOutputLengthWritten = outputIndex;

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, May 2017
udResult udBase64Encode(const char **ppDestStr, const void *pBinary, size_t binaryLength)
{
  udResult result;
  size_t expectedOutputLength = (binaryLength + 2) / 3 * 4 + 1; // +1 for nul terminator
  char *pStr = nullptr;

  UD_ERROR_NULL(ppDestStr, udR_InvalidParameter);
  pStr = udAllocType(char, expectedOutputLength, udAF_None);
  UD_ERROR_NULL(pStr, udR_MemoryAllocationFailure);
  result = udBase64Encode(pBinary, binaryLength, pStr, expectedOutputLength);
  UD_ERROR_HANDLE();
  *ppDestStr = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  return result;
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

  return 1;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

// *********************************************************************
// Author: Samuel Surtees, July 2021
udFilename::udFilename(const udFilename &o)
{
  if (o.pPath == o.path)
    this->pPath = this->path;
  else
    this->pPath = udStrdup(o.pPath);

  memcpy(this->path, o.path, udLengthOf(this->path));
  this->filenameIndex = o.filenameIndex;
  this->extensionIndex = o.extensionIndex;
}

// *********************************************************************
// Author: Samuel Surtees, July 2021
udFilename &udFilename::operator=(const udFilename &o)
{
  if (o.pPath == o.path)
    this->pPath = this->path;
  else
    this->pPath = udStrdup(o.pPath);

  memcpy(this->path, o.path, udLengthOf(this->path));
  this->filenameIndex = o.filenameIndex;
  this->extensionIndex = o.extensionIndex;
  return *this;
}

// *********************************************************************
// Author: Samuel Surtees, July 2021
udFilename::udFilename(udFilename &&o) noexcept
{
  if (o.pPath == o.path)
    this->pPath = this->path;
  else
    this->pPath = o.pPath;

  o.pPath = nullptr;
  memmove(this->path, o.path, udLengthOf(this->path));
  this->filenameIndex = o.filenameIndex;
  this->extensionIndex = o.extensionIndex;
}

// *********************************************************************
// Author: Samuel Surtees, July 2021
udFilename &udFilename::operator=(udFilename &&o) noexcept
{
  if (o.pPath == o.path)
    this->pPath = this->path;
  else
    this->pPath = o.pPath;

  o.pPath = nullptr;
  memmove(this->path, o.path, udLengthOf(this->path));
  this->filenameIndex = o.filenameIndex;
  this->extensionIndex = o.extensionIndex;
  return *this;
}

// *********************************************************************
bool udFilename::SetFromFullPath(const char *pFormat, ...)
{
  if (pFormat)
  {
    va_list args;
    va_start(args, pFormat);
    int requiredLen = udSprintfVA(nullptr, 0, pFormat, args);
    va_end(args);

    // Determine if allocation is required, otherwise use buffer
    if (requiredLen >= (int)sizeof(path))
    {
      if (pPath != path)
        udFree(pPath);

      pPath = udAllocType(char, requiredLen + 1, udAF_None);
      if (pPath == nullptr)
        goto epilogue;

      va_start(args, pFormat);
      udSprintfVA(pPath, requiredLen + 1, pFormat, args);
      va_end(args);
    }
    else
    {
      if (pPath != path)
      {
        udFree(pPath);
        pPath = path;
      }

      va_start(args, pFormat);
      udSprintfVA(path, pFormat, args);
      va_end(args);
    }

    CalculateIndices();
  }
  else
  {
    *pPath = 0;
    filenameIndex = 0;
    extensionIndex = 0;
  }

epilogue:
  if (pPath == nullptr)
  {
    pPath = path;
    *pPath = 0;
    filenameIndex = 0;
    extensionIndex = 0;
  }
  return true;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFolder(const char *pFolder)
{
  char *pNewPath = nullptr;
  char separator = '/';
  size_t i = udStrlen(pFolder);

  // If the path doesn't have a trailing seperator, look for one so we can
  // append one already being used. That is c:\path\ or c:/path/
  if (i > 2 && pFolder[i-2] != '/' && pFolder[i-2] != '\\' && pFolder[i-2] != ':')
  {
    for (--i; i > 0; --i)
    {
      if (pFolder[i-1] == '\\' || pFolder[i-1] == ':')
      {
        separator = '\\';
        break;
      }

      if (pFolder[i-1] == '/')
        break;
    }
    // TODO: Get correct separator from system when one isn't found
  }

  if (udSprintf((const char **)&pNewPath, "%s%c%s", pFolder, separator, GetFilenameWithExt()) != udR_Success)
    return false;

  if (pPath != path)
    udFree(pPath);

  pPath = pNewPath;
  CalculateIndices();

  return true;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameNoExt(const char *pFilenameOnlyComponent)
{
  size_t requiredLength = udStrlen(pFilenameOnlyComponent) + udStrlen(GetExt()) + filenameIndex + 1;
  char *pNewPath = udAllocType(char, requiredLength, udAF_None);
  bool retVal = false;

  udStrncpy(pNewPath, requiredLength, pPath, filenameIndex);
  if (!udStrcat(pNewPath, requiredLength, pFilenameOnlyComponent))
    goto epilogue;
  if (!udStrcat(pNewPath, requiredLength, GetExt()))
    goto epilogue;

  if (pPath != path)
    udFree(pPath);

  pPath = pNewPath;
  pNewPath = nullptr;
  CalculateIndices();

  retVal = true;

epilogue:
  udFree(pNewPath);
  return retVal;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetFilenameWithExt(const char *pFilenameWithExtension)
{
  size_t requiredLength = udStrlen(pFilenameWithExtension) + filenameIndex + 1;
  char *pNewPath = udAllocType(char, requiredLength, udAF_None);
  bool retVal = false;

  udStrncpy(pNewPath, requiredLength, pPath, filenameIndex);
  if (!udStrcat(pNewPath, requiredLength, pFilenameWithExtension))
    goto epilogue;

  if (pPath != path)
    udFree(pPath);

  pPath = pNewPath;
  pNewPath = nullptr;
  CalculateIndices();

  retVal = true;

epilogue:
  udFree(pNewPath);
  return retVal;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udFilename::SetExtension(const char *pExtComponent)
{
  size_t requiredLength = udStrlen(pExtComponent) + extensionIndex + 1;
  char *pNewPath = udAllocType(char, requiredLength, udAF_None);
  bool retVal = false;

  udStrncpy(pNewPath, requiredLength, pPath, extensionIndex);
  if (!udStrcat(pNewPath, requiredLength, pExtComponent))
    goto epilogue;

  if (pPath != path)
    udFree(pPath);

  pPath = pNewPath;
  pNewPath = nullptr;
  CalculateIndices();

  retVal = true;

epilogue:
  udFree(pNewPath);
  return retVal;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFolder(char *pFolder, int folderLen)
{
  int folderChars = filenameIndex;
  if (pFolder)
    udStrncpy(pFolder, folderLen, pPath, folderChars);
  return folderChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udFilename::ExtractFilenameOnly(char *pFilename, int filenameLen)
{
  int filenameChars = extensionIndex - filenameIndex;
  if (pFilename)
  {
    udStrncpy(pFilename, filenameLen, pPath + filenameIndex, filenameChars);
  }
  return filenameChars + 1;
}

// ---------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
void udFilename::CalculateIndices()
{
  int len = (int)strlen(pPath);
  // Set filename and extension indices to null terminator as a sentinal
  filenameIndex = -1;
  extensionIndex = len; // If no extension, point extension to nul terminator

  const char *pGetParams = nullptr;

  if (udStrBeginsWithi(pPath, "http://") || udStrBeginsWithi(pPath, "https://"))
    pGetParams = udStrchr(pPath, "?");

  if (pGetParams != nullptr)
    len = (int)(pGetParams - pPath);

  for (--len; len >= 0 && (filenameIndex == -1 || extensionIndex == -1); --len)
  {
    if (pPath[extensionIndex] == 0 && pPath[len] == '.') // Last period
      extensionIndex = len;
    if (filenameIndex == -1 && (pPath[len] == '/' || pPath[len] == '\\' || pPath[len] == ':'))
      filenameIndex = len + 1;
  }
  // If no path indicators were found the filename is the beginning of the path
  if (filenameIndex == -1)
    filenameIndex = 0;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
void udFilename::Debug()
{
  char folder[260];
  char name[50];

  ExtractFolder(folder, sizeof(folder));
  ExtractFilenameOnly(name, sizeof(name));
  udDebugPrintf("folder<%s> name<%s> ext<%s> filename<%s> -> %s\n", folder, name, GetExt(), GetFilenameWithExt(), pPath);
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
udResult udURL::SetURL(const char *pURL)
{
  udFree(pURLText);
  pScheme = s_udStrEmptyString;
  pDomain = s_udStrEmptyString;
  pPath = s_udStrEmptyString;
  static const char specialChars[]   = { ' ',  '#',   '%',   '+',   '?',   '\0', }; // Made for extending later, not wanting to encode any more than we need to
  static const char *pSpecialSubs[] = { "%20", "%23", "%25", "%2B", "%3F", "", };

  if (pURL)
  {
    // Take a copy of the entire string
    char *p = pURLText = udStrdup(pURL, udStrlen(pURL) * 3 + 2); // Add an extra chars for nul terminate domain, and URL encoding switches for every character (worst case)
    if (!pURLText)
      return udR_MemoryAllocationFailure;

    size_t i, charListIndex;

    // Isolate the scheme
    udStrchr(p, ":/", &i); // Find the colon that breaks the scheme, but don't search past a slash

    if (p[i] == ':') // Test in case of malformed url (potentially allowing a default scheme such as 'http'
    {
      pScheme = p;
      p[i] = 0; // null terminate the scheme
      p = p + i + 1;
      // Skip over the // at start of domain (if present)
      if (p[0] == '/' && p[1] == '/')
        p += 2;
    }

    // Isolate the domain - this is slightly painful because ipv6 has colons
    pDomain = p;
    udStrchr(p, p[0] == '[' ? "/]" : "/:", &i); // Find the character that breaks the domain, but don't search past a slash
    if (p[0] == '[' && p[i] == ']') ++i; // Skip over closing bracket of ipv6 address
    if (p[i] == ':')
    {
      // A colon is present, so decode the port number
      int portChars;
      port = udStrAtoi(&p[i+1], &portChars);
      p[i] = 0; // null terminate the domain
      i += portChars + 1;
    }
    else
    {
      // Otherwise assume port 443 for https, or 80 for anything else (should be http)
      port = udStrEqual(pScheme, "https") ? 443 : 80;
      // Because no colon character exists to terminate the domain, move it forward by 1 byte
      if (p[i] != 0)
      {
        memmove(p + i + 1, p + i, udStrlen(p + i) + 1); // Move the string one to the right to retain the separator (note: 1 byte was added to allocation when udStrdup called)
        p[i++] = 0; // null terminate the domain
      }
    }
    p += i;

    // Finally, the path is the last component (for now, unless the class is extended to support splitting the query string too)
    pPath = p;

    // Now, find any "special" URL characters in the path and encode them
    while ((p = (char*)udStrchr(p, specialChars, nullptr, &charListIndex)) != nullptr)
    {
      size_t len = udStrlen(pSpecialSubs[charListIndex]);
      memmove(p + len - 1, p, udStrlen(p) + 1);
      memcpy(p, pSpecialSubs[charListIndex], len);
      p += len;
    }
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
  udBMPHeader header;
  memset(&header, 0, sizeof(header));
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

  result = udFile_Write(pFile, &header, sizeof(header));
  if (result != udR_Success)
    goto error;

  for (int y = height - 1; y >= 0 ; --y)
  {
    for (int x = 0; x < width; ++x)
      memcpy(&pLine[x*3], &((uint8_t*)pColorData)[y * pitchInBytes + x * 4], 3);

    result = udFile_Write(pFile, pLine, paddedLineSize);
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
  udResult result;
  udBMPHeader header;
  memset(&header, 0, sizeof(header));
  udFile *pFile = nullptr;
  uint8_t *pColors = nullptr;
  uint8_t *pLine = nullptr;
  int paddedLineSize;

  UD_ERROR_NULL(pFilename, udR_InvalidParameter);
  UD_ERROR_NULL(ppColorData, udR_InvalidParameter);
  UD_ERROR_IF(!pWidth || !pHeight, udR_InvalidParameter);

  UD_ERROR_CHECK(udFile_Open(&pFile, pFilename, udFOF_Read));
  UD_ERROR_CHECK(udFile_Read(pFile, &header, sizeof(header)));

  *pWidth = header.biWidth;
  *pHeight = header.biHeight;
  paddedLineSize = (*pWidth * 3 + 3) & ~3;
  pColors = udAllocType(uint8_t, *pWidth * *pHeight * 4, udAF_None);
  UD_ERROR_NULL(pColors, udR_MemoryAllocationFailure);
  pLine = udAllocType(uint8_t, paddedLineSize, udAF_None);
  UD_ERROR_NULL(pLine, udR_MemoryAllocationFailure);

  for (int y = *pHeight - 1; y >= 0 ; --y)
  {
    UD_ERROR_CHECK(udFile_Read(pFile, pLine, paddedLineSize));
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

// ----------------------------------------------------------------------------
struct udFindDirData : public udFindDir
{
#if UDPLATFORM_WINDOWS
  HANDLE hFind;
  WIN32_FIND_DATAW findFileData;
  char utf8Filename[2048];

  void SetMembers()
  {
    // Take a copy of the filename after translation from wide-char to utf8
    udStrcpy(utf8Filename, udOSString(findFileData.cFileName));
    pFilename = utf8Filename;
    isDirectory = !!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  DIR *pDir;
  struct dirent *pDirent;

  void SetMembers()
  {
    pFilename = pDirent->d_name;
    isDirectory = !!(pDirent->d_type & DT_DIR);
  }
#else
#error "Unsupported platform"
#endif
};

#if UD_32BIT
# define UD_STAT_STRUCT stat
# define UD_STAT_FUNC stat
# define UD_STAT_MODTIME (int64_t)st.st_mtime
# define UD_STAT_ISDIR (st.st_mode & S_IFDIR)
#elif UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS
// Apple made these 64bit and deprecated the 64bit variants
# define UD_STAT_STRUCT stat
# define UD_STAT_FUNC stat
# define UD_STAT_MODTIME (int64_t)st.st_mtime
# define UD_STAT_ISDIR S_ISDIR(st.st_mode)
#elif UDPLATFORM_WINDOWS
# define UD_STAT_STRUCT _stat64
# define UD_STAT_FUNC _wstat64
# define UD_STAT_MODTIME (int64_t)st.st_mtime
# define UD_STAT_ISDIR (st.st_mode & _S_IFDIR)
#elif UDPLATFORM_LINUX
# define UD_STAT_STRUCT stat64
# define UD_STAT_FUNC stat64
# define UD_STAT_MODTIME (int64_t)st.st_mtim.tv_sec
# define UD_STAT_ISDIR (st.st_mode & S_IFDIR)
#elif UDPLATFORM_ANDROID
# define UD_STAT_STRUCT stat64
# define UD_STAT_FUNC stat64
# define UD_STAT_MODTIME (int64_t)st.st_mtime
# define UD_STAT_ISDIR (st.st_mode & S_IFDIR)
#else
# error "Unsupported Platform"
#endif

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes, int64_t *pModifiedTime)
{
  const char *pPath = pFilename;
  const char *pNewPath = nullptr;
  if (udFile_TranslatePath((const char **)&pNewPath, pFilename) == udR_Success)
    pPath = pNewPath;

  struct UD_STAT_STRUCT st;
  memset(&st, 0, sizeof(st));

  int result = UD_STAT_FUNC(udOSString(pPath), &st);
  udFree(pNewPath);

  if (result == 0)
  {
    if (pFileLengthInBytes)
      *pFileLengthInBytes = (int64_t)st.st_size;

    if (pModifiedTime)
      *pModifiedTime = UD_STAT_MODTIME;

    return udR_Success;
  }
  else
  {
    return udR_NotFound;
  }
}

// ****************************************************************************
// Author: Damian Madden, January 2020
udResult udDirectoryExists(const char *pFilename, int64_t *pModifiedTime)
{
  // Assume if only a drive letter is specified that it exists (for now)
  if (udStrlen(pFilename) <= 3 && udStrchr(pFilename, ":") != nullptr)
    return udR_Success;

  const char *pPath = pFilename;
  const char *pNewPath = nullptr;
  if (udFile_TranslatePath((const char **)&pNewPath, pFilename) == udR_Success)
    pPath = pNewPath;

  struct UD_STAT_STRUCT st;
  memset(&st, 0, sizeof(st));

  int result = UD_STAT_FUNC(udOSString(pPath), &st);
  udFree(pNewPath);

  if (result == 0)
  {
    if (!UD_STAT_ISDIR)
      return udR_ObjectTypeMismatch; // Exists but isn't directory

    if (pModifiedTime)
      *pModifiedTime = UD_STAT_MODTIME;

    return udR_Success;
  }
  else
  {
    return udR_NotFound;
  }
}

#undef UD_STAT_STRUCT
#undef UD_STAT_FUNC
#undef UD_STAT_ISDIR
#undef UD_STAT_MODTIME

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udFileDelete(const char *pFilename)
{
  const char *pPath = pFilename;
  const char *pNewPath = nullptr;
  if (udFile_TranslatePath((const char **)&pNewPath, pFilename) == udR_Success)
    pPath = pNewPath;

  udResult result = remove(pPath) == -1 ? udR_Failure : udR_Success;
  udFree(pNewPath);

  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder)
{
  udResult result;
  udFindDirData *pFindData = nullptr;

  pFindData = udAllocType(udFindDirData, 1, udAF_Zero);
  UD_ERROR_NULL(pFindData, udR_MemoryAllocationFailure);

#if UDPLATFORM_WINDOWS
  {
    udFilename fn;
    fn.SetFolder(pFolder);
    fn.SetFilenameWithExt("*.*");
    pFindData->hFind = FindFirstFileW(udOSString(fn.GetPath()), &pFindData->findFileData);
    if (pFindData->hFind == INVALID_HANDLE_VALUE)
      UD_ERROR_SET_NO_BREAK(udR_OpenFailure);
    pFindData->SetMembers();
  }
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  pFindData->pDir = opendir(pFolder);
  UD_ERROR_NULL(pFindData->pDir, udR_OpenFailure);
  pFindData->pDirent = readdir(pFindData->pDir);
  UD_ERROR_NULL(pFindData->pDirent, udR_NotFound);
  pFindData->SetMembers();
#else
  #error "Unsupported Platform"
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
    return udR_InvalidParameter;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  if (!FindNextFileW(pFindData->hFind, &pFindData->findFileData))
    return udR_NotFound;
  pFindData->SetMembers();
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  udFindDirData *pFindData = static_cast<udFindDirData *>(pFindDir);
  pFindData->pDirent = readdir(pFindData->pDir);
  if (!pFindData->pDirent)
    return udR_NotFound;
  pFindData->SetMembers();
#else
  #error "Unsupported Platform"
#endif
  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2014
udResult udCloseDir(udFindDir **ppFindDir)
{
  if (!ppFindDir || !*ppFindDir)
    return udR_InvalidParameter;

#if UDPLATFORM_WINDOWS
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->hFind != INVALID_HANDLE_VALUE)
    FindClose(pFindData->hFind);
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
  udFindDirData *pFindData = static_cast<udFindDirData *>(*ppFindDir);
  if (pFindData->pDir)
    closedir(pFindData->pDir);
#else
  #error "Unsupported Platform"
#endif

  udFree(*ppFindDir);
  return udR_Success;
}

// ****************************************************************************
// Author: Dave Pevreal, September 2020
udResult udCreateDir(const char *pDirPath, int *pDirsCreatedCount)
{
  udResult result;
  char *pPath = nullptr;
  size_t currPathLen; // Length of the path string that we're attempting now
  size_t fullPathLen; // Length of the full path (for comparison)
  int dirsCreatedCount = 0;
  char truncChar = 0; // Character at truncation point (currPathLen)

  UD_ERROR_NULL(pDirPath, udR_InvalidParameter);
  if (udFile_TranslatePath(const_cast<const char**>(&pPath), pDirPath) != udR_Success)
  {
    pPath = udStrdup(pDirPath);
    UD_ERROR_NULL(pPath, udR_MemoryAllocationFailure);
  }

  fullPathLen = udStrlen(pPath);
  // If there's a trailing slash(s), remove it(them) because that just creates confusion
  while (fullPathLen > 0 && (pPath[fullPathLen - 1] == '\\' || pPath[fullPathLen - 1] == '/'))
    pPath[--fullPathLen] = 0;

  // Special case allowing creation of empty or root folders to "succeed" with create count being zero
  UD_ERROR_IF(fullPathLen == 0, udR_Success);

  // Attempt the full path first, if it completes this is the most efficient case
  for (currPathLen = fullPathLen; ;)
  {
#if UDPLATFORM_WINDOWS
    bool actuallyCreated = CreateDirectoryW(udOSString(pPath), nullptr) != 0;
    bool alreadyExisted = !actuallyCreated && (GetLastError() == ERROR_ALREADY_EXISTS);
#else
    bool actuallyCreated = mkdir(pPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
    bool alreadyExisted = !actuallyCreated && (errno == EEXIST);
#endif

    if (actuallyCreated)
      ++dirsCreatedCount; // A folder was created, account for it

    if (!actuallyCreated && !alreadyExisted)
    {
      // Directory creation failed, move back one folder and try there
      while (currPathLen > 0 && (pPath[currPathLen] != '\\' && pPath[currPathLen] != '/'))
        --currPathLen;
      if (currPathLen == 0)
        UD_ERROR_SET_NO_BREAK(udR_Failure); // Weren't able to make any of the folders
      truncChar = pPath[currPathLen];
      pPath[currPathLen] = 0;

      // If directory already exists, user can't create folders here
      UD_ERROR_IF(udDirectoryExists(pPath, nullptr) != udR_NotFound, udR_Failure);
    }
    else if (currPathLen != fullPathLen)
    {
      // Directory creation succeeded on a sub-path, replace truncation character and move forward
      pPath[currPathLen++] = truncChar;
      while (currPathLen != fullPathLen && pPath[currPathLen] != '\0')
        ++currPathLen;
    }
    else
    {
      UD_ERROR_SET(udR_Success);
    }
  }

epilogue:
  if (pDirsCreatedCount)
    *pDirsCreatedCount = dirsCreatedCount;
  udFree(pPath);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, August 2018
udResult udRemoveDir(const char *pDirPath, int removeCount)
{
  udResult result;
  char *pPath = nullptr;
  size_t pathLen;

  UD_ERROR_NULL(pDirPath, udR_InvalidParameter);
  if (udFile_TranslatePath(const_cast<const char **>(&pPath), pDirPath) != udR_Success)
  {
    pPath = udStrdup(pDirPath);
    UD_ERROR_NULL(pPath, udR_MemoryAllocationFailure);
  }

  pathLen = udStrlen(pPath);
  // If there's a trailing slash(s), remove it(them) because that just creates confusion
  while (pathLen > 0 && (pPath[pathLen - 1] == '\\' || pPath[pathLen - 1] == '/'))
    pPath[--pathLen] = 0;

  // Special case allowing removal of empty or root folders to "succeed"
  UD_ERROR_IF(pathLen == 0, udR_Success);

  for (; *pPath && removeCount > 0; --removeCount)
  {
#if UDPLATFORM_WINDOWS
    // Returns 0 on fail
    if (RemoveDirectoryW(udOSString(pPath)) == 0)
      UD_ERROR_SET_NO_BREAK(udR_Failure);
#else
    // Returns 0 on success
    if (rmdir(pPath) != 0)
      UD_ERROR_SET_NO_BREAK(udR_Failure);
#endif

    // Directory creation failed, move back one folder and try there
    while (pathLen > 0 && (pPath[pathLen] != '\\' && pPath[pathLen] != '/'))
      --pathLen;
    pPath[pathLen] = 0;
  }

  result = udR_Success;

epilogue:
  udFree(pPath);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
udResult udParseWKT(udJSON *pValue, const char *pString, int *pCharCount)
{
  udResult result = udR_Success;;
  size_t idLen;
  udJSON temp;
  int tempCharCount = 0;
  int parameterNumber = 0;
  const char *pStartString = pString;

  UD_ERROR_NULL(pValue, udR_InvalidParameter);
  UD_ERROR_NULL(pString, udR_InvalidParameter);

  pString = udStrSkipWhiteSpace(pString);
  while (*pString && *pString != ']')
  {
    g_udBreakOnError = false;
    udResult parseResult = temp.Parse(pString, &tempCharCount);
    g_udBreakOnError = true;
    if (parseResult == udR_Success)
    {
      if (!parameterNumber && temp.IsString())
        pValue->Set(&temp, "name");
      else
        pValue->Set(&temp, "values[]");
      ++parameterNumber;
      pString += tempCharCount;
    }
    else
    {
      // Assume an object identifier, or an unquoted constant
      udStrchr(pString, "[],", &idLen);
      if (pString[idLen] == '[')
      {
        temp.Set("type = '%.*s'", (int)idLen, pString);
        result = udParseWKT(&temp, pString + idLen + 1, &tempCharCount);
        UD_ERROR_HANDLE();
        pValue->Set(&temp, "values[]");
        pString += idLen + 1 + tempCharCount;
      }
      else // Any non-parsable is now considered a string, eg AXIS["Easting",EAST] parses as AXIS["Easting","EAST"]
      {
        pValue->Set("values[] = '%.*s'", (int)idLen, pString);
        ++parameterNumber;
        pString += idLen;
      }
    }
    if (*pString == ',')
      ++pString;
    pString = udStrSkipWhiteSpace(pString);
  }
  if (*pString == ']')
    ++pString;
  if (pCharCount)
    *pCharCount = int(pString - pStartString);

epilogue:
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2018
static udResult GetWKTElementStr(const char **ppOutput, const udJSON &value)
{
  udResult result;
  const char *pElementSeperator = ""; // Changed to "," after first element is written
  const char *pStr = nullptr;
  const char *pElementStr = nullptr;
  const char *pTypeStr;
  const char *pNameStr;
  size_t valuesCount;

  pTypeStr = value.Get("type").AsString();
  UD_ERROR_NULL(pTypeStr, udR_ParseError);
  pNameStr = value.Get("name").AsString();
  valuesCount = value.Get("values").ArrayLength();
  UD_ERROR_CHECK(udSprintf(&pStr, "%s[", pTypeStr));
  if (pNameStr)
  {
    UD_ERROR_CHECK(udSprintf(&pStr, "%s\"%s\"", pStr, pNameStr));
    pElementSeperator = ",";
  }
  for (size_t i = 0; i < valuesCount; ++i)
  {
    const udJSON &arrayValue = value.Get("values[%d]", (int)i);
    if (arrayValue.IsObject())
      UD_ERROR_CHECK(GetWKTElementStr(&pElementStr, arrayValue));
    else if (arrayValue.IsString() && i == 0 && udStrEqual(pTypeStr, "AXIS")) // Special case for AXIS, output second string unquoted
      UD_ERROR_CHECK(udSprintf(&pElementStr, "%s", arrayValue.AsString()));
    else if (arrayValue.IsString())
      UD_ERROR_CHECK(udSprintf(&pElementStr, "\"%s\"", arrayValue.AsString()));
    else
      UD_ERROR_CHECK(arrayValue.ToString(&pElementStr));
    UD_ERROR_CHECK(udSprintf(&pStr, "%s%s%s", pStr, pElementSeperator, pElementStr));
    udFree(pElementStr);
    pElementSeperator = ",";
  }
  // Put the closing brace on
  UD_ERROR_CHECK(udSprintf(&pStr, "%s]", pStr));

  // Transfer ownership
  *ppOutput = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  udFree(pElementStr);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
udResult udExportWKT(const char **ppOutput, const udJSON *pValue)
{
  udResult result;
  const char *pStr = nullptr;
  const char *pElementStr = nullptr;
  const char *pTemp = nullptr;
  size_t elemCount;

  UD_ERROR_NULL(ppOutput, udR_InvalidParameter);
  UD_ERROR_NULL(pValue, udR_InvalidParameter);

  elemCount = pValue->Get("values").ArrayLength();
  for (size_t i = 0; i < elemCount; ++i)
  {
    UD_ERROR_CHECK(GetWKTElementStr(&pElementStr, pValue->Get("values[%d]", (int)i)));
    if (!pStr)
    {
      pStr = pElementStr;
      pElementStr = nullptr;
    }
    else
    {
      pTemp = pStr;
      pStr = nullptr;
      UD_ERROR_CHECK(udSprintf(&pStr, "%s%s", pTemp, pElementStr));
    }
    udFree(pElementStr);
    udFree(pTemp);
  }
  // Transfer ownership
  *ppOutput = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  udFree(pStr);
  udFree(pElementStr);
  udFree(pTemp);
  return result;
}
