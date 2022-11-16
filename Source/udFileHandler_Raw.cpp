#include "udCompression.h"
#include "udFile.h"
#include "udFileHandler.h"
#include "udMath.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

// Raw support udFile handler
// To encode to base64 use this: https://www.browserling.com/tools/file-to-base64
// The filename is the file. raw://SGVsbG8gV29ybGQ= = "Hello World"
// Alternatively udFile_GenerateRawFilename can be used which offers compression and support for writing

// Declarations of the fall-back standard handler that uses crt Raw as a back-end
static udFile_SeekReadHandlerFunc udFileHandler_RawSeekRead;
static udFile_SeekWriteHandlerFunc udFileHandler_RawSeekWrite;
static udFile_CloseHandlerFunc udFileHandler_RawClose;

// The udFile derivative for supporting base64 raw and compressed files
struct udFile_Raw : public udFile
{
  const char *pOriginalFilename; // Copy of "original" filename, generally a human-readable name rather than base64
  uint8_t *pData;
  int64_t fp;
  size_t dataLen;
  size_t allocationSize; // When closing a file opened for write, this is the limit of the data written back to the filename pointer
  udCompressionType ct;  // For writeable files the compression type specified in the original text
};

// ****************************************************************************
// Author: Dave Pevreal, August 2018
udResult udFile_GenerateRawFilename(const char **ppResultFilename, const void *pBuffer, size_t bufferLen, udCompressionType ct, const char *pOriginalFilename, size_t allocationSize, uint32_t charsPerLine)
{
  udResult result;
  const char *pBase64 = nullptr;
  const char *pDeclare = nullptr;
  void *pCompressed = nullptr;
  char *pResult = nullptr;
  size_t base64Len;
  size_t declareLen;

  UD_ERROR_IF(!pBuffer && bufferLen, udR_InvalidParameter);
  UD_ERROR_IF(ct < 0 || ct >= udCT_Count, udR_InvalidParameter);

  if (ct != udCT_None && bufferLen)
  {
    size_t compressedLen;
    UD_ERROR_CHECK(udCompression_Deflate(&pCompressed, &compressedLen, pBuffer, bufferLen, ct));
    UD_ERROR_CHECK(udBase64Encode(&pBase64, pCompressed, compressedLen));
    udFree(pCompressed);
  }
  else
  {
    UD_ERROR_CHECK(udBase64Encode(&pBase64, pBuffer, bufferLen));
  }

  UD_ERROR_CHECK(udSprintf(&pDeclare, "raw://"));
  // For filename quoting. if outputting to debug window (ie ppResultFilename null), escape the quotes so that it's ready to paste into source code
  if (pOriginalFilename)
    UD_ERROR_CHECK(udSprintf(&pDeclare, "%sfilename=%s%s%s,", pDeclare, !ppResultFilename ? "\\\"" : "\"", pOriginalFilename, !ppResultFilename ? "\\\"" : "\""));
  // Add compression type if compression used
  if (ct != udCT_None)
    UD_ERROR_CHECK(udSprintf(&pDeclare, "%scompression=%s,", pDeclare, udCompressionTypeAsString(ct)));
  if (allocationSize)
    UD_ERROR_CHECK(udSprintf(&pDeclare, "%sallocationSize=%zu,", pDeclare, allocationSize));
  UD_ERROR_CHECK(udSprintf(&pDeclare, "%ssize=%zu@", pDeclare, bufferLen));

  declareLen = udStrlen(pDeclare);
  base64Len = udStrlen(pBase64);

  if (ppResultFilename)
  {
    size_t resultLen = declareLen + base64Len + 1;
    if (allocationSize && (allocationSize < resultLen))
    {
      udDebugPrintf("Raw file write buffer too small. Need min %zu bytes (%s)\n", resultLen, pDeclare);
      UD_ERROR_SET(udR_BufferTooSmall);
    }
    pResult = udAllocType(char, allocationSize ? allocationSize : resultLen, udAF_None);
    UD_ERROR_NULL(pResult, udR_MemoryAllocationFailure);
    udStrcpy(pResult, resultLen, pDeclare);
    udStrcat(pResult, resultLen, pBase64);
    *ppResultFilename = pResult;
    pResult = nullptr;
  }
  else
  {
    uint32_t i = (uint32_t)declareLen;
    charsPerLine = udMax(charsPerLine, i);
    udDebugPrintf("\"%s%.*s\"\n", pDeclare, (charsPerLine - i), pBase64);
    i = charsPerLine - i;
    for (; i < base64Len; i += charsPerLine)
      udDebugPrintf("\"%.*s\"\n", charsPerLine, pBase64 + i);
  }
  result = udR_Success;

epilogue:
  udFree(pResult);
  udFree(pCompressed);
  udFree(pBase64);
  udFree(pDeclare);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2019
bool udFile_IsRaw(const char *pFilename, size_t *pOffsetToBase64, const char **ppOriginalFilename, size_t *pSize, udCompressionType *pCompressionType, size_t *pAllocationSize)
{
  if (!udStrBeginsWithi(pFilename, "raw://"))
    return false;

  size_t dataStart = 6; // length of raw://
  // Assign defaults for case where not specified in filename text
  if (ppOriginalFilename)
    *ppOriginalFilename = nullptr;
  if (pCompressionType)
    *pCompressionType = udCT_None;
  if (pAllocationSize)
    *pAllocationSize = 0; // Zero denotes not specified and therefore attempting to open for write will fail

  if (udStrchr(pFilename + dataStart, "@") != nullptr)
  {
    while (pFilename[dataStart] != '@')
    {
      if (udStrBeginsWithi(pFilename + dataStart, "filename=\""))
      {
        size_t filenameLen = udStrMatchBrace(pFilename + dataStart + 9, '\\');
        if (ppOriginalFilename)
          *ppOriginalFilename = udStrndup(pFilename + dataStart + 10, filenameLen - 2);
        dataStart += 9 + filenameLen;
      }
      else if (udStrBeginsWithi(pFilename + dataStart, "compression="))
      {
        dataStart += udStrlen("compression=");
        udCompressionType ct;
        for (ct = (udCompressionType)(udCT_None + 1); ct < udCT_Count; ct = (udCompressionType)(ct + 1))
        {
          if (udStrBeginsWithi(pFilename + dataStart, udCompressionTypeAsString(ct)))
          {
            size_t ctLen = udStrlen(udCompressionTypeAsString(ct));
            if (pCompressionType)
              *pCompressionType = ct;
            dataStart += ctLen;
            break;
          }
        }
      }
      else if (udStrBeginsWithi(pFilename + dataStart, "size="))
      {
        dataStart += udStrlen("size=");
        int charCount = 0;
        size_t size = udStrAtou(pFilename + dataStart, &charCount);
        if (pSize)
          *pSize = size;
        dataStart += charCount;
      }
      else if (udStrBeginsWithi(pFilename + dataStart, "allocationSize="))
      {
        dataStart += udStrlen("allocationSize=");
        int charCount = 0;
        size_t size = udStrAtou(pFilename + dataStart, &charCount);
        if (pAllocationSize)
          *pAllocationSize = size;
        dataStart += charCount;
      }
      else
      {
        break; // An error
      }
      if (pFilename[dataStart] == ',')
        ++dataStart;
    }
    ++dataStart;
  }

  if (pOffsetToBase64)
    *pOffsetToBase64 = dataStart;

  return true;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, March 2014
// Implementation of OpenHandler to access the crt Raw i/o functions
udResult udFileHandler_RawOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  UDTRACE();
  udResult result;
  udFile_Raw *pRaw = nullptr;
  size_t offsetToBase64;
  uint8_t *pCompressed = nullptr;
  size_t compressedLen = 0;

  pRaw = udAllocType(udFile_Raw, 1, udAF_Zero);
  UD_ERROR_NULL(pRaw, udR_MemoryAllocationFailure);

  UD_ERROR_IF(!udFile_IsRaw(pFilename, &offsetToBase64, &pRaw->pOriginalFilename, &pRaw->dataLen, &pRaw->ct, &pRaw->allocationSize), udR_Failure);

  pRaw->pFilenameCopy = pFilename; // Just take a reference to the filename as it can't be duplicated
  pRaw->fpRead = udFileHandler_RawSeekRead;
  pRaw->fpClose = udFileHandler_RawClose;
  if (flags & udFOF_Write)
  {
    // Don't permit opening for write without an allocation size specified
    UD_ERROR_IF(!pRaw->allocationSize, udR_OpenFailure); // TODO: better error code
    pRaw->fpWrite = udFileHandler_RawSeekWrite;
  }

  // It is legal to provide no base64 text in the event of an empty raw file (eg opening for write, or just testing an empty file)
  if (pFilename[offsetToBase64])
  {
    if (pRaw->ct != udCT_None)
    {
      UD_ERROR_IF(pRaw->dataLen == 0, udR_InvalidConfiguration); // Need size specified
      UD_ERROR_CHECK(udBase64Decode(&pCompressed, &compressedLen, pFilename + offsetToBase64));
      pRaw->pData = udAllocType(uint8_t, pRaw->dataLen, udAF_None);
      UD_ERROR_NULL(pRaw->pData, udR_MemoryAllocationFailure);
      UD_ERROR_CHECK(udCompression_Inflate(pRaw->pData, pRaw->dataLen, pCompressed, compressedLen, nullptr, pRaw->ct));
    }
    else
    {
      UD_ERROR_CHECK(udBase64Decode(&pRaw->pData, &pRaw->dataLen, pFilename + offsetToBase64));
    }
  }
  // If a create is specified, ensure the existing file is discarded
  if (flags & udFOF_Create)
    pRaw->dataLen = 0;

  pRaw->fileLength = (int64_t)pRaw->dataLen;

  *ppFile = pRaw;
  pRaw = nullptr;
  result = udR_Success;

epilogue:
  udFree(pCompressed);
  if (pRaw)
  {
    udFree(pRaw->pOriginalFilename);
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
  udFile_Raw *pRaw = static_cast<udFile_Raw *>(pFile);
  size_t actualRead;

  UD_ERROR_IF(seekOffset < 0 || seekOffset >= (int64_t)pRaw->dataLen, udR_InvalidParameter);
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
  udFile_Raw *pRaw = static_cast<udFile_Raw *>(pFile);

  UD_ERROR_IF(seekOffset < 0, udR_InvalidParameter);
  if ((seekOffset + bufferLength) > pRaw->dataLen)
  {
    // Need to extend the file
    size_t newLen = (size_t)(seekOffset + bufferLength);
    void *pNewData = udRealloc(pRaw->pData, newLen);
    UD_ERROR_NULL(pNewData, udR_MemoryAllocationFailure);
    pRaw->pData = (uint8_t *)pNewData;
    if (seekOffset > (int64_t)pRaw->dataLen)
      memset(pRaw->pData + pRaw->dataLen, 0, seekOffset - pRaw->dataLen); // Zero the part of the extension not written to
    pRaw->dataLen = newLen;
  }
  memcpy(pRaw->pData + seekOffset, pBuffer, bufferLength);

  if (pActualWritten)
    *pActualWritten = bufferLength;

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
  udResult result;
  udFile_Raw *pRaw = static_cast<udFile_Raw *>(*ppFile);
  *ppFile = nullptr;
  const char *pTempRaw = nullptr;

  if (pRaw && pRaw->fpWrite)
  {
    UD_ERROR_CHECK(udFile_GenerateRawFilename(&pTempRaw, pRaw->pData, pRaw->dataLen, pRaw->ct, pRaw->pOriginalFilename, pRaw->allocationSize));
    udStrcpy(const_cast<char *>(pRaw->pFilenameCopy), pRaw->allocationSize, pTempRaw);
  }

  result = udR_Success;

epilogue:
  udFree(pTempRaw);
  if (pRaw)
  {
    udFree(pRaw->pOriginalFilename);
    udFree(pRaw->pData);
    udFree(pRaw);
  }
  return result;
}
