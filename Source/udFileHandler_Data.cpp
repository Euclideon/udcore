#include "udFile.h"
#include "udFileHandler.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udCompression.h"
#include "udMath.h"

// Data URLs support udFile handler
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URIs

static udFile_SeekReadHandlerFunc   udFileHandler_DataSeekRead;
static udFile_CloseHandlerFunc      udFileHandler_DataClose;

struct udFile_Data : public udFile
{
  uint8_t *pData;
  size_t dataLen;
};

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
udResult udFileHandler_DataOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  UDTRACE();
  udResult result;
  udFile_Data *pData = nullptr;

  const char *pBase64 = nullptr;
  const char *pDataPtr = nullptr;
  size_t dataIndex = 0;

  udUnused(flags);

  pData = udAllocType(udFile_Data, 1, udAF_Zero);
  UD_ERROR_NULL(pData, udR_MemoryAllocationFailure);

  pDataPtr = udStrchr(pFilename, ",", &dataIndex) + 1;
  pBase64 = udStrstr(pFilename, dataIndex, ";base64");

  if (pBase64)
  {
    UD_ERROR_CHECK(udBase64Decode(&pData->pData, &pData->dataLen, pDataPtr));
  }
  else
  {
    size_t originalLen = udStrlen(pDataPtr);
    pData->dataLen = originalLen;
    pData->pData = udAllocType(uint8_t, pData->dataLen, udAF_None);

    // Copy data, and convert any URL escaped encoded characters
    for (size_t i = 0, j = 0; i < originalLen; ++i, ++j)
    {
      if (pDataPtr[i] == '%')
      {
        char code[] = { pDataPtr[i + 1], pDataPtr[i + 2], 0 };
        pData->pData[j] = (char)udStrAtoi(code, nullptr, 16);
        i += 2;
        pData->dataLen -= 2;
      }
      else
      {
        pData->pData[j] = pDataPtr[i];
      }
    }
  }

  pData->fpRead = udFileHandler_DataSeekRead;
  pData->fpClose = udFileHandler_DataClose;
  pData->fileLength = pData->dataLen;

  *ppFile = pData;
  pData = nullptr;
  result = udR_Success;

epilogue:
  if (pData)
  {
    udFree(pData);
  }
  return result;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
static udResult udFileHandler_DataSeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  udResult result;
  udFile_Data *pData = static_cast<udFile_Data *>(pFile);
  size_t actualRead;

  UD_ERROR_IF(seekOffset < 0 || seekOffset >= (int64_t)pData->dataLen, udR_InvalidParameter);
  actualRead = udMin(bufferLength, pData->dataLen - (size_t)seekOffset);
  memcpy(pBuffer, pData->pData + seekOffset, actualRead);

  if (pActualRead)
    *pActualRead = actualRead;

  result = udR_Success;

epilogue:
  return result;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, June 2020
static udResult udFileHandler_DataClose(udFile **ppFile)
{
  UDTRACE();
  udResult result;
  udFile_Data *pData = static_cast<udFile_Data *>(*ppFile);
  *ppFile = nullptr;

  result = udR_Success;

  if (pData)
  {
    udFree(pData->pData);
    udFree(pData);
  }
  return result;
}
