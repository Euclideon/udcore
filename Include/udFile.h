#ifndef UDFILE_H
#define UDFILE_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
//
// This module is used to perform file i/o, with a number of handlers provided internally.
// The module can be extended to provide custom handlers
// By default, a file will open with crt FILE
// The prefix raw://base64 can be used for in-memory files contained in the filename
// The prefix raw://compression=ZlibDeflate,size=123@base64 can be used for compressed in-memory files contained in the filename (see udCompressionTypeAsString)
//

#include "udPlatform.h"
#include "udResult.h"
#include "udCompression.h"

struct udFile;
enum udFileOpenFlags
{
  udFOF_Read  = 1,
  udFOF_Write = 2,
  udFOF_Create = 4,
  udFOF_Multithread = 8,
  udFOF_FastOpen = 16   // No checks performed, file length not supported. Currently functional for FILE (deferred open) and HTTP (stateless)
};
// Inline of operator to allow flags to be combined and retain type-safety
inline udFileOpenFlags operator|(udFileOpenFlags a, udFileOpenFlags b) { return (udFileOpenFlags)(int(a) | int(b)); }

enum udFileSeekWhence
{
  udFSW_SeekSet = 0,
  udFSW_SeekCur = 1,
  udFSW_SeekEnd = 2,
};

// An opaque structure to hold state for the underlying file handler to process a pipelined request
struct udFilePipelinedRequest
{
  uint64_t reserved[6];
};

// A structure to return performance info about a given file
struct udFilePerformance
{
  uint64_t throughput;
  float mbPerSec;
  int requestsInFlight;
};

// Load an entire file, appending a nul terminator. Calls Open/Read/Close internally.
udResult udFile_Load(const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes = nullptr);

template<typename T>
inline udResult udFile_Load(const char *pFilename, T **ppMemory, int64_t *pFileLengthInBytes = nullptr) { return udFile_Load(pFilename, (void**)ppMemory, pFileLengthInBytes); }

// Save an entire file, Calls Open/Write/Close internally.
udResult udFile_Save(const char *pFilename, const void *pBuffer, size_t length);

// Open a file. The filename contains a prefix such as http: to access registered file handlers (see udFileHandler.h)
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes = nullptr);

// Set a base value added to all udFSW_SeekSet positions (useful when a file is wrapped inside another file), optionally set new length
void udFile_SetSeekBase(udFile *pFile, int64_t seekBase, int64_t newLength = 0);

// For files that are achives (such as a zip file), this API specifies the subfile that is returned when reading
udResult udFile_SetSubFilename(udFile *pFile, const char *pSubFilename, int64_t *pFileLengthInBytes = nullptr);

// Set the encryption key/nonce (currently only supported on files opened for read due to alignment complexities)
udResult udFile_SetEncryption(udFile *pFile, uint8_t *pKey, int keylen, uint64_t nonce, int64_t counterOffset = 0);

// Get the filename associated with the file
const char *udFile_GetFilename(udFile *pFile);

// Get performance information
udResult udFile_GetPerformance(udFile *pFile, udFilePerformance *pPerformance);

// Seek and read some data
udResult udFile_Read(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualRead = nullptr, int64_t *pFilePos = nullptr, udFilePipelinedRequest *pPipelinedRequest = nullptr);

// Seek and write some data
udResult udFile_Write(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualWritten = nullptr, int64_t *pFilePos = nullptr);

// Receive the data for a piped request, returning an error if attempting to receive pipelined requests out of order
udResult udFile_BlockForPipelinedRequest(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead = nullptr);

// Release the underlying file handle (optional) to be re-opened upon next use - used to have more open files than internal (o/s) limits would otherwise allow
udResult udFile_Release(udFile *pFile);

// Increment the reference count on the file, requiring additional calls to udFile_Close to actually close
void udFile_AddReference(udFile *pFile);

// Close the file (sets the udFile pointer to null)
udResult udFile_Close(udFile **ppFile);

// Translate special path identifiers to correct locations ('~' to '/home/<username>' or 'C:\Users\<username>')
// (*ppNewPath) needs to be freed by the caller
udResult udFile_TranslatePath(const char **ppNewPath, const char *pPath);

// Optional handlers (optional as it requires networking libraries, WS2_32.lib on Windows platform)
udResult udFile_RegisterHTTP();

// Helper function to output a raw filename for a given buffer to ppResultFilename, or debug output if ppResultFilename is null (line-breaking at charsPerLine characters)
udResult udFile_GenerateRawFilename(const char **ppResultFilename, const void *pBuffer, size_t bufferLen, udCompressionType ct = udCT_None, const char *pOriginalFilename = nullptr, size_t allocationSize = 0, uint32_t charsPerLine = 64);

// Helper to return the base 64 text offset from a Raw filename, plus optionally the original filename (caller to free) and size/compression info if present in filename string
bool udFile_IsRaw(const char *pFilename, size_t *pOffsetToBase64 = nullptr, const char **ppOriginalFilename = nullptr, size_t *pSize = nullptr, udCompressionType *pCompressionType = nullptr, size_t *pAllocationSize = nullptr);

#endif // UDFILE_H
