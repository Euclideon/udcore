#ifndef UDPLATFORM_UTIL_H
#define UDPLATFORM_UTIL_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, September 2013
//
// Utility functions and helpers
//

#include "udPlatform.h"
#include <stdarg.h>
#include <stdio.h>

class udJSON;

#define UDNAMEASSTRING(s) #s
#define UDSTRINGIFY(s)    UDNAMEASSTRING(s)

// *********************************************************************
// Some simple utility template functions
// *********************************************************************

template <typename SourceType, typename DestType>
inline DestType udCastToTypeOf(const SourceType &source, const DestType &) { return DestType(source); }

// A utility macro to isolate a bit-field style value from an integer, using an enumerate convention
#define udBitFieldGet(composite, id) (((composite) >> id##Shift) & id##Mask)                                              // Retrieve a field from a composite
#define udBitFieldClear(composite, id)      ((composite) & ~(udCastToTypeOf(id##Mask, composite) << id##Shift)            // Clear a field, generally used before updating just one field of a composite
#define udBitFieldSet(composite, id, value) ((composite) | ((udCastToTypeOf(value, composite) & id##Mask) << id##Shift))  // NOTE! Field should be cleared first, most common case is building a composite from zero

template <typename T>
T *udAddBytes(T *ptr, ptrdiff_t bytes)
{
  return (T *)(bytes + (char *)ptr);
}

template <typename T, typename P>
inline void udAssignUnaligned(P *ptr, T value)
{
#if UDPLATFORM_EMSCRIPTEN
  memcpy(ptr, &value, sizeof(value));
#else
  *(T *)ptr = value;
#endif
}

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
  memcpy((void *)pDest, pSrc, bytesRequired);
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
  memcpy((void *)pDest, pSrc, bytesRequired);
  pDest = udAddBytes(pDest, bytesRequired);
  return udR_Success;
}
template <typename T, typename P>
inline udResult udWriteValueToPointer(T value, P *&pDest, int *pBytesRemaining = nullptr)
{
  return udWriteToPointer(&value, pDest, pBytesRemaining);
}

// *********************************************************************
// Camera helpers
// *********************************************************************
// Update in-place a camera matrix given yaw, pitch and translation parameters
void udUpdateCamera(double camera[16], double yawRadians, double pitchRadians, double tx, double ty, double tz);
void udUpdateCamera(float camera[16], float yawRadians, float pitchRadians, float tx, float ty, float tz);

// *********************************************************************
// Time and timing
// *********************************************************************
uint32_t udGetTimeMs();                                                 // Get a millisecond-resolution timer that is thread-independent - timeGetTime() on windows
uint64_t udPerfCounterStart();                                          // Get a starting point value for now (thread dependent)
float udPerfCounterMilliseconds(uint64_t startValue, uint64_t end = 0); // Get elapsed time since previous start (end value is "now" by default)
float udPerfCounterSeconds(uint64_t startValue, uint64_t end = 0);      // Get elapsed time since previous start (end value is "now" by default)
int udDaysUntilExpired(int maxDays, const char **ppExpireDateStr);      // Return the number of days until the build expires

int64_t udGetEpochSecsUTCd();
double udGetEpochSecsUTCf();

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

  operator const wchar_t *() { return pWide; }
  operator const char *() { return pUTF8; }
  void *pAllocation;
  wchar_t *pWide;
  char *pUTF8;
};
#else
#  define udOSString(p) p
#endif

// *********************************************************************
// Helper for growing an array as required TODO: Use configured memory allocators
// *********************************************************************
template <typename T>
bool udSizeArray(T *&ptr, size_t &currentLength, size_t requiredLength, size_t allocationMultiples = 0)
{
  if (requiredLength > currentLength || allocationMultiples == 0)
  {
    size_t newLength;
    if (allocationMultiples > 1)
      newLength = ((requiredLength + allocationMultiples - 1) / allocationMultiples) * allocationMultiples;
    else
      newLength = requiredLength;
    void *resized = udRealloc(ptr, newLength * sizeof(T));
    if (!resized)
      return false;

    ptr = (T *)resized;
    currentLength = newLength;
  }
  return true;
}

// *********************************************************************
// Count the number of bits set
// *********************************************************************

