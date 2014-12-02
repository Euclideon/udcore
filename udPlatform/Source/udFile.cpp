//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
// 

#include "udFileHandler.h"
#include "udPlatformUtil.h"

#define MAX_HANDLERS 16

udFile_OpenHandlerFunc udFileHandler_FILEOpen;     // Default crt FILE based handler

struct udFileHandler
{
  udFile_OpenHandlerFunc *fpOpen;
  char prefix[16];              // The prefix that this handler will respond to, eg 'http:', or an empty string for regular filenames
};

static udFileHandler s_handlers[MAX_HANDLERS] = 
{
  { udFileHandler_FILEOpen, "" }    // Default file handler
};
static int s_handlersCount = 1;

// ****************************************************************************
// Author: Dave Pevreal, Ocober 2014
udResult udFile_Load(const char *pFilename, void **ppMemory, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  if (!pFilename || !ppMemory)
    return udR_InvalidParameter_;
  udFile *pFile = nullptr;
  char *pMemory = nullptr;
  int64_t length = 0;
  size_t actualRead;

  udResult result = udFile_Open(&pFile, pFilename, udFOF_Read, &length); // NOTE: Length can be zero. Chrome does this on cached files.
  if (result != udR_Success)
    goto epilogue;

  if (length)
  {
    pMemory = (char*)udAlloc((size_t)length + 1); // Note always allocating 1 extra byte
    result = udFile_SeekRead(pFile, pMemory, length, 0, udFSW_SeekCur, &actualRead);
    if (result != udR_Success)
      goto epilogue;
    if (actualRead != (size_t)length)
    {
      result = udR_File_ReadFailure;
      goto epilogue;
    }
  }
  else
  {
    udDebugPrintf("udFile_Load: %s open succeeded, length unknown\n", pFilename);
    size_t alreadyRead = 0, attemptRead = 0;
    length = 16;
    for (actualRead = 0; attemptRead == actualRead; alreadyRead += actualRead)
    {
      if (alreadyRead > (size_t)length)
        length += 16;
      result = udR_MemoryAllocationFailure;
      void *pNewMem = udRealloc(pMemory, (size_t)length + 1); // Note always allocating 1 extra byte
      if (!pNewMem)
        goto epilogue;
      pMemory = (char*)pNewMem;

      attemptRead = (size_t)length + 1 - alreadyRead; // Note attempt to read 1 extra byte so EOF is detected
      result = udFile_SeekRead(pFile, pMemory + alreadyRead, attemptRead, 0, udFSW_SeekCur, &actualRead);
      if (result != udR_Success)
        goto epilogue;
    }
    UDASSERT(length >= alreadyRead, "Logic error in read loop");
    if ((size_t)length != alreadyRead)
    {
      length = alreadyRead;
      void *pNewMem = udRealloc(pMemory, (size_t)length + 1);
      if (!pNewMem)
        goto epilogue;
      pMemory = (char*)pNewMem;
    }
  }
  pMemory[length] = 0; // A nul-terminator for text files

  if (result != udR_Success)
    goto epilogue;

  // Success, pass the memory back to the caller
  *ppMemory = pMemory;
  pMemory = nullptr;

  if (pFileLengthInBytes) // Pass length back if requested
    *pFileLengthInBytes = length;

epilogue:
  udFile_Close(&pFile);
  udFree(pMemory);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  udResult result = udR_File_OpenFailure;
  if (ppFile == nullptr || pFilename == nullptr)
  {
    result = udR_InvalidParameter_;
    goto epilogue;
  }

  *ppFile = nullptr;
  if (pFileLengthInBytes)
    *pFileLengthInBytes = 0;

  for (int i = s_handlersCount - 1; i >= 0; --i)
  {
    udFileHandler *pHandler = s_handlers + i;
    if (udStrBeginsWith(pFilename, pHandler->prefix))
    {
      result = pHandler->fpOpen(ppFile, pFilename, flags, pFileLengthInBytes);
      if (result == udR_Success)
      {
        (*ppFile)->pFilenameCopy = udStrdup(pFilename);
        if (flags & udFOF_Multithread)
          (*ppFile)->pMutex = udCreateMutex();
        goto epilogue;
      }
    }
  }

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2014
const char *udFile_GetFilename(udFile *pFile)
{
  if (pFile)
    return pFile->pFilenameCopy;
  else
    return nullptr;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_GetPerformance(udFile *pFile, float *pMBPerSec, uint32_t *pRequestsInFlight)
{
  UDTRACE();
  if (!pFile)
    return udR_InvalidParameter_;

  if (pMBPerSec)
    *pMBPerSec = pFile->mbPerSec;
  if (pRequestsInFlight)
    *pRequestsInFlight = pFile->requestsInFlight;

  return udR_Success;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
static void udUpdateFilePerformance(udFile *pFile, size_t actualRead)
{
  UDTRACE();
  pFile->msAccumulator += udGetTimeMs();
  pFile->totalBytes += actualRead;
  if (--pFile->requestsInFlight == 0)
    pFile->mbPerSec = float((pFile->totalBytes/1048576.0) / (pFile->msAccumulator / 1000.0));
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead, udFilePipelinedRequest *pPipelinedRequest)
{
  UDTRACE();
  udResult result;
  size_t actualRead;

  result = udR_File_ReadFailure;
  if (pFile == nullptr || pFile->fpRead == nullptr)
  {
    result = udR_InvalidParameter_;
    goto epilogue;
  }

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);
  ++pFile->requestsInFlight;
  pFile->msAccumulator -= udGetTimeMs();
  result = pFile->fpRead(pFile, pBuffer, bufferLength, seekOffset, seekWhence, &actualRead, pFile->fpBlockPipedRequest ? pPipelinedRequest : nullptr);

  // Save off the actualRead in the request for the case where the handler doesn't support piped requests
  if (pPipelinedRequest && !pFile->fpBlockPipedRequest)
    pPipelinedRequest->reserved[0] = (uint64_t)actualRead;

  // Update the performance stats unless it's a supported pipelined request (in which case the stats are updated in the block function)
  if (!pPipelinedRequest || !pFile->fpBlockPipedRequest)
    udUpdateFilePerformance(pFile, actualRead);

  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  if (pActualRead)
    *pActualRead = actualRead;

  // If the caller isn't checking the actual read (ie it's null), and it's not the requested amount, return an error when full amount isn't actually read
  if (result == udR_Success && pActualRead == nullptr && actualRead != bufferLength)
    result = udR_File_ReadFailure;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_SeekWrite(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten)
{
  UDTRACE();
  udResult result;
  size_t actualWritten = 0; // Assign to zero to avoid incorrect compiler warning;

  result = udR_File_WriteFailure;
  if (pFile == nullptr || pFile->fpRead == nullptr)
    goto epilogue;

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);
  result = pFile->fpWrite(pFile, pBuffer, bufferLength, seekOffset, seekWhence, pActualWritten ? pActualWritten : &actualWritten);
  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  // If the caller isn't checking the actual written (ie it's null), and it's not the requested amount, return an error when full amount isn't actually written
  if (result == udR_Success && pActualWritten == nullptr && actualWritten != bufferLength)
    result = udR_File_WriteFailure;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_BlockForPipelinedRequest(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead)
{
  UDTRACE();
  udResult result;

  if (pFile->fpBlockPipedRequest)
  {
    size_t actualRead;
    result = pFile->fpBlockPipedRequest(pFile, pPipelinedRequest, &actualRead);
    udUpdateFilePerformance(pFile, actualRead);
    if (pActualRead)
      *pActualRead = actualRead;
  }
  else
  {
    if (pActualRead)
      *pActualRead = (size_t)pPipelinedRequest->reserved[0];
    result = udR_Success;
  }

  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Close(udFile **ppFile)
{
  UDTRACE();
  if (ppFile == nullptr)
    return udR_InvalidParameter_;

  if (*ppFile != nullptr && (*ppFile)->fpClose != nullptr)
  {
    if ((*ppFile)->pMutex)
      udDestroyMutex(&(*ppFile)->pMutex);
    udFree((*ppFile)->pFilenameCopy);
    return (*ppFile)->fpClose(ppFile);
  }
  else
  {
    return udR_File_CloseFailure;
  }
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc *fpHandler, const char *pPrefix)
{
  UDTRACE();
  if (s_handlersCount >= MAX_HANDLERS)
    return udR_CountExceeded;
  s_handlers[s_handlersCount].fpOpen = fpHandler;
  udStrcpy(s_handlers[s_handlersCount].prefix, sizeof(s_handlers[s_handlersCount].prefix), pPrefix);
  ++s_handlersCount;
  return udR_Success;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_DeregisterHandler(udFile_OpenHandlerFunc *fpHandler)
{
  UDTRACE();
  for (int handlerIndex = 0; handlerIndex < s_handlersCount; ++handlerIndex)
  {
    if (s_handlers[handlerIndex].fpOpen == fpHandler)
    {
      if (++handlerIndex < s_handlersCount)
        memcpy(s_handlers + handlerIndex - 1, s_handlers + handlerIndex, (s_handlersCount - handlerIndex) * sizeof(s_handlers[0]));
      --s_handlersCount;
      return udR_Success;
    }
  }

  return udR_ObjectNotFound;
}

