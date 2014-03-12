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

// Some function prototypes required if implementing a custom file handler

// The OpenHandler is responsible for allocating a derivative of udFile (generally a structure inherited from it) and writing
// any state required for SeekRead/SeekWrite/Close to function. The fpWrite can be null if writing is not supported.
typedef udResult udFile_OpenHandlerFunc(udFile **ppFile, const char *pFilename, udFileOpenFlags flags);

// Perform a seek followed by read, though the default parameters are a SeekCur of 0 (no seek operation) so in that case it may be skipped
typedef udResult udFile_SeekReadHandlerFunc(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead);

// Perform a seek followed by write, though the default parameters are a SeekCur of 0 (no seek operation) so in that case it may be skipped
typedef udResult udFile_SeekWriteHandlerFunc(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten);

// Close the file and free all resources allocated, including the udFile structure itself
typedef udResult udFile_CloseHandlerFunc(udFile **ppFile);

// The base file structure, hile handlers are expected to extend the udFile structure to include custom data to manage state.
struct udFile
{
  udFile_SeekReadHandlerFunc *fpRead;
  udFile_SeekWriteHandlerFunc *fpWrite;
  udFile_CloseHandlerFunc *fpClose;
  udMutex *pMutex;    // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads, OpenHandlerFunc's need only set to null
};

// Register a file handler, optionally pushing to the back of the list if low priority is desired behaviour
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc *pHandler, bool lowPriority);

// Deregister a file handler removing it from the internal list (note that functions may still be called if there are open files)
udResult udFile_DeregisterHandler(udFile_OpenHandlerFunc *pHandler);



#endif // UDFILEHANDLER_H
