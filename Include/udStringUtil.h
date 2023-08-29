#ifndef UDSTRINGUTIL_H
#define UDSTRINGUTIL_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, September 2013
//
// C-String helpers
//

#include "udPlatform.h"
#include <stdarg.h>

// *********************************************************************
// Some string functions that are safe,
//  - on success all return the length of the result in characters
//    (including null) which is always greater than zero
//  - on overflow a zero returned, also note that
//    a function designed to overwrite the destination (eg udStrcpy)
//    will clear the destination. Functions designed to work
//    with a valid destination (eg udStrcat) will leave the
//    the destination unaltered.
// *********************************************************************
size_t udStrcpy(char *pDest, size_t destLen, const char *pSrc);
size_t udStrncpy(char *pDest, size_t destLen, const char *pSrc, size_t maxChars);
size_t udStrcat(char *pDest, size_t destLen, const char *pSrc);
size_t udStrlen(const char *pStr);

// *********************************************************************
// Helper functions for fixed size strings
// *********************************************************************
template <size_t N> inline size_t udStrcpy(char(&dest)[N], const char *pSrc) { return udStrcpy(dest, N, pSrc); }
template <size_t N> inline size_t udStrncpy(char(&dest)[N], const char *pSrc, size_t maxChars) { return udStrncpy(dest, N, pSrc, maxChars); }
template <size_t N> inline size_t udStrcat(char(&dest)[N], const char *pSrc) { return udStrcat(dest, N, pSrc); }

// *********************************************************************
// String maniplulation functions, NULL-safe
// *********************************************************************
// udStrdup behaves much like strdup, optionally allocating additional characters and replicating NULL
char *udStrdup(const char *pStr, size_t additionalChars = 0, const std::source_location location = MEMORY_DEBUG_LOCATION());
// udStrndup behaves much like strndup, optionally allocating additional characters and replicating NULL
char *udStrndup(const char *pStr, size_t maxChars, size_t additionalChars = 0, const std::source_location location = MEMORY_DEBUG_LOCATION());
// udStrchr behaves much like strchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrchr(const char *pStr, const char *pCharList, size_t *pIndex = nullptr, size_t *pCharListIndex = nullptr);
const char *udStrchri(const char *pStr, const char *pCharList, size_t *pIndex = nullptr, size_t *pCharListIndex = nullptr);
// udStrrchr behaves much like strrchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrrchr(const char *pStr, const char *pCharList, size_t *pIndex = nullptr, size_t *pCharListIndex = nullptr);
const char *udStrrchri(const char *pStr, const char *pCharList, size_t *pIndex = nullptr, size_t *pCharListIndex = nullptr);
// udStrstr behaves much like strstr, though the length of s can be supplied for safety
// (zero indicates assume nul-terminated). Optionally the function can provide the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrstr(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex = nullptr);
const char *udStrstri(const char *pStr, size_t sLen, const char *pSubString, size_t *pIndex = nullptr);
// udStrAtoi behaves much like atoi, optionally giving the number of characters parsed
// and the radix of 2 - 36 can be supplied to parse numbers such as hex(16) or binary(2)
// No overflow testing is performed at this time
int32_t udStrAtoi(const char *pStr, int *pCharCount = nullptr, int radix = 10);
uint32_t udStrAtou(const char *pStr, int *pCharCount = nullptr, int radix = 10);
int64_t udStrAtoi64(const char *pStr, int *pCharCount = nullptr, int radix = 10);
uint64_t udStrAtou64(const char *pStr, int *pCharCount = nullptr, int radix = 10);
// udStrAtof behaves much like atol, but much faster and optionally gives the number of characters parsed
float udStrAtof(const char *pStr, int *pCharCount = nullptr);
double udStrAtof64(const char *pStr, int *pCharCount = nullptr);
// udStr*toa convert numbers to ascii, returning the number of characters required
size_t udStrUtoa(char *pStr, size_t strLen, uint64_t value, int radix = 10, size_t minChars = 1);
size_t udStrItoa(char *pStr, size_t strLen, int32_t value, int radix = 10, size_t minChars = 1);
size_t udStrItoa64(char *pStr, size_t strLen, int64_t value, int radix = 10, size_t minChars = 1);
size_t udStrFtoa(char *pStr, size_t strLen, double value, int precision, size_t minChars = 1);
// udStr*toa helpers for fixed size strings
template <size_t N> inline size_t udStrUtoa(char(&str)[N], uint64_t value, int radix = 10, size_t minChars = 1) { return udStrUtoa(str, N, value, radix, minChars); }
template <size_t N> inline size_t udStrItoa(char(&str)[N], int32_t value, int radix = 10, size_t minChars = 1) { return udStrItoa(str, N, value, radix, minChars); }
template <size_t N> inline size_t udStrItoa64(char(&str)[N], int64_t value, int radix = 10, size_t minChars = 1) { return udStrItoa64(str, N, value, radix, minChars); }
template <size_t N> inline size_t udStrFtoa(char(&str)[N], double value, int precision, size_t minChars = 1) { return udStrFtoa(str, N, value, precision, minChars); }

