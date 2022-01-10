#define _CRT_SECURE_NO_WARNINGS
#include "udStringUtil.h"
#include "udMath.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <atomic>

static char s_udStrEmptyString[] = "";

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
  // Find number of characters in string but stop at maxChars (faster than using strlen)
  size_t srcChars = 0;
  while (srcChars < maxChars && src[srcChars])
    ++srcChars;

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
size_t udStrcat(char *pDest, size_t destLen, const char *pSrc)
{
  if (!pDest) return 0;
  if (!pSrc) pSrc = s_udStrEmptyString;

  size_t destChars = strlen(pDest); // Note: Not including terminator
  size_t srcChars = strlen(pSrc);
  if ((destChars + srcChars + 1) > destLen)
    return 0;
  strcat(pDest, pSrc);
  return destChars + srcChars + 1;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmp(const char *pStr1, const char *pStr2)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result;
  do
  {
    result = *pStr1 - *pStr2;
  } while (!result && *pStr1++ && *pStr2++);

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udStrcmpi(const char *pStr1, const char *pStr2)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result;
  do
  {
    result = tolower(*pStr1) - tolower(*pStr2);
  } while (!result && *pStr1++ && *pStr2++);

  return result;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
int udStrncmp(const char *pStr1, const char *pStr2, size_t maxChars)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result = 0;
  if (maxChars)
  {
    do
    {
      result = *pStr1 - *pStr2;
    } while (!result && *pStr1++ && *pStr2++ && --maxChars);
  }

  return result;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
int udStrncmpi(const char *pStr1, const char *pStr2, size_t maxChars)
{
  if (!pStr1) pStr1 = s_udStrEmptyString;
  if (!pStr2) pStr2 = s_udStrEmptyString;

  int result = 0;
  if (maxChars)
  {
    do
    {
      result = tolower(*pStr1) - tolower(*pStr2);
    } while (!result && *pStr1++ && *pStr2++ && --maxChars);
  }

  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
size_t udStrlen(const char *pStr)
{
  if (!pStr) pStr = s_udStrEmptyString;

  size_t len = 0;
  while (*pStr++)
    ++len;

  return len;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
bool udStrBeginsWith(const char *pStr, const char *pPrefix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pPrefix) pPrefix = s_udStrEmptyString;

  while (*pPrefix)
  {
    if (*pStr++ != *pPrefix++)
      return false;
  }
  return true;
}

// *********************************************************************
// Author: Samuel Surtees, April 2017
bool udStrBeginsWithi(const char *pStr, const char *pPrefix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pPrefix) pPrefix = s_udStrEmptyString;

  while (*pPrefix)
  {
    if (tolower(*pStr++) != tolower(*pPrefix++))
      return false;
  }
  return true;
}

// *********************************************************************
// Author: Dave Pevreal, August 2018
bool udStrEndsWith(const char *pStr, const char *pSuffix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSuffix) pSuffix = s_udStrEmptyString;
  size_t sLen = udStrlen(pStr);
  size_t suffixLen = udStrlen(pSuffix);

  if (sLen < suffixLen)
    return false;
  return udStrcmp(pStr + sLen - suffixLen, pSuffix) == 0;
}

// *********************************************************************
// Author: Dave Pevreal, August 2018
bool udStrEndsWithi(const char *pStr, const char *pSuffix)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSuffix) pSuffix = s_udStrEmptyString;
  size_t sLen = udStrlen(pStr);
  size_t suffixLen = udStrlen(pSuffix);

  if (sLen < suffixLen)
    return false;
  return udStrcmpi(pStr + sLen - suffixLen, pSuffix) == 0;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
#ifdef __MEMORY_DEBUG__
char *_udStrdup(const char *pStr, size_t additionalChars, const char *pFile, int line)
#else
char *udStrdup(const char *pStr, size_t additionalChars)
#endif
{
  if (!pStr && !additionalChars) return nullptr; // This allows us to duplicate null's as null's
  if (!pStr) pStr = s_udStrEmptyString;

  size_t len = udStrlen(pStr) + 1;
  char *pDup = (char *)_udAlloc(sizeof(char) * (len + additionalChars), udAF_None, IF_MEMORY_DEBUG(pFile, line));
  if (pDup)
    memcpy(pDup, pStr, len);

  return pDup;
}


