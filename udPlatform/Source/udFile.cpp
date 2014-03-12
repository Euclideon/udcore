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

struct udFileHandler
{
  udFile_OpenHandlerFunc *fpOpen;
  char prefix[16];              // The prefix that this handler will respond to, eg 'http:', or an empty string for regular filenames
  udFileHandler *pNext;         // Temp until udSparsePtrArray is up and running
};

static udFileHandler s_defaultFileHandler = { udFileHandler_FILEOpen, "", nullptr };
static udFileHandler *s_fileHandlers = &s_defaultFileHandler;

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
// Author: Dave Pevreal   Date: March, 2014
udResult udFile_SeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead)
{
  udResult result;
  size_t actualRead;

  result = udR_File_ReadFailure;
  if (pFile == nullptr || pFile->fpRead == nullptr)
    goto epilogue;

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);
  result = pFile->fpRead(pFile, pBuffer, bufferLength, seekOffset, seekWhence, pActualRead ? pActualRead : &actualRead);
  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  // If the caller isn't checking the actual read (ie it's null), and it's not the requested amount, return an error when full amount isn't actually read
  if (result == udR_Success && pActualRead == nullptr && actualRead != bufferLength)
    result = udR_File_ReadFailure;

epilogue:
  return result;
}


// ****************************************************************************
// Author: Dave Pevreal   Date: March, 2014
udResult udFile_SeekWrite(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten)
{
  udResult result;
  size_t actualWritten;

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
// Author: Dave Pevreal  March, 2014
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
// Author: Dave Pevreal  March, 2014
udResult udFile_RegisterHandler(udFile_OpenHandlerFunc * /*fpHandler*/, bool /*lowPriority*/)
{
  return udR_Failure_;
}


// ****************************************************************************
// Author: Dave Pevreal  March, 2014
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
// Implementation of OpenHandler to access the crt FILE i/o functions
// Author: Dave Pevreal  March, 2014
static udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udFile_FILE *pFile = nullptr;
  udResult result;
  const char *pMode;

  result = udR_MemoryAllocationFailure;
  pFile = udNew udFile_FILE;
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
    udDelete(pFile);
  }

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of SeekReadHandler to access the crt FILE i/o functions
// Author: Dave Pevreal  March, 2014
static udResult udFileHandler_FILESeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead)
{
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);
  udResult result;
  size_t actualRead;

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualRead = fread(pBuffer, 1, bufferLength, pFILE->pCrtFile);
  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

//epilogue:

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of SeekWriteHandler to access the crt FILE i/o functions
// Author: Dave Pevreal  March, 2014
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
// Implementation of CloseHandler to access the crt FILE i/o functions
// Author: Dave Pevreal  March, 2014
static udResult udFileHandler_FILEClose(udFile **ppFile)
{
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(*ppFile);
  udResult result;

  *ppFile = nullptr;
  result = udR_File_CloseFailure;
  if (fclose(pFILE->pCrtFile) == 0)
    result = udR_Success;

//epilogue:

  return result;
}


