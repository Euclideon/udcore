//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
//
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "udPlatform.h"
#include "udSocket.h"

#include "udPlatformUtil.h"
#include "udFileHandler.h"
#include "udMath.h"
#include <stdio.h>

static udFile_OpenHandlerFunc                     udFileHandler_HTTPOpen;
static udFile_SeekReadHandlerFunc                 udFileHandler_HTTPSeekRead;
static udFile_BlockForPipelinedRequestHandlerFunc udFileHandler_HTTPBlockForPipelinedRequest;
static udFile_CloseHandlerFunc                    udFileHandler_HTTPClose;

#if !UDPLATFORM_EMSCRIPTEN
// Register the HTTP handler (optional as it requires networking libraries, WS2_32.lib on Windows platform)
udResult udFile_RegisterHTTP()
{
  udResult result = udFile_RegisterHandler(udFileHandler_HTTPOpen, "http:");
  if (result == udR_Success)
    result = udFile_RegisterHandler(udFileHandler_HTTPOpen, "https:");
  return result;
}
#endif


static char s_HTTPHeaderString[] = "HEAD %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\nUser-Agent: Euclideon udSDK/2.0\r\n\r\n";
static char s_HTTPGetString[] = "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Euclideon udSDK/2.0\r\nConnection: Keep-Alive\r\nRange: bytes=%lld-%lld\r\n\r\n";


// The udFile derivative for supporting HTTP/S
struct udFile_HTTP : public udFile
{
  udMutex *pMutex;                        // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads
  udURL url;
  bool wsInitialised;
  char recvBuffer[1024];
  udSocket *pSocket;
  int sockID; // Each time a socket it created we increment this number, this way pipelined requests from a dead socket can be identified as dead
};


// ----------------------------------------------------------------------------
// Open the socket
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPOpenSocket(udFile_HTTP *pFile)
{
  udResult result;

  if (!pFile->pSocket || !udSocket_IsValidSocket(pFile->pSocket))
  {
    if (pFile->pSocket)
      udSocket_Close(&pFile->pSocket);
    result = udSocket_Open(&pFile->pSocket, pFile->url.GetDomain(), pFile->url.GetPort(), udStrEqual(pFile->url.GetScheme(), "https") ? udSCF_UseTLS : udSCF_None);
  }
  else
  {
    // Valid socket already opened
    result = udR_Success;
  }

  if (result != udR_Success)
    udDebugPrintf("Error %s opening socket\n", udResultAsString(result));
  return result;
}


// ----------------------------------------------------------------------------
// Close the socket
// Author: Dave Pevreal, March 2014
static void udFileHandler_HTTPCloseSocket(udFile_HTTP *pFile)
{
  udSocket_Close(&pFile->pSocket);
  ++pFile->sockID;
}


// ----------------------------------------------------------------------------
// Send a request
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSendRequest(udFile_HTTP *pFile, int len)
{
  udResult result;

  UD_ERROR_CHECK(udFileHandler_HTTPOpenSocket(pFile));
  result = udSocket_SendData(pFile->pSocket, (const uint8_t*)pFile->recvBuffer, (int64_t)len);
  if (result == udR_SocketError)
  {
    // On error, first try closing and re-opening the socket before giving up
    udFileHandler_HTTPCloseSocket(pFile);
    udFileHandler_HTTPOpenSocket(pFile);
    result = udSocket_SendData(pFile->pSocket, (const uint8_t*)pFile->recvBuffer, (int64_t)len);
  }

epilogue:
  if (result != udR_Success)
    udDebugPrintf("Error %s sending request:\n%s\n--end--\n", udResultAsString(result), pFile->recvBuffer);
  return result;
}


