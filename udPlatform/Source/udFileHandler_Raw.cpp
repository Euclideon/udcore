#include "udFile.h"
#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include "udCompression.h"
#include "udMath.h"

// Raw support udFile handler
// To encode to base64 use this: https://www.browserling.com/tools/file-to-base64
// The filename is the file. raw://SGVsbG8gV29ybGQ= = "Hello World"

// Declarations of the fall-back standard handler that uses crt Raw as a back-end
static udFile_SeekReadHandlerFunc   udFileHandler_RawSeekRead;
static udFile_SeekWriteHandlerFunc  udFileHandler_RawSeekWrite;
static udFile_CloseHandlerFunc      udFileHandler_RawClose;

// The udFile derivative for supporting base64 raw and compressed files
struct udFile_Raw : public udFile
{
  uint8_t *pData;
  size_t dataLen;
  int64_t fp;
};


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt Raw i/o functions
udResult udFileHandler_RawOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags /*flags*/)
{
  UDTRACE();
  udFile_Raw *pRaw = nullptr;
  udResult result;
  size_t dataStart = 6; // length of raw://
  udCompressionType compressionType = udCT_None;
  uint8_t *pCompressed = nullptr;
  size_t compressedLen = 0;
  size_t size = 0;

  pRaw = udAllocType(udFile_Raw, 1, udAF_Zero);
  UD_ERROR_NULL(pRaw, udR_MemoryAllocationFailure);

  pRaw->fpRead = udFileHandler_RawSeekRead;
  pRaw->fpWrite = udFileHandler_RawSeekWrite;
  pRaw->fpClose = udFileHandler_RawClose;

  if (udStrchr(pFilename + dataStart, "@") != nullptr)
  {
    while (pFilename[dataStart] != '@')
    {
      if (udStrBeginsWithi(pFilename + dataStart, "compression="))
      {
        dataStart += udStrlen("compression=");
        for (udCompressionType ct = (udCompressionType)(udCT_None + 1); ct < udCT_Count; ct = (udCompressionType)(ct + 1))
        {
          if (udStrBeginsWithi(pFilename + dataStart, udCompressionTypeAsString(ct)))
          {
            size_t ctLen = udStrlen(udCompressionTypeAsString(ct));
            compressionType = ct;
            dataStart += ctLen;
            break;
          }
        }
        UD_ERROR_IF(compressionType == udCT_None, udR_ParseError);
      }
      else if (udStrBeginsWithi(pFilename + dataStart, "size="))
      {
        dataStart += udStrlen("size=");
        int charCount = 0;
        size = udStrAtou(pFilename + dataStart, &charCount);
        dataStart += charCount;
      }
      else
      {
        break; // An error
      }
      if (pFilename[dataStart] == ',')
        ++dataStart;
    }
    UD_ERROR_IF(pFilename[dataStart] != '@', udR_ParseError);
    ++dataStart;
  }

  if (compressionType != udCT_None)
  {
    UD_ERROR_IF(size == 0, udR_InvalidConfiguration); // Need size specified
    UD_ERROR_CHECK(udBase64Decode(&pCompressed, &compressedLen, pFilename + dataStart));
    pRaw->pData = udAllocType(uint8_t, size, udAF_None);
    pRaw->dataLen = size;
    UD_ERROR_NULL(pRaw->pData, udR_MemoryAllocationFailure);
    UD_ERROR_CHECK(udCompression_Inflate(pRaw->pData, pRaw->dataLen, pCompressed, compressedLen, nullptr, compressionType));
  }
  else
  {
    UD_ERROR_CHECK(udBase64Decode(&pRaw->pData, &pRaw->dataLen, pFilename + dataStart));
  }

  pRaw->fileLength = (int64_t)pRaw->dataLen;
  *ppFile = pRaw;
  pRaw = nullptr;
  result = udR_Success;

epilogue:
  udFree(pCompressed);
  if (pRaw)
  {
    udFree(pRaw->pData);
    udFree(pRaw);
  }
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
static udResult udFileHandler_RawSeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udResult result;
  udFile_Raw *pRaw = static_cast<udFile_Raw*>(pFile);
  size_t actualRead;

  UD_ERROR_IF(seekOffset < 0 || seekOffset >= (int64_t)pRaw->dataLen, udR_InvalidParameter_);
  actualRead = udMin(bufferLength, pRaw->dataLen - (size_t)seekOffset);
  memcpy(pBuffer, pRaw->pData + seekOffset, actualRead);

  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

epilogue:
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
static udResult udFileHandler_RawSeekWrite(udFile *pFile, const void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualWritten)
{
  UDTRACE();
  udResult result;
  udFile_Raw *pRaw = static_cast<udFile_Raw*>(pFile);
  size_t actualWritten;

  UD_ERROR_IF(seekOffset < 0 || seekOffset >= (int64_t)pRaw->dataLen, udR_InvalidParameter_);
  actualWritten = udMin(bufferLength, pRaw->dataLen - (size_t)seekOffset);
  memcpy(pRaw->pData + seekOffset, pBuffer, actualWritten);

  if (pActualWritten)
    *pActualWritten = actualWritten;

  result = udR_Success;

epilogue:
  return result;
}


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of CloseHandler to access the crt Raw i/o functions
static udResult udFileHandler_RawClose(udFile **ppFile)
{
  UDTRACE();
  udFile_Raw *pRaw = static_cast<udFile_Raw*>(*ppFile);
  *ppFile = nullptr;

  if (pRaw)
  {
    udFree(pRaw->pData);
    udFree(pRaw);
  }

  return udR_Success;
}


