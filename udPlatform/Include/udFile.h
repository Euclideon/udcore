#ifndef UDFILE_H
#define UDFILE_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
// 
// This module is used to perform file i/o, with a number of handlers provided internally.
// The module can be extended to provide custom handlers
// 

#include "udPlatform.h"
#include "udResult.h"

struct udFile;
enum udFileOpenFlags
{
  udFOF_Read  = 1,
  udFOF_Write = 2,
  udFOF_Create = 4,
  udFOF_Multithread = 8
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

// Load an entire file, appending a nul terminator. Calls Open/SeekRead/Close internally.
udResult udFile_Load(const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes = nullptr);

// Open a file. The filename contains a prefix such as http: to access registered file handlers (see udFileHandler.h)
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes = nullptr);

// Get performance information
udResult udFile_GetPerformance(udFile *pFile, float *pKBPerSec, uint32_t *pRequestsInFlight);

// Seek and read some data
udResult udFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualRead = nullptr, udFilePipelinedRequest *pPipelinedRequest = nullptr);

// Seek and write some data
udResult udFile_SeekWrite(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualWritten = nullptr);

// Receive the data for a piped request, returning an error if attempting to receive pipelined requests out of order
udResult udFile_BlockForPipelinedRequest(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead = nullptr);

// Close the file (sets the udFile pointer to null)
udResult udFile_Close(udFile **ppFile);

// Optional handlers (optional as it requires networking libraries, WS2_32.lib on Windows platform)
udResult udFile_RegisterHTTP();
udResult udFile_RegisterNaclHTTP(void*);

#endif // UDFILE_H
