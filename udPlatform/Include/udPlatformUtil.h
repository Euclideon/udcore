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

// *********************************************************************
// Helper for growing an array as required
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
    void *resized = realloc(ptr, newLength * sizeof(T));
    if (!resized)
      return false;

    ptr = (T*)resized;
    currentLength = newLength;
  }
  return true;
}

// *********************************************************************
// Count the number of bits set in a 8 bit number
// *********************************************************************
inline uint32_t udCountBits(uint8_t a_number)
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
// Initialise stringTable to NULL and stringTableLength to 0, 
// and table will be reallocated as necessary
// *********************************************************************
int udAddToStringTable(char *&stringTable, uint32_t &stringTableLength, const char *addString);


// *********************************************************************
// A helper class for dealing with filenames, no memory allocation
// *********************************************************************
class udFilename
{
public:
  // Construct either empty as default or from a path
  // No destructor (or memory allocation) to keep the object simple and copyable
  udFilename() { SetFromFullPath(nullptr); }
  udFilename(const char *path) { SetFromFullPath(path); }

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

  // Temporary function to output debug info until unit tests are done to prove reliability
  void Debug();

protected:
  void CalculateIndices();
  int m_filenameIndex;      // Index to starting character of filename
  int m_extensionIndex;     // Index to starting character of extension
  char m_path[MaxPath];         // Buffer for the path, set to 260 characters
};

#endif // UDPLATFORM_UTIL_H
