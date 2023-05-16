#include "udPlatform.h"

#if UDPLATFORM_EMSCRIPTEN

// For cross-origin requests, the server containing the remote data must be configured to
// respond with the correct headers giving "permission" for the browser to issue the request.
// The following headers must be sent:
//  Access-Control-Allow-Origin: *
//  Access-Control-Allow-Headers: Content-Type, Range
//  Access-Control-Allow-Methods: HEAD, GET, POST, OPTIONS

#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udFileHandler.h"
#include <emscripten/fetch.h>
#include <algorithm>

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, November 2018
static udResult udFileHandler_EmscriptenHTTPSeekRead(udFile *pFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest * /*pPipelinedRequest*/)
{
  UDTRACE();
  udResult result = udR_Success;
  emscripten_fetch_attr_t attr = {};
  emscripten_fetch_t *pFetch = nullptr;
  const char *pHeaders[] = { "Range", udTempStr("bytes=%lld-%lld", seekOffset, seekOffset + bufferLength - 1), nullptr };

  // Don't perform the request if the user provides no buffer space
  UD_ERROR_IF(bufferLength == 0, udR_Success);

  emscripten_fetch_attr_init(&attr);
  udStrcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_WAITABLE;
  attr.requestHeaders = pHeaders;
  pFetch = emscripten_fetch(&attr, pFile->pFilenameCopy);
  {
    EMSCRIPTEN_RESULT ret;
    do
    {
      ret = emscripten_fetch_wait(pFetch, 60000);
    } while (ret == EMSCRIPTEN_RESULT_TIMED_OUT);
  }
  UD_ERROR_IF(pFetch->status != 200 && pFetch->status != 206, udR_ReadFailure);
  memcpy(pBuffer, pFetch->data, std::min(pFetch->numBytes, (uint64_t)bufferLength));
  if (pActualRead)
    *pActualRead = std::min(pFetch->numBytes, (uint64_t)bufferLength);

epilogue:
  if (pFetch)
    emscripten_fetch_close(pFetch);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, November 2018
static udResult udFileHandler_EmscriptenHTTPClose(udFile **ppFile)
{
  UDTRACE();
  udResult result = udR_Success;
  UD_ERROR_NULL(ppFile, udR_InvalidParameter);
  udFree(*ppFile);
epilogue:
  return result;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, November 2018
udResult udFileHandler_EmscriptenHTTPOpen(udFile **ppFile, const char * /*pFilename*/, udFileOpenFlags /*flags*/)
{
  UDTRACE();
  udResult result;
  udFile *pFile = nullptr;
  //emscripten_fetch_attr_t attr = {};
  emscripten_fetch_t *pFetch = nullptr;

  pFile = udAllocType(udFile, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  //emscripten_fetch_attr_init(&attr);
  //udStrcpy(attr.requestMethod, "GET");
  //attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_WAITABLE;
  //pFetch = emscripten_fetch(&attr, pFilename);
  //{
  //  EMSCRIPTEN_RESULT ret;
  //  do
  //  {
  //    ret = emscripten_fetch_wait(pFetch, 60000);
  //  } while (ret == EMSCRIPTEN_RESULT_TIMED_OUT);
  //}
  //UD_ERROR_IF(pFetch->status != 200 && pFetch->status != 206, udR_OpenFailure);
  //pFile->fileLength = pFetch->totalBytes;

  pFile->fpRead = udFileHandler_EmscriptenHTTPSeekRead;
  pFile->fpClose = udFileHandler_EmscriptenHTTPClose;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFetch)
    emscripten_fetch_close(pFetch);

  if (pFile)
    udFileHandler_EmscriptenHTTPClose((udFile**)&pFile);

  return result;
}

// ----------------------------------------------------------------------------
// Author: Samuel Surtees, November 2018
udResult udFile_RegisterHTTP()
{
  udResult result = udFile_RegisterHandler(udFileHandler_EmscriptenHTTPOpen, "http:");
  udDebugPrintf("registered EmscriptenHTTP (%s)\n", udResultAsString(result));
  return result;
}

#endif // UDPLATFORM_EMSCRIPTEN
