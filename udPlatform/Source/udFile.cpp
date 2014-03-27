//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
// 

#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <memory.h>

// Declarations of the fall-back standard handler that uses crt FILE as a back-end
static udFile_OpenHandlerFunc       udFileHandler_FILEOpen;
static udFile_SeekReadHandlerFunc   udFileHandler_FILESeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_FILESeekWrite;
static udFile_CloseHandlerFunc      udFileHandler_FILEClose;
       udFile_OpenHandlerFunc       udFileHandler_HTTPOpen;     // Plus the HTTP handler

struct udFileHandler
{
  udFile_OpenHandlerFunc *fpOpen;
  char prefix[16];              // The prefix that this handler will respond to, eg 'http:', or an empty string for regular filenames
  udFileHandler *pNext;         // Temp until udSparsePtrArray is up and running
};

// TEMP: Pre-initialise the linked list, do this via the Register function when there is an initialisation path
static udFileHandler s_defaultFileHandler = { udFileHandler_FILEOpen, "", nullptr };
static udFileHandler s_httpFileHandler =    { udFileHandler_HTTPOpen, "http:", &s_defaultFileHandler };
static udFileHandler *s_fileHandlers = &s_httpFileHandler;

// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_Open(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udResult result = udR_File_OpenFailure;
  for (udFileHandler *pHandler = s_fileHandlers; pHandler; pHandler = pHandler->pNext)
  {
    if (udStrBeginsWith(pFilename, pHandler->prefix))
    {
      result = pHandler->fpOpen(ppFile, pFilename, flags);
      if (result == udR_Success)
      {
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
// Author: Dave Pevreal, March 2014
udResult udFile_GetPerformance(udFile *pFile, float *pMBPerSec, uint32_t *pRequestsInFlight)
{
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
  pFile->msAccumulator += udGetTimeMs();
  pFile->totalBytes += actualRead;
  if (--pFile->requestsInFlight == 0)
    pFile->mbPerSec = float((pFile->totalBytes/1048576.0) / (pFile->msAccumulator / 1000.0));
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead, udFilePipelinedRequest *pPipelinedRequest)
{
  udResult result;
  size_t actualRead;

  result = udR_File_ReadFailure;
  if (pFile == nullptr || pFile->fpRead == nullptr)
    goto epilogue;

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);
  ++pFile->requestsInFlight;
  pFile->msAccumulator -= udGetTimeMs();
  result = pFile->fpRead(pFile, pBuffer, bufferLength, seekOffset, seekWhence, &actualRead, pFile->fnBlockPipedRequest ? pPipelinedRequest : nullptr);

  // Save off the actualRead in the request for the case where the handler doesn't support piped requests
  if (pPipelinedRequest && !pFile->fnBlockPipedRequest)
    pPipelinedRequest->reserved[0] = (uint64_t)actualRead;

  // Update the performance stats unless it's a supported pipelined request (in which case the stats are updated in the block function)
  if (!pPipelinedRequest || !pFile->fnBlockPipedRequest)
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
udResult udFile_SeekWrite(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten)
{
  udResult result;
  size_t actualWritten = 0; // Assign to zero to avoid incorrect compiler warning;

  result = udR_File_ReadFailure;
  if (pFile == nullptr || pFile->fpRead == nullptr)
    goto epilogue;

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);
  result = pFile->fpWrite(pFile, pBuffer, bufferLength, seekOffset, seekWhence, pActualWritten ? pActualWritten : &actualWritten);
  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  // If the caller isn't checking the actual written (ie it's null), and it's not the requested amount, return an error when full amount isn't actually written
  if (result == udR_Success && pActualWritten == nullptr && actualWritten != bufferLength)
    result = udR_File_ReadFailure;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_BlockForPipelinedRequest(udFile *pFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead)
{
  udResult result;

  if (pFile->fnBlockPipedRequest)
  {
    size_t actualRead;
    result = pFile->fnBlockPipedRequest(pFile, pPipelinedRequest, &actualRead);
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
  if (ppFile == nullptr)
    return udR_InvalidParameter_;

  if (*ppFile != nullptr && (*ppFile)->fpClose != nullptr)
  {
    if ((*ppFile)->pMutex)
      udDestroyMutex(&(*ppFile)->pMutex);
    return (*ppFile)->fpClose(ppFile);
  }
  else
  {
    return udR_File_CloseFailure;
  }
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc * /*fpHandler*/, bool /*lowPriority*/)
{
  return udR_Failure_;
}


// ****************************************************************************
// Author: Dave Pevreal, March 2014
udResult udFile_DeregisterHandler(udFileHandler * /*fpHandler*/)
{
  return udR_Failure_;
}


// The udFile derivative for supporting standard runtime library FILE i/o
struct udFile_FILE : public udFile
{
  FILE *pCrtFile;
};


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udFile_FILE *pFile = nullptr;
  udResult result;
  const char *pMode;

  result = udR_MemoryAllocationFailure;
  pFile = udAllocType(udFile_FILE);
  if (pFile == nullptr)
    goto epilogue;

  memset(pFile, 0, sizeof(*pFile)); // Ensure everything is null
  pFile->fpRead = udFileHandler_FILESeekRead;
  pFile->fpWrite = udFileHandler_FILESeekWrite;
  pFile->fpClose = udFileHandler_FILEClose;
  
  result = udR_File_OpenFailure;
  pMode = "";

  if ((flags & udFOF_Read) && (flags & udFOF_Write) && (flags & udFOF_Create))
    pMode = "w+b";  // Read/write, any existing file destroyed
  else if ((flags & udFOF_Read) && (flags & udFOF_Write))
    pMode = "r+b"; // Read/write, but file must already exist
  else if (flags & udFOF_Read)
    pMode = "rb"; // Read, file must already exist
  else if (flags & udFOF_Write)
    pMode = "wb"; // Write, any existing file destroyed (Create flag ignored in this case)
  pFile->pCrtFile = fopen(pFilename, pMode);
  if (pFile->pCrtFile == nullptr)
    goto epilogue;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
  {
    if (pFile->pCrtFile)
      fclose(pFile->pCrtFile);
    udFree(pFile);
  }

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of SeekReadHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILESeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);
  udResult result;
  size_t actualRead;

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualRead = bufferLength ? fread(pBuffer, 1, bufferLength, pFILE->pCrtFile) : 0;
  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

//epilogue:

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of SeekWriteHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILESeekWrite(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten)
{
  udResult result;
  size_t actualWritten;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualWritten = fwrite(pBuffer, 1, bufferLength, pFILE->pCrtFile);
  if (pActualWritten)
    *pActualWritten = actualWritten;

  result = udR_Success;

//epilogue:

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of CloseHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILEClose(udFile **ppFile)
{
  udFile_FILE *pFile = static_cast<udFile_FILE*>(*ppFile);
  udResult result;

  *ppFile = nullptr;

  result = udR_File_CloseFailure;
  if (fclose(pFile->pCrtFile) == 0)
    result = udR_Success;

  udFree(pFile);
  result = udR_Success;

//epilogue:

  return result;
}