// ----------------------------------------------------------------------------
// Receive a response for a GET packet, parsing the string header before
// delivering the payload
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPRecvGET(udFile_HTTP *pFile, void *pBuffer, size_t bufferLength, size_t *pActualRead)
{
  udResult result;
  size_t bytesReceived = 0;      // Number of bytes received from this packet
  int code;
  size_t headerLength; // length of the response header, before any payload
  const char *s;
  bool closeConnection = false;
  int64_t contentLength;
  int64_t actualReceived;

  result = udFileHandler_HTTPOpenSocket(pFile);
  if (result != udR_Success)
    udDebugPrintf("Unable to open socket\n");
  UD_ERROR_HANDLE();

  result = udSocket_ReceiveData(pFile->pSocket, (uint8_t*)pFile->recvBuffer, (int64_t)sizeof(pFile->recvBuffer), &actualReceived);
  if (result == udR_SocketError)
  {
    // Close and re-open the socket on error
    udFileHandler_HTTPCloseSocket(pFile);
    UD_ERROR_CHECK(udFileHandler_HTTPOpenSocket(pFile));
    UD_ERROR_CHECK(udSocket_ReceiveData(pFile->pSocket, (uint8_t*)pFile->recvBuffer, (int64_t)sizeof(pFile->recvBuffer), &actualReceived));
  }
  bytesReceived += (size_t)actualReceived;

  while (udStrstr(pFile->recvBuffer, bytesReceived, "\r\n\r\n", &headerLength) == nullptr && (size_t)bytesReceived < sizeof(pFile->recvBuffer))
  {
    UD_ERROR_CHECK(udSocket_ReceiveData(pFile->pSocket, (uint8_t*)pFile->recvBuffer + bytesReceived, (int64_t)sizeof(pFile->recvBuffer) - bytesReceived, &actualReceived));
    bytesReceived += (size_t)actualReceived;
  }

  // First, check the top line for HTTP version and error code
  code = 0;
  sscanf(pFile->recvBuffer, "HTTP/1.1 %d", &code);
  if ((code != 200 && code != 206) || (headerLength == (size_t)bytesReceived)) // if headerLength is bytesReceived, never found the \r\n\r\n
  {
    udDebugPrintf("Fail on packet: code = %d headerLength = %d (bytesReceived = %d)\n", code, (int)headerLength, (int)bytesReceived);
    UD_ERROR_SET(udR_SocketError);
  }

  headerLength += 4;
  pFile->recvBuffer[headerLength-1] = 0; // null terminate the header part
  //udDebugPrintf("Received:\n%s--end--\n", pFile->recvBuffer);

  // Check for a request from the server to close the connection after dealing with this
  closeConnection = udStrstr(pFile->recvBuffer, headerLength, "Connection: close") != nullptr;
  if (closeConnection)
    udDebugPrintf("Server requesting connection close\n");

  s = udStrstr(pFile->recvBuffer, headerLength, "Content-Length:");
  if (!s)
  {
    udDebugPrintf("http: No content-length field found\n");
    UD_ERROR_SET(udR_SocketError);
  }
  contentLength = udStrAtoi64(s + 15);

  if (!pBuffer)
  {
    // Parsing response to the HEAD to get size of overall file
    pFile->fileLength = contentLength;
  }
  else
  {
    // Parsing response to a GET
    if (contentLength > (int64_t)bufferLength)
    {
      udDebugPrintf("contentLength=%lld bufferLength=%lld\n", contentLength, bufferLength);
      UD_ERROR_SET(udR_SocketError);
    }

    // Some servers send more data after the content, we should throw it out
    bytesReceived = udMin((int64_t)bytesReceived - (int64_t)headerLength, contentLength);
    memcpy(pBuffer, pFile->recvBuffer + headerLength, bytesReceived);

    while (bytesReceived < (size_t)contentLength)
    {
      UD_ERROR_CHECK(udSocket_ReceiveData(pFile->pSocket, (uint8_t*)pBuffer + bytesReceived, (int64_t)bufferLength - bytesReceived, &actualReceived));
      bytesReceived += (size_t)actualReceived;
    }
    if (pActualRead)
      *pActualRead = bytesReceived;
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success || closeConnection)
    udFileHandler_HTTPCloseSocket(pFile);
  if (result != udR_Success)
    udDebugPrintf("Error receiving request:\n%s\n--end--\n", pFile->recvBuffer);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of OpenHandler via HTTP
// Author: Dave Pevreal, March 2014
udResult udFileHandler_HTTPOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udResult result;
  udFile_HTTP *pFile = nullptr;
  int actualHeaderLen;

  // Automatically fail if trying to write to files on http
  if (flags & (udFOF_Write|udFOF_Create))
    UD_ERROR_SET(udR_OpenFailure);

  pFile = udAllocType(udFile_HTTP, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  if (flags & udFOF_Multithread)
  {
    pFile->pMutex = udCreateMutex();
    UD_ERROR_NULL(pFile->pMutex, udR_InternalError);
  }

  pFile->url.Construct();
  pFile->wsInitialised = false;

  UD_ERROR_CHECK(pFile->url.SetURL(pFilename));
  UD_ERROR_IF(!udStrEqual(pFile->url.GetScheme(), "http") && !udStrEqual(pFile->url.GetScheme(), "https"), udR_OpenFailure);
  UD_ERROR_CHECK(udSocket_InitSystem());
  pFile->wsInitialised = true;

  actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPHeaderString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain());
  UD_ERROR_IF(actualHeaderLen < 0, udR_Failure_);

  //udDebugPrintf("Sending:\n%s", pFile->recvBuffer);
  UD_ERROR_CHECK(udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen));
  UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, nullptr, 0, nullptr));

  pFile->fpRead = udFileHandler_HTTPSeekRead;
  pFile->fpBlockPipedRequest = udFileHandler_HTTPBlockForPipelinedRequest;
  pFile->fpClose = udFileHandler_HTTPClose;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFileHandler_HTTPClose((udFile**)&pFile);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of SeekReadHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSeekRead(udFile *pBaseFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest *pPipelinedRequest)
{
  udResult result;
  udFile_HTTP *pFile = static_cast<udFile_HTTP *>(pBaseFile);

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);

  //udDebugPrintf("\nSeekRead: %lld bytes at offset %lld\n", bufferLength, offset);
  size_t actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPGetString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain(), seekOffset, seekOffset + bufferLength-1);

  UD_ERROR_CHECK(udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen));

  if (pPipelinedRequest)
  {
    pPipelinedRequest->reserved[0] = (uint64_t)(pBuffer);
    pPipelinedRequest->reserved[1] = (uint64_t)(bufferLength);
    pPipelinedRequest->reserved[2] = (uint64_t)pFile->sockID;
    pPipelinedRequest->reserved[3] = 0;
    if (pActualRead)
      *pActualRead = bufferLength; // Being optimistic
  }
  else
  {
    UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, pBuffer, bufferLength, pActualRead));
  }