inline uint32_t udCountBits32(uint32_t v)
{
  v = v - ((v >> 1) & 0x55555555);                         // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);          // temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

inline uint32_t udCountBits64(uint64_t v)
{
  return udCountBits32((uint32_t)v) + udCountBits32((uint32_t)(v >> 32));
}

inline uint32_t udCountBits8(uint8_t a_number)
{
  static const uint8_t bits[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                                     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                     1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                     2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                     3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                     4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };
  return bits[a_number];
}

// *********************************************************************
// Create (or optionally update) a standard 32-bit CRC (polynomial 0xedb88320)
uint32_t udCrc(const void *pBuffer, size_t length, uint32_t updateCrc = 0);

// Create (or optionally update) a iSCSI standard 32-bit CRC (polynomial 0x1edc6f41)
uint32_t udCrc32c(const void *pBuffer, size_t length, uint32_t updateCrc = 0);

// *********************************************************************
// Simple base64 decoder, output can be same memory as input
// Pass nullptr for pOutput to count output bytes
udResult udBase64Decode(const char *pString, size_t length, uint8_t *pOutput, size_t outputLength, size_t *pOutputLengthWritten = nullptr);
udResult udBase64Decode(uint8_t **ppOutput, size_t *pOutputLength, const char *pString);

// *********************************************************************
// Simple base64 encoder, unlike decode output CANNOT be the same memory as input
// encodes binaryLength bytes from pBinary to strLength characters in pString
// Pass nullptr for pString to count output bytes without actually writing
udResult udBase64Encode(const void *pBinary, size_t binaryLength, char *pString, size_t strLength, size_t *pOutputLengthWritten = nullptr);
udResult udBase64Encode(const char **ppDestStr, const void *pBinary, size_t binaryLength);

// *********************************************************************
// Threading and concurrency
// *********************************************************************
int udGetHardwareThreadCount();

// *********************************************************************
// A helper class for dealing with filenames
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udFilename
{
public:
  // Construct either empty as default or from a path
  udFilename() : pPath(path) { Construct(); }
  udFilename(const char *path) : pPath(this->path) { SetFromFullPath("%s", path); }
  ~udFilename() { Destruct(); }

  udFilename(const udFilename &o);
  udFilename &operator=(const udFilename &o);
  udFilename(udFilename &&o) noexcept;
  udFilename &operator=(udFilename &&o) noexcept;
  void Construct() { SetFromFullPath(NULL); }
  void Destruct()
  {
    if (pPath != path)
      udFree(pPath);
  }

  enum
  {
    MaxPath = 260
  };

  //Cast operator for convenience
  operator const char *() { return pPath; }

  //
  // Set methods: (set all or part of the internal fully pathed filename, return false if path too long)
  //    FromFullPath      - Set all components from a fully pathed filename
  //    Folder            - Set the folder, retaining the existing filename and extension
  //    SetFilenameNoExt  - Set the filename without extension, retaining the existing folder and extension
  //    SetExtension      - Set the extension, retaining the folder and filename
  //
  UD_PRINTF_FORMAT_FUNC(2)
  bool SetFromFullPath(const char *pFormat, ...);
  bool SetFolder(const char *folder);
  bool SetFilenameNoExt(const char *filenameOnlyComponent);
  bool SetFilenameWithExt(const char *filenameWithExtension);
  bool SetExtension(const char *extComponent);

  //
  // Get methods: (all return a const pointer to the internal fully pathed filename)
  //    GetPath             - Get the complete path (eg pass to fopen)
  //    GetFilenameWithExt  - Get the filename and extension, without the path
  //    GetExt              - Get just the extension (starting with the dot)
  const char *GetPath() const { return pPath; }
  const char *GetFilenameWithExt() const { return pPath + filenameIndex; }
  const char *GetExt() const { return pPath + extensionIndex; }

  //
  // Extract methods: (take portions from within the full path to a user supplied buffer, returning size required)
  //    ExtractFolder       - Extract the folder, including the trailing slash
  //    ExtractFilenameOnly - Extract the filename without extension
  //
  int ExtractFolder(char *folder, int folderLen);
  int ExtractFilenameOnly(char *filename, int filenameLen);

  //
  // Test methods: to determine what is present in the filename
  bool HasFilename() const { return pPath[filenameIndex] != 0; }
  bool HasExt() const { return pPath[extensionIndex] != 0; }

  // Temporary function to output debug info until unit tests are done to prove reliability
  void Debug();

protected:
  void CalculateIndices();
  int filenameIndex;  // Index to starting character of filename
  int extensionIndex; // Index to starting character of extension
  char path[MaxPath]; // Buffer for the path, set to 260 characters
  char *pPath;        // Pointer to actual path, may point to path
};

// *********************************************************************
// A helper class for dealing with filenames, allocates memory due to unknown maximum length of url
// If allocated rather than created with new, call Construct method
// *********************************************************************
class udURL
{
public:
  udURL(const char *pURLText = nullptr)
  {
    Construct();
    SetURL(pURLText);
  }
  ~udURL() { SetURL(nullptr); }
  void Construct() { pURLText = nullptr; }

  udResult SetURL(const char *pURLText);
  const char *GetScheme() { return pScheme; }
  const char *GetDomain() { return pDomain; }
  int GetPort() { return port; }
  const char *GetPathWithQuery() { return pPath; }

protected:
  char *pURLText;
  const char *pScheme;
  const char *pDomain;
  const char *pPath;
  int port;
};

// Load or Save a BMP
udResult udSaveBMP(const char *pFilename, int width, int height, uint32_t *pColorData, int pitch = 0);
udResult udLoadBMP(const char *pFilename, int *pWidth, int *pHeight, uint32_t **pColorData);

// *********************************************************************
// Directory iteration for OS file system only
struct udFindDir
{
  const char *pFilename;
  bool isDirectory;
};

// Test for existence of a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileExists(const char *pFilename, int64_t *pFileLengthInBytes = nullptr, int64_t *pModifiedTime = nullptr);

// Delete a file, on OS FILESYSTEM only (not registered file handlers)
udResult udFileDelete(const char *pFilename);

// Open a folder for reading
udResult udOpenDir(udFindDir **ppFindDir, const char *pFolder);

// Read next entry in the folder
udResult udReadDir(udFindDir *pFindDir);

// Free resources associated with the directory
udResult udCloseDir(udFindDir **ppFindDir);

// Create a directory, potentially creating other directories in the path leading to it
udResult udCreateDir(const char *pDirPath, int *pDirsCreatedCount = nullptr);

// Removes a folder and a number of sub-folders (pass count returned by udCreateDir to undo; passing zero will not remove the folder)
udResult udRemoveDir(const char *pDirPath, int removeCount = 1);

// *********************************************************************
// Geospatial helper functions
// *********************************************************************

// Parse a WKT string to a udJSON, each WKT element is parsed as an object
// with element 'type' defining the type string, if the first parameter of
// the element is a string it is assigned to the element 'name', all other
// parameters are stored in an array element 'values'
udResult udParseWKT(udJSON *pValue, const char *pWKTString, int *pCharCount = nullptr);

// Export a previously parsed WKT back to WKT format
udResult udExportWKT(const char **ppOutput, const udJSON *pValue);

#endif // UDPLATFORM_UTIL_H
