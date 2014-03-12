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
inline udFileOpenFlags operator|(udFileOpenFlags a, udFileOpenFlags b) { return (udFileOpenFlags)(int(a) | int(b)); }

enum udFileSeekWhence
{
  udFSW_SeekSet = 0,
  udFSW_SeekCur = 1,
  udFSW_SeekEnd = 2,
};


// Open a file. The filename contains a prefix such as http: to access registers file handlers (see udFileHandler.h)
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags);

// Seek and read some data
udResult udFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualRead = nullptr);

// Seek and write some data
udResult udFile_SeekWrite(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset = 0, udFileSeekWhence seekWhence = udFSW_SeekCur, size_t *pActualWritten = nullptr);

// Close the file (sets the udFile pointer to null)
udResult udFile_Close(udFile **ppFile);


#endif // UDFILE_H