epilogue:

  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of BlockForPipelinedRequest via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPBlockForPipelinedRequest(udFile *pBaseFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead)
{
  udResult result;
  udFile_HTTP *pFile = static_cast<udFile_HTTP *>(pBaseFile);

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);

  void *pBuffer = (void*)(pPipelinedRequest->reserved[0]);
  size_t bufferLength = (size_t)(pPipelinedRequest->reserved[1]);
  int sockID = (int)pPipelinedRequest->reserved[2];
  if (sockID != pFile->sockID)
  {
    udDebugPrintf("Pipelined request failed due to socket close/reopen. Expected %d, socket id is now %d\n", sockID, pFile->sockID);
    UD_ERROR_SET(udR_SocketError);
  }
  else
  {
    UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, pBuffer, bufferLength, pActualRead));
  }
  result = udR_Success;

epilogue:
  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of CloseHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPClose(udFile **ppFile)
{
  udFile_HTTP *pFile = nullptr;

  if (ppFile)
  {
    pFile = static_cast<udFile_HTTP *>(*ppFile);
    *ppFile = nullptr;
    if (pFile)
    {
      udFileHandler_HTTPCloseSocket(pFile);
      if (pFile->wsInitialised)
      {
        udSocket_DeinitSystem();
        pFile->wsInitialised = false;
      }
      if (pFile->pMutex)
        udDestroyMutex(&pFile->pMutex);
      pFile->url.~udURL();
      udFree(pFile);
    }
  }

  return udR_Success;
}
