//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, April 2014
// 

#include "udFile.h"
#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <memory.h>

// Declarations of the fall-back standard handler that uses crt FILE as a back-end
static udFile_SeekReadHandlerFunc   udFileHandler_FILESeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_FILESeekWrite;
static udFile_CloseHandlerFunc      udFileHandler_FILEClose;


// The udFile derivative for supporting standard runtime library FILE i/o
struct udFile_FILE : public udFile
{
  FILE *pCrtFile;
};


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt FILE i/o functions
udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udFile_FILE *pFile = nullptr;
  udResult result;
  const char *pMode;

  result = udR_MemoryAllocationFailure;
  pFile = udAllocType(udFile_FILE, 1);
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