// Split a line into an array of tokens
int udStrTokenSplit(char *pLine, const char *pDelimiters, char *pTokenArray[], int maxTokens);
// Find the offset of the character FOLLOWING the matching brace character pointed to by pLine
// (may point to null terminator if not found). Brace characters: ({[<"' matched with )}]>"'
size_t udStrMatchBrace(const char *pLine, char escapeChar = 0);
// Helper to skip white space (space,8,10,13), optionally incrementing a line number appropriately
const char *udStrSkipWhiteSpace(const char *pLine, int *pCharCount = nullptr, int *pLineNumber = nullptr);
// Helper to skip to the end of line or null character, optionally incrementing a line number appropriately
const char *udStrSkipToEOL(const char *pLine, int *pCharCount = nullptr, int *pLineNumber = nullptr);
// In-place remove all non-quoted white space (newlines, spaces, tabs), returns new length
size_t udStrStripWhiteSpace(char *pLine);
// Return new string with \ char before any chars in pCharList, optionally freeing pStr
const char *udStrEscape(const char *pStr, const char *pCharList, bool freeOriginal);

// *********************************************************************
// String comparison functions that can be relied upon, NULL-safe
// *********************************************************************
int udStrcmp(const char *pStr1, const char *pStr2);
int udStrcmpi(const char *pStr1, const char *pStr2);
int udStrncmp(const char *pStr1, const char *pStr2, size_t maxChars);
int udStrncmpi(const char *pStr1, const char *pStr2, size_t maxChars);
bool udStrBeginsWith(const char *pStr, const char *pPrefix);
bool udStrBeginsWithi(const char *pStr, const char *pPrefix);
bool udStrEndsWith(const char *pStr, const char *pSuffix);
bool udStrEndsWithi(const char *pStr, const char *pSuffix);
inline bool udStrEqual(const char *pStr1, const char *pStr2) { return udStrcmp(pStr1, pStr2) == 0; }
inline bool udStrEquali(const char *pStr1, const char *pStr2) { return udStrcmpi(pStr1, pStr2) == 0; }

// *********************************************************************
// Add a string to a dynamic table of unique strings.
// Initialise pStringTable to NULL and stringTableLength to 0,
// and table will be reallocated as necessary
// *********************************************************************
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString, bool knownUnique = false);

// *********************************************************************
// Some helper functions that make use of internal cycling buffers for convenience (threadsafe)
// The caller has the responsibility not to hold the pointer or do stupid
// things with these functions. They are generally useful for passing a
// string directly to a function, the buffer can be overwritten.
// Do not assume the buffer can be used beyond the string null terminator
// *********************************************************************

// Create a temporary small string using one of a number of cycling static buffers
UD_PRINTF_FORMAT_FUNC(1) const char *udTempStr(const char *pFormat, ...);

// Give back pretty version (ie with commas) of an int using one of a number of cycling static buffers
const char *udTempStr_CommaInt(int64_t n);
inline const char *udCommaInt(int64_t n) { return udTempStr_CommaInt(n); } // DEPRECATED NAME

// Give back a double whose trailing zeroes are trimmed if possible (0 will trim decimal point also if possible)
const char *udTempStr_TrimDouble(double v, int maxDecimalPlaces, int minDecimalPlaces = 0, bool undoRounding = false);

// Give back a H:MM:SS format string, optionally trimming to MM:SS if hours is zero using one of a number of cycling static buffers
const char *udTempStr_ElapsedTime(int seconds, bool trimHours = true);
inline const char *udSecondsToString(int seconds, bool trimHours = true) { return udTempStr_ElapsedTime(seconds, trimHours); } // DEPRECATED NAME

// Return a human readable measurement string such as 1cm for 0.01, 2mm for 0.002 etc using one of a number of cycling static buffers
const char *udTempStr_HumanMeasurement(double measurement);


// Write a formatted string to the buffer
UD_PRINTF_FORMAT_FUNC(3) int udSprintf(char *pDest, size_t destlength, const char *pFormat, ...);
int udSprintfVA(char *pDest, size_t destlength, const char *pFormat, va_list args);
// Create an allocated string to fit the output, *ppDest will be freed if non-null, and may be an argument to the string
UD_PRINTF_FORMAT_FUNC(2) udResult udSprintf(const char **ppDest, const char *pFormat, ...);
// Helper functions for fixed size arrays
template <size_t N> UD_PRINTF_FORMAT_FUNC(2) inline int udSprintf(char(&dest)[N], const char *pFormat, ...) { va_list args; va_start(args, pFormat); int length = udSprintfVA(dest, N, pFormat, args); va_end(args); return length; }
template <size_t N> inline int udSprintfVA(char(&dest)[N], const char *pFormat, va_list args) { return udSprintfVA(dest, N, pFormat, args); }

#endif // UDSTRINGUTIL_H
