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
# define _LARGEFILE_SOURCE
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

#define FILE_DEBUG 0

// Declarations of the fall-back standard handler that uses crt FILE as a back-end
static udFile_SeekReadHandlerFunc   udFileHandler_FILESeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_FILESeekWrite;
static udFile_ReleaseHandlerFunc    udFileHandler_FILERelease;
static udFile_CloseHandlerFunc      udFileHandler_FILEClose;


// The udFile derivative for supporting standard runtime library FILE i/o
struct udFile_FILE : public udFile
{
  FILE *pCrtFile;
  int64_t filePos;
  udMutex *pMutex;                        // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads
};


// ----------------------------------------------------------------------------
static FILE *OpenWithFlags(const char *pFilename, udFileOpenFlags flags)
{
  const char *pMode = "";
  FILE *pFile = nullptr;

  if ((flags & udFOF_Read) && (flags & udFOF_Write) && (flags & udFOF_Create))
    pMode = "w+b";  // Read/write, any existing file destroyed
  else if ((flags & udFOF_Read) && (flags & udFOF_Write))
    pMode = "r+b"; // Read/write, but file must already exist
  else if (flags & udFOF_Read)
    pMode = "rb"; // Read, file must already exist
  else if ((flags & udFOF_Write) || (flags & udFOF_Create))
    pMode = "wb"; // Write, any existing file destroyed (Create flag treated as Write in this case)
  else
    return nullptr;

#if UDPLATFORM_WINDOWS
  pFile = _wfopen(udOSString(pFilename), udOSString(pMode));
#else
  pFile = fopen(pFilename, pMode);
#endif

#if FILE_DEBUG
  if (!pFile)
    udDebugPrintf("Error opening %s (%s)\n", pFilename, pMode);
#endif

  return pFile;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt FILE i/o functions
udResult udFileHandler_FILEOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags, int64_t *pFileLengthInBytes)
{
  UDTRACE();
  udFile_FILE *pFile = nullptr;
  udResult result;

  if (pFileLengthInBytes && (flags & udFOF_Read) && !(flags & udFOF_FastOpen))
  {
    result = udFileExists(pFilename, pFileLengthInBytes);
    if (result != udR_Success)
      *pFileLengthInBytes = 0;
  }

  pFile = udAllocType(udFile_FILE, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  pFile->fpRead = udFileHandler_FILESeekRead;
  pFile->fpWrite = udFileHandler_FILESeekWrite;
  pFile->fpRelease = udFileHandler_FILERelease;
  pFile->fpClose = udFileHandler_FILEClose;

  if (!(flags & udFOF_FastOpen)) // With FastOpen flag, just don't open the file, let the first read do that
  {
    pFile->pCrtFile = OpenWithFlags(pFilename, flags);
    UD_ERROR_NULL(pFile->pCrtFile, udR_File_OpenFailure);
  }

  if (flags & udFOF_Multithread)
  {
    pFile->pMutex = udCreateMutex();
    UD_ERROR_NULL(pFile->pMutex, udR_InternalError);
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

  if (pFILE->pCrtFile == nullptr)
  {
#if FILE_DEBUG
    udDebugPrintf("Reopening handle for %s\n", pFile->pFilenameCopy);
#endif
    pFILE->pCrtFile = OpenWithFlags(pFile->pFilenameCopy, pFile->flagsCopy);
    UD_ERROR_NULL(pFILE->pCrtFile, udR_File_OpenFailure);
    if (seekWhence == udFSW_SeekCur)
      fseeko(pFILE->pCrtFile, pFILE->filePos, SEEK_SET);
  }

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
  {
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);
    pFILE->filePos = ftello(pFILE->pCrtFile);
  }

  actualRead = bufferLength ? fread(pBuffer, 1, bufferLength, pFILE->pCrtFile) : 0;
  if (pActualRead)
    *pActualRead = actualRead;
  UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_File_ReadFailure);

  pFILE->filePos = ftello(pFILE->pCrtFile);
  UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_File_ReadFailure);
  if (pFilePos)
    *pFilePos = pFILE->filePos;

  result = udR_Success;

epilogue:
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

  UD_ERROR_NULL(pFILE->pCrtFile, udR_File_OpenFailure);

  if (seekOffset != 0 || seekWhence != udFSW_SeekCur)
    fseeko(pFILE->pCrtFile, seekOffset, seekWhence);

  actualWritten = fwrite(pBuffer, 1, bufferLength, pFILE->pCrtFile);
  if (pActualWritten)
    *pActualWritten = actualWritten;
  UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_File_WriteFailure);

  pFILE->filePos = ftello(pFILE->pCrtFile);
  if (pFilePos)
    UD_ERROR_IF(ferror(pFILE->pCrtFile) != 0, udR_File_WriteFailure);

  result = udR_Success;

epilogue:
  if (pFILE->pMutex)
    udReleaseMutex(pFILE->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2016
// Implementation of Release to release the underlying file handle
static udResult udFileHandler_FILERelease(udFile *pFile)
{
  udResult result;
  udFile_FILE *pFILE = static_cast<udFile_FILE*>(pFile);

  // Early-exit that doesn't involve locking the mutex
  if (!pFILE->pCrtFile)
    return udR_NothingToDo;

  if (pFILE->pMutex)
    udLockMutex(pFILE->pMutex);

  // Don't support release/reopen on files for create/writing
  UD_ERROR_IF(!pFile->pFilenameCopy || (pFile->flagsCopy & (udFOF_Create|udFOF_Write)), udR_InvalidConfiguration);

#if FILE_DEBUG
  udDebugPrintf("Releasing handle for %s\n", pFile->pFilenameCopy);
#endif
  fclose(pFILE->pCrtFile);
  pFILE->pCrtFile = nullptr;

  result = udR_Success;

epilogue:

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
  *ppFile = nullptr;

  udResult result = (pFILE->pCrtFile && fclose(pFILE->pCrtFile) != 0) ? udR_File_CloseFailure : udR_Success;

  if (pFILE->pMutex)
    udDestroyMutex(&pFILE->pMutex);
  udFree(pFILE);

  return result;
}


