#ifndef UDPLATFORM_UTIL_H
#define UDPLATFORM_UTIL_H

#include "udPlatform.h"
#include <stdio.h>

// *********************************************************************
// Some simple utility template functions
// *********************************************************************

template <typename SourceType, typename DestType>
inline DestType udCastToTypeOf(const SourceType &source, const DestType &) { return DestType(source); }

// A utility macro to isolate a bit-field style value from an integer, using an enumerate convention
#define udBitFieldGet(composite, id)        (((composite) >> id##Shift) & id##Mask)                                       // Retrieve a field from a composite
#define udBitFieldClear(composite, id)      ((composite) & ~(udCastToTypeOf(id##Mask, composite) << id##Shift)            // Clear a field, generally used before updating just one field of a composite
#define udBitFieldSet(composite, id, value) ((composite) | ((udCastToTypeOf(value, composite) & id##Mask) << id##Shift))  // NOTE! Field should be cleared first, most common case is building a composite from zero

template <typename T>
T *udAddBytes(T *ptr, ptrdiff_t bytes) { return (T*)(bytes + (char*)ptr); }

// Template to read from a pointer, does a hard cast to allow reading into const char arrays
template <typename T, typename P>
inline udResult udReadFromPointer(T *pDest, P *&pSrc, int *pBytesRemaining = nullptr, int arrayCount = 1)
{
  int bytesRequired = (int)sizeof(T) * arrayCount;
  if (pBytesRemaining)
  {
    if (*pBytesRemaining < bytesRequired)
      return udR_CountExceeded;
    *pBytesRemaining -= bytesRequired;
  }
  memcpy((void*)pDest, pSrc, bytesRequired);
  pSrc = udAddBytes(pSrc, bytesRequired);
  return udR_Success;
}
// Template to write to a pointer, the complementary function to udReadFromPointer
template <typename T, typename P>
inline udResult udWriteToPointer(T *pSrc, P *&pDest, int *pBytesRemaining = nullptr, int arrayCount = 1)
{
  int bytesRequired = (int)sizeof(T) * arrayCount;
  if (pBytesRemaining)
  {
    if (*pBytesRemaining < bytesRequired)
      return udR_CountExceeded;
    *pBytesRemaining -= bytesRequired;
  }
  memcpy((void*)pDest, pSrc, bytesRequired);
  pDest = udAddBytes(pDest, bytesRequired);
  return udR_Success;
}

// *********************************************************************
// Time and timing
// *********************************************************************
uint32_t udGetTimeMs(); // Get a millisecond-resolution timer that is thread-independent - timeGetTime() on windows
uint64_t udPerfCounterStart(); // Get a starting point value for now (thread dependent)
float udPerfCounterMilliseconds(uint64_t startValue, uint64_t end = 0); // Get elapsed time since previous start (end value is "now" by default)


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
size_t udStrcpy(char *dest, size_t destLen, const char *src);
size_t udStrncpy(char *dest, size_t destLen, const char *src, size_t maxChars);
size_t udStrcat(char *dest, size_t destLen, const char *src);
size_t udStrlen(const char *str);


// *********************************************************************
// String maniplulation functions, NULL-safe
// *********************************************************************
// udStrdup behaves much like strdup, optionally allocating additional characters
char *udStrdup(const char *s, size_t additionalChars = 0);
// udStrchr behaves much like strchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrchr(const char *s, const char *pCharList, size_t *pIndex = nullptr);
// udStrrchr behaves much like strrchr, optionally also providing the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrrchr(const char *s, const char *pCharList, size_t *pIndex = nullptr);
// udStrstr behaves much like strstr, though the length of s can be supplied for safety
// (zero indicates assume nul-terminated). Optionally the function can provide the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrstr(const char *s, size_t sLen, const char *pSubString, size_t *pIndex = nullptr);
// udStrAtoi behaves much like atoi, optionally giving the number of characters parsed
// and the radix can be supplied to parse hex(16) or binary(2) numbers
int32_t udStrAtoi(const char *s, int *pCharCount = nullptr, int radix = 10);
int64_t udStrAtoi64(const char *s, int *pCharCount = nullptr, int radix = 10);
// udStrAtof behaves much like atol, but much faster and optionally gives the number of characters parsed
float udStrAtof(const char *s, int *pCharCount = nullptr);
double udStrAtof64(const char *s, int *pCharCount = nullptr);
// Split a line into an array of tokens
int udTokenSplit(char *pLine, const char *pDelimiters, char **ppTokens, int maxTokens);

// *********************************************************************
// String comparison functions that can be relied upon, NULL-safe
// *********************************************************************
int udStrcmp(const char *s1, const char *s2);
int udStrcmpi(const char *s1, const char *s2);
bool udStrBeginsWith(const char *s, const char *prefix);
inline bool udStrEqual(const char *s1, const char *s2) { return udStrcmp(s1, s2) == 0; }

#if UDPLATFORM_WINDOWS
// *********************************************************************
// Helper to convert a UTF8 string to wide char for Windows OS calls
// *********************************************************************
class udOSString
{
public:
  udOSString(const char *pString);
  udOSString(const wchar_t *pString);
  ~udOSString();

  operator const wchar_t *() { return m_pWide; }
  operator const char *() { return m_pUTF8; }
  void *m_pAllocation;
  wchar_t *m_pWide;
  char *m_pUTF8;
};
#else
# define udOSString(p) p
#endif

