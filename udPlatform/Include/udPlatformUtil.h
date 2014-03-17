#ifndef UDPLATFORM_UTIL_H
#define UDPLATFORM_UTIL_H

#include "udPlatform.h"

// *********************************************************************
// Some simple utility template functions
// *********************************************************************

template <typename T>
inline T udMax(const T &a, const T &b) { return (a > b) ? a : b;  }

template <typename T>
inline T udMin(const T &a, const T &b) { return (a < b) ? a : b;  }

template <typename T>
inline T udClamp(const T &a, const T &minVal, const T &maxVal) { return udMin(udMax(a, minVal), maxVal);  }

template <typename T>
inline T udRoundPow2(T n, int align) { return ((n + (align-1)) & -align); }


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
// udStrstr behaves much like strstr, though the length of s can be supplied for safety
// (zero indicates assume nul-terminated). Optionally the function can provide the index
// of the find, which will be the length if not found (ie when null is returned)
const char *udStrstr(const char *s, size_t sLen, const char *pSubString, size_t *pIndex = nullptr);
// udAtoi behaves much like atoi, optionally giving the number of characters parsed
// and the radix can be supplied to parse hex(16) or binary(2) numbers
int32_t udStrAtoi(const char *s, size_t *pCharCount = nullptr, int radix = 10);
// udAtoi behaves much like atol, optionally giving the number of characters parsed
// and the radix can be supplied to parse hex(16) or binary(2) numbers
int64_t udStrAtoi64(const char *s, size_t *pCharCount = nullptr, int radix = 10);

// *********************************************************************
// String comparison functions that can be relied upon, NULL-safe
// *********************************************************************
int udStrcmp(const char *s1, const char *s2);
bool udStrBeginsWith(const char *s, const char *prefix);
inline bool udStrEqual(const char *s1, const char *s2) { return udStrcmp(s1, s2) == 0; }

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
// Add a string to a dynamic table of unique strings. 
// Initialise pStringTable to NULL and stringTableLength to 0, 
// and table will be reallocated as necessary
// *********************************************************************
int udAddToStringTable(char *&pStringTable, uint32_t &stringTableLength, const char *addString);


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
  const char *GetPath()                 { return m_path; }
  const char *GetFilenameWithExt()      { return m_path + m_filenameIndex; }
  const char *GetExt()                  { return m_path + m_extensionIndex; }

  //
  // Extract methods: (take portions from within the full path to a user supplied buffer, returning size required)
  //    ExtractFolder       - Extract the folder, including the trailing slash
  //    ExtractFilenameOnly - Extract the filename without extension
  //
  int ExtractFolder(char *folder, int folderLen);
  int ExtractFilenameOnly(char *filename, int filenameLen);

  //
  // Test methods: to determine what is present in the filename
  bool HasFilename()                    { return m_path[m_filenameIndex] != 0; }
  bool HasExt()                         { return m_path[m_extensionIndex] != 0; }

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

#define UDARRAYSIZE(_array) ( sizeof(_array) / sizeof(_array[0]) )

#endif // UDPLATFORM_UTIL_H