// *********************************************************************
// Author: Dave Pevreal, May 2017
char *udStrndup(const char *pStr, size_t maxChars, size_t additionalChars)
{
  if (!pStr && !additionalChars) return nullptr; // This allows us to duplicate null's as null's
  if (!pStr) pStr = s_udStrEmptyString;
  // Find minimum of maxChars and udStrlen(pStr) without using udStrlen which can be slow
  size_t len = 0;
  while (len < maxChars && pStr[len])
    ++len;
  char *pDup = udAllocType(char, len + 1 + additionalChars, udAF_None);
  if (pDup)
  {
    memcpy(pDup, pStr, len);
    pDup[len] = 0;
  }

  return pDup;
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
template <bool insensitive>
const char *udStrchr_Internal(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t index;
  size_t listLen = udStrlen(pCharList);

  for (index = 0; pStr[index]; ++index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (pStr[index] == pCharList[i] || (insensitive && tolower(pStr[index]) == tolower(pCharList[i])))
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        if (pCharListIndex)
          *pCharListIndex = i;
        return pStr + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = index;
  return nullptr;
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrchr(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  return udStrchr_Internal<false>(pStr, pCharList, pIndex, pCharListIndex);
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrchri(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  return udStrchr_Internal<true>(pStr, pCharList, pIndex, pCharListIndex);
}


// *********************************************************************
// Author: Samuel Surtees, May 2015
template <bool insensitive>
const char *udStrrchr_Internal(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pCharList) pCharList = s_udStrEmptyString;

  size_t sLen = udStrlen(pStr);
  size_t listLen = udStrlen(pCharList);

  for (ptrdiff_t index = sLen - 1; index >= 0; --index)
  {
    for (size_t i = 0; i < listLen; ++i)
    {
      if (pStr[index] == pCharList[i] || (insensitive && tolower(pStr[index]) == tolower(pCharList[i])))
      {
        // Found, set index if required and return a pointer to the found character
        if (pIndex)
          *pIndex = index;
        if (pCharListIndex)
          *pCharListIndex = i;
        return pStr + index;
      }
    }
  }
  // Not found, but update the index if required
  if (pIndex)
    *pIndex = sLen;
  return nullptr;
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrrchr(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  return udStrrchr_Internal<false>(pStr, pCharList, pIndex, pCharListIndex);
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrrchri(const char *pStr, const char *pCharList, size_t *pIndex, size_t *pCharListIndex)
{
  return udStrrchr_Internal<true>(pStr, pCharList, pIndex, pCharListIndex);
}


// *********************************************************************
// Author: Dave Pevreal, March 2014
template <bool insensitive>
const char *udStrstr_Internal(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex)
{
  if (!pStr) pStr = s_udStrEmptyString;
  if (!pSubString) pSubString = s_udStrEmptyString;
  size_t i;
  size_t subStringIndex = 0;

  for (i = 0; pStr[i] && (!sLen || i < sLen); ++i)
  {
    if (pStr[i] == pSubString[subStringIndex] || (insensitive && tolower(pStr[i]) == tolower(pSubString[subStringIndex])))
    {
      if (pSubString[++subStringIndex] == 0)
      {
        // Substring found, move index to beginning of instance
        i = i + 1 - subStringIndex;
        if (pIndex)
          *pIndex = i;
        return pStr + i;
      }
    }
    else
    {
      i -= subStringIndex;
      subStringIndex = 0;
    }
  }

  // Substring not found
  if (pIndex)
    *pIndex = i;
  return nullptr;
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrstr(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex)
{
  return udStrstr_Internal<false>(pStr, sLen, pSubString, pIndex);
}


// *********************************************************************
// Author: Samuel Surtees, December 2020
const char *udStrstri(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex)
{
  return udStrstr_Internal<true>(pStr, sLen, pSubString, pIndex);
}

// *********************************************************************
// Author: Dave Pevreal, August 2014
int udStrTokenSplit(char *pLine, const char *pDelimiters, char *pTokenArray[], int maxTokens)
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
      pLine += delimiterIndex;                          // Move pLine to end of the line
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
int32_t udStrAtoi(const char *pStr, int *pCharCount, int radix)
{
  return (int32_t)udStrAtoi64(pStr, pCharCount, radix);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint32_t udStrAtou(const char *pStr, int *pCharCount, int radix)
{
  return (uint32_t)udStrAtou64(pStr, pCharCount, radix);
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int64_t udStrAtoi64(const char *pStr, int *pCharCount, int radix)
{
  int64_t result = 0;
  int charCount = 0;
  int digitCount = 0;
  bool negate = false;

  if (pStr && radix >= 2 && radix <= 36)
  {
    while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
      ++charCount;
    if (pStr[charCount] == '+')
      ++charCount;
    if (pStr[charCount] == '-')
    {
      negate = true;
      ++charCount;
    }
    result = (int64_t)udStrAtou64(pStr + charCount, &digitCount, radix);
    if (negate)
      result = -result;
  }
  if (pCharCount)
    *pCharCount = charCount + digitCount;
  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
uint64_t udStrAtou64(const char *pStr, int *pCharCount, int radix)
{
  uint64_t result = 0;
  int charCount = 0;
  if (pStr && radix >= 2 && radix <= 36)
  {
    while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
      ++charCount;
    for (; pStr[charCount]; ++charCount)
    {
      int nextValue = radix; // A sentinal to force end of processing
      if (pStr[charCount] >= '0' && pStr[charCount] <= '9')
        nextValue = pStr[charCount] - '0';
      else if (pStr[charCount] >= 'a' && pStr[charCount] < ('a' + radix - 10))
        nextValue = 10 + (pStr[charCount] - 'a');
      else if (pStr[charCount] >= 'A' && pStr[charCount] < ('A' + radix - 10))
        nextValue = 10 + (pStr[charCount] - 'A');

      if (nextValue >= radix)
        break;

      result = result * radix + nextValue;
    }
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
size_t udStrUtoa(char *pStr, size_t strLen, uint64_t value, int radix, size_t minChars)
{
  int upperCase = (radix < 0) ? 36 : 0;
  radix = udAbs(radix);
  if (radix < 2 || radix > 36)
    return 0;
  static const char *pLetters = "0123456789abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char buf[65]; // Accomodate the largest string which would be 64 binary characters (largest decimal is 20 characters: 18446744073709551615)
  size_t i = 0;
  while (value || (i < minChars))
  {
    buf[i++] = pLetters[value % radix + upperCase];
    value = value / radix;
  }
  if (pStr)
  {
    size_t j;
    for (j = 0; j < strLen - 1 && j < i; ++j)
    {
      pStr[j] = buf[i - j - 1];
    }
    pStr[j] = 0; // nul terminate
  }
  return i; // Return number of characters required, not necessarily number of characters written
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
size_t udStrItoa(char *pStr, size_t strLen, int32_t value, int radix, size_t minChars)
{
  if (!pStr || strLen < 2 || (radix < 2 || radix > 36))
    return 0;
  int minus = 0;
  if (radix != 2 && value < 0) // We don't do + sign for binary
  {
    minus = 1;
    *pStr = '-';
    value = -value;
  }
  return udStrUtoa(pStr + minus, strLen - minus, (uint64_t)value, radix, minChars) + minus;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
size_t udStrItoa64(char *pStr, size_t strLen, int64_t value, int radix, size_t minChars)
{
  if (!pStr || strLen < 2 || (radix < 2 || radix > 36))
    return 0;
  int minus = 0;
  if (radix != 2 && value < 0) // We don't do + sign for binary
  {
    minus = 1;
    *pStr = '-';
    value = -value;
  }
  return udStrUtoa(pStr + minus, strLen - minus, (uint64_t)value, radix, minChars) + minus;
}

// *********************************************************************
// Author: Dave Pevreal, March 2017
size_t udStrFtoa(char *pStr, size_t strLen, double value, int precision, size_t minChars)
{
  int64_t temp;
  memcpy(&temp, &value, sizeof(value));
  bool negative = temp < 0; // Only reliable way I've found to detect -0.0
                            // Apply the rounding before splitting whole and fraction so that the whole rounds if necessary
  value += pow(10.0, -precision) * (negative ? -0.5 : 0.5);
  // modf does this however it doesn't split the sign out whereas this removes the sign efficiently
  double whole = negative ? -ceil(value) : floor(value);
  double frac = negative ? -(value + whole) : value - whole;

  size_t charCount = 0;
  if (charCount < (strLen - 1) && negative)
    pStr[charCount++] = '-';
  charCount += udStrUtoa(pStr + charCount, strLen, (uint64_t)whole, 10, (size_t)udMax(1, int(minChars) - int(charCount) - (precision ? precision + 1 : 0)));
  if (charCount < (strLen - 1) && precision > 0)
  {
    pStr[charCount++] = '.';
    while (precision)
    {
      int localPrecision = udMin(precision, 16); // Asciify in small batches to avoid overflowing the whole component of the double
      frac = frac * pow(10.0, localPrecision);
      charCount += udStrItoa64(pStr + charCount, strLen - charCount, (int64_t)frac, 10, localPrecision);
      frac = frac - floor(frac); // no need for proper trunc as frac is always positive
      precision -= localPrecision;
    }
  }

  return charCount;
}

// *********************************************************************
// Author: Dave Pevreal, April 2016
size_t udStrMatchBrace(const char *pLine, char escapeChar)
{
  size_t offset;
  char matchChar;

  switch (*pLine)
  {
  case '{': matchChar = '}'; break;
  case '[': matchChar = ']'; break;
  case '(': matchChar = ')'; break;
  case '<': matchChar = '>'; break;
  case '\"': matchChar = '\"'; break;
  case '\'': matchChar = '\''; break;
  default: return udStrlen(pLine);
  }
  int depth = 1;
  for (offset = 1; pLine[offset]; ++offset)
  {
    if (pLine[offset] == escapeChar && (pLine[offset + 1] == matchChar || pLine[offset + 1] == escapeChar))
    {
      ++offset; // Skip escaped characters (only consider match or escape characters)
    }
    else if (pLine[offset] == matchChar)
    {
      if (--depth == 0)
        return offset + 1;
    }
    else if (pLine[offset] == *pLine)
    {
      ++depth;
    }
  }
  return offset;
}

// *********************************************************************
// Author: Dave Pevreal, April 2017
const char *udStrSkipWhiteSpace(const char *pLine, int *pCharCount, int *pLineNumber)
{
  int charCount = 0;
  if (pLine)
  {
    while (pLine[charCount] == ' ' || pLine[charCount] == '\t' || pLine[charCount] == '\r' || pLine[charCount] == '\n')
    {
      if (pLineNumber && pLine[charCount] == '\n')
        ++*pLineNumber;
      ++charCount;
    }
  }
  if (pCharCount)
    *pCharCount = charCount;
  return pLine + charCount;
}

// *********************************************************************
// Author: Dave Pevreal, July 2018
const char *udStrSkipToEOL(const char *pLine, int *pCharCount, int *pLineNumber)
{
  int charCount = 0;
  // Skip to NUL, \r or \n
  while (pLine[charCount] != '\0' && pLine[charCount] != '\r' && pLine[charCount] != '\n')
    ++charCount;
  // Do a check for \r\n combination
  if (pLine[charCount] == '\r' && pLine[charCount + 1] == '\n')
    ++charCount;
  // If not a NUL, skip over the EOL character (whatever it may have been)
  if (pLine[charCount] != '\0')
  {
    ++charCount;
    if (pLineNumber)
      ++*pLineNumber;
  }
  if (pCharCount)
    *pCharCount = charCount;
  return pLine + charCount;
}

// *********************************************************************
// Author: Dave Pevreal, April 2016
size_t udStrStripWhiteSpace(char *pLine)
{
  size_t len, i;
  char inQuote = 0; // Use a quote var so later we can handle single quotes if necessary
  for (len = i = 0; pLine[i]; ++i)
  {
    if (pLine[i] == inQuote)
    {
      if (pLine[i - 1] != '\\') // Handle escaped quotes in quotes
        inQuote = 0;
    }
    else
    {
      if (pLine[i] == '\"')
        inQuote = pLine[i];
    }
    if (inQuote || (pLine[i] != ' ' && pLine[i] != '\t' && pLine[i] != '\r' && pLine[i] != '\n'))
      pLine[len++] = pLine[i];
  }
  pLine[len++] = pLine[i++];
  return len;
}

// *********************************************************************
// Author: Dave Pevreal, August 2014
const char *udStrEscape(const char *pStr, const char *pCharList, bool freeOriginal)
{
  int escCharCount = 0;
  char *pEscStr = nullptr;
  size_t allocLen;
  for (const char *p = udStrchr(pStr, pCharList); p; p = udStrchr(p + 1, pCharList))
    ++escCharCount;
  if (!escCharCount && freeOriginal)
    return pStr;
  allocLen = udStrlen(pStr) + 1 + escCharCount;
  pEscStr = udAllocType(char, allocLen, udAF_None);
  if (pEscStr)
  {
    const char *pSource = pStr;
    size_t escLen = 0;
    while (*pSource)
    {
      size_t index;
      udStrchr(pSource, pCharList, &index);
      UDASSERT(escLen + index < allocLen, "AllocLen calculation wrong (1)");
      memcpy(pEscStr + escLen, pSource, index);
      escLen += index;
      if (pSource[index])
      {
        UDASSERT(escLen + 2 < allocLen, "AllocLen calculation wrong (2)");
        pEscStr[escLen++] = '\\';
        pEscStr[escLen++] = pSource[index++];
      }
      pSource += index;
    }
    UDASSERT(escLen + 1 == allocLen, "AllocLen calculation wrong (3)");
    pEscStr[escLen++] = 0;
  }
  if (freeOriginal)
    udFree(pStr);
  return pEscStr;
}

// *********************************************************************
// Author: Dave Pevreal, August 2014
float udStrAtof(const char *pStr, int *pCharCount)
{
  if (!pStr) pStr = s_udStrEmptyString;
  int charCount = 0;
  int tmpCharCount = 0;

  float negate = 1.0f;
  while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
    ++charCount;

  // Process negation separately
  if (pStr[charCount] == '-')
  {
    negate = -1.0f;
    ++charCount;
  }

  float result = (float)udStrAtoi64(pStr + charCount, &tmpCharCount);
  charCount += tmpCharCount;
  if (pStr[charCount] == '.')
  {
    ++charCount;
    int64_t fraction = udStrAtoi64(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    if (result >= 0.f)
      result += fraction / powf(10.f, (float)tmpCharCount);
    else
      result -= fraction / powf(10.f, (float)tmpCharCount);
  }
  if (pStr[charCount] == 'e' || pStr[charCount] == 'E')
  {
    ++charCount;
    float e = (float)udStrAtoi(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    result *= powf(10, e);
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result * negate;
}


// *********************************************************************
// Author: Dave Pevreal, August 2014
double udStrAtof64(const char *pStr, int *pCharCount)
{
  if (!pStr) pStr = s_udStrEmptyString;
  int charCount = 0;
  int tmpCharCount = 0;

  double negate = 1.0f;
  while (pStr[charCount] == ' ' || pStr[charCount] == '\t')
    ++charCount;

  // Process negation separately
  if (pStr[charCount] == '-')
  {
    negate = -1.0f;
    ++charCount;
  }

  double result = (double)udStrAtoi64(pStr + charCount, &tmpCharCount);
  charCount += tmpCharCount;
  if (pStr[charCount] == '.')
  {
    ++charCount;
    int64_t fraction = udStrAtoi64(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    if (result >= 0.0)
      result += fraction / pow(10.0, (double)tmpCharCount);
    else
      result -= fraction / pow(10.0, (double)tmpCharCount);
  }
  if (pStr[charCount] == 'e' || pStr[charCount] == 'E')
  {
    ++charCount;
    double e = (double)udStrAtoi64(pStr + charCount, &tmpCharCount);
    charCount += tmpCharCount;
    result *= pow(10, e);
  }
  if (pCharCount)
    *pCharCount = charCount;
  return result * negate;
}

// *********************************************************************
// Author: Dave Pevreal, March 2014
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString, bool knownUnique)
{
  uint32_t offset = 0;
  int addStrLen = (int)udStrlen(addString); // Length of string to be added

  if (knownUnique)
    offset = *pStringTableLength;
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

#define SMALLSTRING_BUFFER_COUNT 32
#define SMALLSTRING_BUFFER_SIZE 64
static char s_smallStringBuffers[SMALLSTRING_BUFFER_COUNT][SMALLSTRING_BUFFER_SIZE]; // 32 cycling buffers of 64 characters
static std::atomic<int32_t> s_smallStringBufferIndex(0);  // Cycling index, always and with (SMALLSTRING_BUFFER_COUNT-1) to get buffer index

                                                       // ****************************************************************************
                                                       // Author: Dave Pevreal, May 2018
const char *udTempStr(const char *pFormat, ...)
{
  int32_t bufIndex = (s_smallStringBufferIndex++) & (SMALLSTRING_BUFFER_COUNT - 1);
  size_t bufferSize = SMALLSTRING_BUFFER_SIZE;

retry:
  UDASSERT(bufIndex < SMALLSTRING_BUFFER_COUNT, "buffer index out of range");
  UDASSERT(bufIndex * SMALLSTRING_BUFFER_SIZE + bufferSize <= (int)sizeof(s_smallStringBuffers), "bufferSize would lead to overrun");
  char *pBuf = s_smallStringBuffers[bufIndex];
  va_list args;
  va_start(args, pFormat);
  int charCount = udSprintfVA(pBuf, bufferSize, pFormat, args);
  va_end(args);

  if (charCount >= (int)bufferSize && bufferSize < (SMALLSTRING_BUFFER_COUNT * SMALLSTRING_BUFFER_SIZE))
  {
    // The output buffer wasn't big enough, so look for a series of contiguous buffers
    // To keep things simple, attempt to undo the allocation done on the first line of the function
    int previous = bufIndex | (s_smallStringBufferIndex & ~(SMALLSTRING_BUFFER_COUNT - 1));
    // Reset to previous iff current value is exactly previous + 1
    int expectedPrev = previous + 1;
    s_smallStringBufferIndex.compare_exchange_strong(expectedPrev, previous);

    int requiredBufferCount = udMin(SMALLSTRING_BUFFER_COUNT, (charCount + SMALLSTRING_BUFFER_SIZE) / SMALLSTRING_BUFFER_SIZE);
    bufferSize = requiredBufferCount * SMALLSTRING_BUFFER_SIZE;
    // Try to allocate a number of sequential buffers, understanding that another thread can allocate one also
    while ((((bufIndex = s_smallStringBufferIndex) & (SMALLSTRING_BUFFER_COUNT - 1)) + requiredBufferCount) <= SMALLSTRING_BUFFER_COUNT)
    {
      if (s_smallStringBufferIndex.compare_exchange_strong(bufIndex, bufIndex + requiredBufferCount))
      {
        // The bufIndex has upper bits set for the compareExchange, clear them before retrying
        bufIndex &= (SMALLSTRING_BUFFER_COUNT - 1);
        goto retry;
      }
    }
    // We need to wrap and hopefully no other thread is still using their string.
    if (requiredBufferCount > (s_smallStringBufferIndex & (SMALLSTRING_BUFFER_COUNT - 1)))
      udDebugPrintf("Warning: very long string (%d chars) created using udTempStr - NOT THREADSAFE\n", charCount + 1);
    s_smallStringBufferIndex = requiredBufferCount;
    bufIndex = 0;
    goto retry;
  }
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, October 2015
const char *udTempStr_CommaInt(int64_t n)
{
  char *pBuf = s_smallStringBuffers[(s_smallStringBufferIndex++) & (SMALLSTRING_BUFFER_COUNT - 1)];
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

// ****************************************************************************
// Author: Dave Pevreal, September 2018
const char *udTempStr_TrimDouble(double v, int maxDecimalPlaces, int minDecimalPlaces, bool undoRounding)
{
  char *pBuf = s_smallStringBuffers[(s_smallStringBufferIndex++) & (SMALLSTRING_BUFFER_COUNT - 1)];
  udStrFtoa(pBuf, SMALLSTRING_BUFFER_SIZE, v, maxDecimalPlaces + (undoRounding ? 1 : 0));
  size_t pointIndex;
  if (udStrchr(pBuf, ".", &pointIndex))
  {
    size_t i = udStrlen(pBuf) - 1;
    // If requested, truncate the last decimal place (to undo rounding)
    if (undoRounding && i > pointIndex)
      pBuf[i--] = '\0';
    for (; i > (pointIndex + minDecimalPlaces); --i)
    {
      if (pBuf[i] == '0')
        pBuf[i] = '\0';
      else
        break;
    }
    if (minDecimalPlaces == 0 && pBuf[pointIndex + 1] == '\0')
      pBuf[pointIndex] = '\0';
  }
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
const char *udTempStr_ElapsedTime(int seconds, bool trimHours)
{
  int hours = seconds / (60 * 60);
  int minutes = (seconds / 60) % 60;
  int secs = seconds % 60;
  const char *pBuf = udTempStr("%d:%02d:%02d", hours, minutes, secs);
  if (trimHours && !hours)
    pBuf += 2; // Skip leading 0: when hours is zero
  return pBuf;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2018
const char *udTempStr_HumanMeasurement(double measurement)
{
  static const char *pSuffixStrings[] = { "m", "cm", "mm" };
  static double suffixMult[] = { 1, 100, 1000 };
  size_t suffixIndex = 0;
  while ((suffixIndex + 1) < UDARRAYSIZE(pSuffixStrings) && (measurement * suffixMult[suffixIndex]) < 1.0)
    ++suffixIndex;

  // Generate float scale to 6 decimal places
  char temp[32];
  size_t charCount = udStrFtoa(temp, measurement * suffixMult[suffixIndex], 6);

  // Trim unnecessary trailing zeros or decimal point for human friendly number
  while (charCount > 1)
  {
    char c = temp[--charCount];
    if (c == '0' || c == '.')
      temp[charCount] = 0;
    if (c != '0')
      break;
  }
  return udTempStr("%s%s", temp, pSuffixStrings[suffixIndex]);
}

// ****************************************************************************
// Author: Paul Fox, September 2015
int udSprintf(char *pDest, size_t destlength, const char *pFormat, ...)
{
  va_list args;
  va_start(args, pFormat);
  int length = udSprintfVA(pDest, destlength, pFormat, args);
  va_end(args);

  return length;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2017
int udSprintfVA(char *pDest, size_t destLength, const char *pFormat, va_list args)
{
#if 1
  return vsnprintf(pDest, destLength, pFormat, args);
#else
  int errorCode = 0; // -1 == unknown specifier, -2 == unexpected width/precision specifier
  size_t length = 0;
  if (!pDest || !pFormat)
    return 0;
  // Keep processing until we're out of format string or destination string
  while (*pFormat && !errorCode)
  {
    if (*pFormat == '%')
    {
      char padChar = ' ';
      bool hashSpec = false; // # prefixes hex numbers with 0x or forces point for decimals
      bool longSpec = false;
      bool leftJustify = false;
      bool forcePlus = false;
      bool widthSpec = false;
      size_t width = 0;
      int precision = 1;
      bool precisionSpec = false; // Need a flag to know precision was specified, as floats have a different default value (6)
      ++pFormat;

      while (*pFormat)
      {
        const char *pInjectStr = nullptr;
        size_t injectLen = 0;
        int charCount = 0;
        char numericBuffer[70]; // enough characters for any known number (64) plus 0x or decimal etc

        charCount = 1; // As a default unless a number is processed
                       // Process formatted string
        switch (*pFormat)
        {
        case '0': padChar = '0'; break;
        case 'l': longSpec = true; break;
        case '+': forcePlus = true; break;
        case '-': leftJustify = true; break;
        case '#': hashSpec = true; break;
        case '%': pInjectStr = pFormat; injectLen = 1; break;
        case '*': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        {
          int value = *pFormat == '*' ? udAbs(va_arg(args, int)) : udStrAtoi(pFormat, &charCount);
          if (!widthSpec)
            width = (size_t)value;
          else
            errorCode = -2;
          widthSpec = true;
        }
        break;
        case '.':
        {
          ++pFormat;
          int value = *pFormat == '*' ? udAbs(va_arg(args, int)) : udStrAtoi(pFormat, &charCount);
          precision = (size_t)value;
          precisionSpec = true;
        }
        break;
        case 'c':
        {
          numericBuffer[0] = (char)va_arg(args, int); // Chars are passed as integers
          pInjectStr = numericBuffer;
          injectLen = 1;
        }
        break;
        case 's':
        {
          pInjectStr = va_arg(args, char*);
          injectLen = udStrlen(pInjectStr);
        }
        break;
        case 'd':
        case 'i':
          if (longSpec)
            udStrItoa64(numericBuffer, va_arg(args, int64_t), 10, precision);
          else
            udStrItoa(numericBuffer, va_arg(args, uint32_t), 10, precision);
          pInjectStr = numericBuffer;
          injectLen = udStrlen(pInjectStr);
          break;
        case 'u':
          if (longSpec)
            udStrUtoa(numericBuffer, va_arg(args, uint64_t), 10, precision);
          else
            udStrUtoa(numericBuffer, va_arg(args, uint32_t), 10, precision);
          pInjectStr = numericBuffer;
          injectLen = udStrlen(pInjectStr);
          break;
        case 'x':
        case 'X':
          udStrcpy(numericBuffer, isupper(*pFormat) ? "0X" : "0x");
          if (longSpec)
            udStrUtoa(numericBuffer + 2, sizeof(numericBuffer) - 2, va_arg(args, uint64_t), isupper(*pFormat) ? -16 : 16, precision);
          else
            udStrUtoa(numericBuffer + 2, sizeof(numericBuffer) - 2, va_arg(args, uint32_t), isupper(*pFormat) ? -16 : 16, precision);
          pInjectStr = numericBuffer + ((hashSpec) ? 0 : 2);
          injectLen = udStrlen(pInjectStr);
          break;
        case 'b':
          if (longSpec)
            udStrUtoa(numericBuffer, va_arg(args, uint64_t), 2, precision);
          else
            udStrUtoa(numericBuffer, va_arg(args, uint32_t), 2, precision);
          pInjectStr = numericBuffer;
          injectLen = udStrlen(pInjectStr);
          break;
        case 'p':
        case 'P':
          if (sizeof(pInjectStr) == 8)
            udStrUtoa(numericBuffer, va_arg(args, uint64_t), isupper(*pFormat) ? -16 : 16, precision);
          else
            udStrUtoa(numericBuffer, va_arg(args, uint32_t), isupper(*pFormat) ? -16 : 16, precision);
          pInjectStr = numericBuffer;
          injectLen = udStrlen(pInjectStr);
          break;
        case 'f':
          udStrFtoa(numericBuffer, va_arg(args, double), precisionSpec ? precision : 6, padChar == ' ' ? 1 : udMax((int)width, 1));
          pInjectStr = numericBuffer;
          injectLen = udStrlen(pInjectStr);
          break;

        default:
          errorCode = -1;
        }
        pFormat += charCount;

        if (pInjectStr)
        {
          size_t padLen = (width > injectLen) ? width - injectLen : 0;
          if (padLen && !leftJustify && padChar == ' ')
          {
            for (size_t i = 0; i < padLen && (length + i) < destLength; ++i)
              pDest[length + i] = padChar;
            length += padLen;
          }
          if (injectLen)
          {
            for (size_t i = 0; i < injectLen && (length + i) < destLength; ++i)
              pDest[length + i] = pInjectStr[i];
            length += injectLen;
          }
          if (padLen && leftJustify)
          {
            for (size_t i = 0; i < padLen && (length + i) < destLength; ++i)
              pDest[length + i] = ' '; // Always use spaces for post-padding
            length += padLen;
          }
          break;
        }
      }
    }
    else
    {
      if (length < destLength)
        pDest[length] = *pFormat;
      ++length;
      ++pFormat;
    }
  }
  if (length < destLength)
    pDest[length] = '\0';
  else if (destLength > 0)
    pDest[destLength - 1] = '\0';

  return errorCode ? errorCode : (int)length;
#endif
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udSprintf(const char **ppDest, const char *pFormat, ...)
{
  size_t length;
  char *pStr = nullptr;

  va_list ap;
  va_start(ap, pFormat);
  length = udSprintfVA(nullptr, 0, pFormat, ap);
  va_end(ap);

  pStr = udAllocType(char, length + 1, udAF_None);
  if (!pStr)
    return udR_MemoryAllocationFailure;

  va_start(ap, pFormat);
  udSprintfVA(pStr, length + 1, pFormat, ap);
  va_end(ap);
  udFree(*ppDest);
  *ppDest = pStr;

  return udR_Success;
}