// *********************************************************************
// Helper for growing an array as required TODO: Use configured memory allocators
// *********************************************************************
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
    void *resized = udRealloc(ptr, newLength * sizeof(T));
    if (!resized)
      return false;

    ptr = (T*)resized;
    currentLength = newLength;
  }
  return true;
}

// *********************************************************************
// Count the number of bits set
// *********************************************************************

inline uint32_t udCountBits32(uint32_t v)
{
  v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

inline uint32_t udCountBits64(uint64_t v)
{
  return udCountBits32((uint32_t)v) + udCountBits32((uint32_t)(v >> 32));
}

inline uint32_t udCountBits8(uint8_t a_number)
{
  static const uint8_t bits[256] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8 };
  return bits[a_number];
}


// *********************************************************************
// Create (or optionally update) a standard 32-bit CRC
uint32_t udCrc(const void *pBuffer, size_t length, uint32_t updateCrc = 0);

// *********************************************************************
// Simple base64 decoder, output can be same memory as input
// Pass zero for outputLength to count output bytes without actually writing
// Returns actual number of bytes after decoding
size_t udDecodeBase64(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength);

// *********************************************************************
// Add a string to a dynamic table of unique strings.
// Initialise pStringTable to NULL and stringTableLength to 0,
// and table will be reallocated as necessary
// *********************************************************************
int udAddToStringTable(char *&pStringTable, uint32_t *pStringTableLength, const char *addString);


// *********************************************************************
// Threading and concurrency
// *********************************************************************
int udGetHardwareThreadCount();


// *********************************************************************
// A helper class for dealing with filenames, no memory allocation
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udFilename
{
public:
  // Construct either empty as default or from a path
  // No destructor (or memory allocation) to keep the object simple and copyable
  udFilename() { Construct(); }
  udFilename(const char *path) { SetFromFullPath(path); }
  void Construct() { SetFromFullPath(NULL); }

  enum { MaxPath = 260 };

  //Cast operator for convenience
  operator const char *() { return m_path; }

  //
  // Set methods: (set all or part of the internal fully pathed filename, return false if path too long)
  //    FromFullPath      - Set all components from a fully pathed filename
  //    Folder            - Set the folder, retaining the existing filename and extension
  //    SetFilenameNoExt  - Set the filename without extension, retaining the existing folder and extension
  //    SetExtension      - Set the extension, retaining the folder and filename
  //
  bool SetFromFullPath(const char *fullPath);
  bool SetFolder(const char *folder);
  bool SetFilenameNoExt(const char *filenameOnlyComponent);
  bool SetFilenameWithExt(const char *filenameWithExtension);
  bool SetExtension(const char *extComponent);

  //
  // Get methods: (all return a const pointer to the internal fully pathed filename)
  //    GetPath             - Get the complete path (eg pass to fopen)
  //    GetFilenameWithExt  - Get the filename and extension, without the path
  //    GetExt              - Get just the extension (starting with the dot)
  const char *GetPath() const            { return m_path; }
  const char *GetFilenameWithExt() const { return m_path + m_filenameIndex; }
  const char *GetExt() const             { return m_path + m_extensionIndex; }

  //
  // Extract methods: (take portions from within the full path to a user supplied buffer, returning size required)
  //    ExtractFolder       - Extract the folder, including the trailing slash
  //    ExtractFilenameOnly - Extract the filename without extension
  //
  int ExtractFolder(char *folder, int folderLen);
  int ExtractFilenameOnly(char *filename, int filenameLen);

  //
  // Test methods: to determine what is present in the filename
  bool HasFilename() const              { return m_path[m_filenameIndex] != 0; }
  bool HasExt() const                   { return m_path[m_extensionIndex] != 0; }

  // Temporary function to output debug info until unit tests are done to prove reliability
  void Debug();

protected:
  void CalculateIndices();
  int m_filenameIndex;      // Index to starting character of filename
  int m_extensionIndex;     // Index to starting character of extension
  char m_path[MaxPath];         // Buffer for the path, set to 260 characters
};


// *********************************************************************
// A helper class for dealing with filenames, allocates memory due to unknown maximum length of url
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udURL
{
public:
  udURL(const char *pURLText = nullptr) { Construct(); SetURL(pURLText); }
  ~udURL() { SetURL(nullptr); }
  void Construct() { m_pURLText = nullptr; }

  udResult SetURL(const char *pURLText);
  const char *GetScheme()         { return m_pScheme; }
  const char *GetDomain()         { return m_pDomain; }
  int GetPort()                   { return m_port; }
  const char *GetPathWithQuery()  { return m_pPath; }

protected:
  char *m_pURLText;
  const char *m_pScheme;
  const char *m_pDomain;
  const char *m_pPath;
  int m_port;
};


// Load or Save a BMP
udResult udSaveBMP(const char *pFilename, int width, int height, uint32_t *pColorData, int pitch = 0);
udResult udLoadBMP(const char *pFilename, int *pWidth, int *pHeight, uint32_t **pColorData);

// Directory iteration for OS file system only
struct udFindDir
{
  const char *pFilename;
  bool isDirectory;
};

// Test for existence of a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes = nullptr);

// Delete a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileDelete(const char *pFilename);

// Open a folder for reading
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder);

// Read next entry in the folder
udResult udReadDir(udFindDir *pFindDir);

// Free resources associated with the directory
udResult udCloseDir(udFindDir **ppFindDir);

// Write a formatted string to the buffer
int udSprintf(char *pDest, size_t destlength, const char *format, ...);

#endif // UDPLATFORM_UTIL_H
