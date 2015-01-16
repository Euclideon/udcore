//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, April 2014
//

#define _FILE_OFFSET_BITS 64
#if defined(_MSC_VER)
# define _CRT_SECURE_NO_WARNINGS
# define fseeko _fseeki64
# define ftello _ftelli64
# if !defined(_OFF_T_DEFINED)
    typedef __int64 _off_t;
    typedef _off_t off_t;
#   define _OFF_T_DEFINED
# endif //_OFF_T_DEFINED
#elif defined(__linux__)
# if !defined(_LARGEFILE_SOURCE )
  // This must be set for linux to expose fseeko and ftello
# error "_LARGEFILE_SOURCE  not defined"
#endif

#endif


#include "udFile.h"
#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <sys/stat.h>

#if UDPLATFORM_NACL
# define fseeko fseek
# define ftello ftell
#endif

// Declarations of the fall-back standard handler that uses crt FILE as a back-end
static udFile_SeekReadHandlerFunc   udFileHandler_FILESeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_FILESeekWrite;
static udFile_CloseHandlerFunc      udFileHandler_FILEClose;


// The udFile derivative for supporting standard runtime library FILE i/o
struct udFile_FILE : public udFile
{
  FILE *pCrtFile;
  udMutex *pMutex;                        // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads
};


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt FILE i/o functions
udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  udFile_FILE *pFile = nullptr;
  udResult result;
  const char *pMode;

  if (pFileLengthInBytes && (flags & udFOF_Read))
  {
    result = udFileExists(pFilename, pFileLengthInBytes);
    if (result != udR_Success)
      *pFileLengthInBytes = 0;
  }
  result = udR_MemoryAllocationFailure;
  pFile = udAllocType(udFile_FILE, 1, udAF_Zero);
  if (pFile == nullptr)
    goto epilogue;

  pFile->fpRead = udFileHandler_FILESeekRead;
  pFile->fpWrite = udFileHandler_FILESeekWrite;
  pFile->fpClose = udFileHandler_FILEClose;

  result = udR_File_OpenFailure;
  pMode = "";

  if ((flags & udFOF_Read) && (flags & udFOF_Write) && (flags & udFOF_Create))
  {
    pMode = "w+b";  // Read/write, any existing file destroyed
  }
  else if ((flags & udFOF_Read) && (flags & udFOF_Write))
  {
    pMode = "r+b"; // Read/write, but file must already exist
  }
  else if (flags & udFOF_Read)
  {
    pMode = "rb"; // Read, file must already exist
  }
  else if ((flags & udFOF_Write) || (flags & udFOF_Create))
  {
    pMode = "wb"; // Write, any existing file destroyed (Create flag treated as Write in this case)
  }
  else
  {
    result = udR_InvalidParameter_;
    goto epilogue;
  }
  pFile->pCrtFile = fopen(pFilename, pMode);
  if (pFile->pCrtFile == nullptr)
    goto epilogue;

  if (flags & udFOF_Multithread)
  {
    result = udR_InternalError;
    pFile->pMutex = udCreateMutex();
    if (!pFile->pMutex)
      goto epilogue;
  }

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
static udResult udFileHandler_FILESeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead, int64_t *pFilePos, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);
  udResult result;
  size_t actualRead;

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);
  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualRead = bufferLength ? fread(pBuffer, 1, bufferLength, pFILE->pCrtFile) : 0;

  if (pFilePos)
    *pFilePos = ftell(pFILE->pCrtFile);

  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

//epilogue:
  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of SeekWriteHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILESeekWrite(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualWritten, int64_t *pFilePos)
{
  UDTRACE();
  udResult result;
  size_t actualWritten;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualWritten = fwrite(pBuffer, 1, bufferLength, pFILE->pCrtFile);

  if (pActualWritten)
    *pActualWritten = actualWritten;

  if (pFilePos)
    *pFilePos = ftell(pFILE->pCrtFile);

  result = udR_Success;

//epilogue:
  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of CloseHandler to access the crt FILE i/o functions
static udResult udFileHandler_FILEClose(udFile **ppFile)
{
  UDTRACE();
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(*ppFile);
  udResult result;

  *ppFile = nullptr;

  result = udR_File_CloseFailure;
  if (fclose(pFILE->pCrtFile) == 0)
    result = udR_Success;

  if (pFILE->pMutex)
    udDestroyMutex(&pFILE->pMutex);

  udFree(pFILE);
  result = udR_Success;

//epilogue:

  return result;
}


