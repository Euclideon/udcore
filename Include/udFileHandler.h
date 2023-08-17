#ifndef UDFILEHANDLER_H
#define UDFILEHANDLER_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal
//
// This module allows new file handlers to be registered with the system, and is used
// internally to provide file i/o to direct file system, http and other future protocols
//

#include "udPlatform.h"
#include "udFile.h"
#include "udThread.h"

// Some function prototypes required if implementing a custom file handler

// The OpenHandler is responsible for allocating a derivative of udFile (generally a structure inherited from it) and writing
// any state required for SeekRead/SeekWrite/Close to function. The fpWrite can be null if writing is not supported.
typedef udResult udFile_OpenHandlerFunc(udFile **ppFile, const char *pFilename, udFileOpenFlags flags);

// Optional handler for archives to handle setting a new subfile
typedef udResult udFile_SetSubFilenameFunc(udFile *pFile, const char *pSubFilename);

// Perform a load - reads the entire file into memory
typedef udResult udFile_LoadHandlerFunc(udFile *pFile, void **ppBuffer, int64_t *pBufferLength);

// Perform a seek followed by read
typedef udResult udFile_SeekReadHandlerFunc(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest *pPipelinedRequest);

// Perform a seek followed by write
typedef udResult udFile_SeekWriteHandlerFunc(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualWritten);

// Receive the data for a piped request, returning an error if attempting to receive pipelined requests out of order
typedef udResult udFile_BlockForPipelinedRequestHandlerFunc(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead);

// Release the underlying file handle (optional) to be re-opened upon next use - used to have more open files than internal (o/s) limits would otherwise allow
typedef udResult udFile_ReleaseHandlerFunc(udFile *pFile);

// Close the file and free all resources allocated, including the udFile structure itself
typedef udResult udFile_CloseHandlerFunc(udFile **ppFile);

// The base file structure, file handlers are expected to zero this structure and extend the to include custom data to manage state.
struct udFile
{
  const char *pFilenameCopy;              // If assigned by a handler, set filenameCopyRequiresFree, or handler is responsible for free
  udFileOpenFlags flagsCopy;              // Set by udFile, not handlers. A copy of the flags used to open the file
  udFile_SetSubFilenameFunc *fpSetSubFilename; // Optional, for handlers of archive files such as zip etc
  udFile_LoadHandlerFunc *fpLoad;              // Optional, for handlers that can optimize the Open/Read/Close approach of udFile_Load, such as HTTP
  udFile_SeekReadHandlerFunc *fpRead;
  udFile_SeekWriteHandlerFunc *fpWrite;
  udFile_BlockForPipelinedRequestHandlerFunc *fpBlockPipedRequest;
  udFile_ReleaseHandlerFunc *fpRelease;
  udFile_CloseHandlerFunc *fpClose;
  struct udCryptoCipherContext *pCipherCtx;
  int64_t nonce, counterOffset;  // For CTR mode, the nonce and an offset added to calculated counter, used mainly by split files or to add perceived security
  int64_t seekBase;
  int64_t filePos;
  int64_t fileLength;
  uint32_t msAccumulator;
  uint32_t requestsInFlight;
  uint64_t totalBytes;
  float mbPerSec;
  bool filenameCopyRequiresFree;          // Set if the filename copy was allocated, will be freed prior to calling handler close function
};

// Register a file handler
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc *fpHandler, const char *pPrefix, bool overrideExisting = false);

// Deregister a file handler removing it from the internal list (note that functions may still be called if there are open files)
udResult udFile_DeregisterHandler(udFile_OpenHandlerFunc *fpHandler);

#endif // UDFILEHANDLER_H
