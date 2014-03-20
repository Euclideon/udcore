#include "udPlatform.h"
#include "udPlatformUtil.h"
#include "udFileHandler.h"
#if UDPLATFORM_WINDOWS
# include <winsock2.h>
#define snprintf _snprintf
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# define INVALID_SOCKET -1
# define SOCKET_ERROR -1
#endif
#include <stdio.h>
#include <memory.h>

udFile_OpenHandlerFunc              udFileHandler_HTTPOpen;
static udFile_SeekReadHandlerFunc   udFileHandler_HTTPSeekRead;
static udFile_CloseHandlerFunc      udFileHandler_HTTPClose;

static char s_HTTPHeaderString[] = "HEAD %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\nUser-Agent: Euclideon udSDK/2.0\r\n\r\n";
static char s_HTTPGetString[] = "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Euclideon udSDK/2.0\r\nConnection: Keep-Alive\r\nRange: bytes=%lld-%lld\r\n\r\n";


// The udFile derivative for supporting HTTP
struct udFile_HTTP : public udFile
{
  udURL url;
  int64_t length;
  int64_t currentOffset;
  bool wsInitialised;
  char recvBuffer[1024];
  struct sockaddr_in server;
#if UDPLATFORM_WINDOWS
  WSADATA wsaData;
  SOCKET sock;
#else
  int sock;
#endif
};


// ----------------------------------------------------------------------------
// Open the socket
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPOpenSocket(udFile_HTTP *pFile)
{
  udResult result;

  result = udR_File_SocketError;
  if (pFile->sock == INVALID_SOCKET)
  {
    pFile->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pFile->sock == INVALID_SOCKET)
      goto epilogue;

    if (connect(pFile->sock, (struct sockaddr*)&pFile->server, sizeof(pFile->server)) == SOCKET_ERROR)
      goto epilogue;
  }
  
  // TODO: TCP_NODELAY disables the Nagle algorithm.
  //       SO_KEEPALIVE enables periodic 'liveness' pings, if supported by the OS.

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udDebugPrintf("Error opening socket\n");
  return result;
}


// ----------------------------------------------------------------------------
// Close the socket
// Author: Dave Pevreal, March 2014
static void udFileHandler_HTTPCloseSocket(udFile_HTTP *pFile)
{
  if (pFile->sock != INVALID_SOCKET)
  {
#if UDPLATFORM_WINDOWS
	  shutdown(pFile->sock, SD_BOTH);
#else
	  close(pFile->sock);
#endif
    pFile->sock = INVALID_SOCKET;
  }
}


// ----------------------------------------------------------------------------
// Send a request
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSendRequest(udFile_HTTP *pFile, int len)
{
  udResult result;

  result = udFileHandler_HTTPOpenSocket(pFile);
  if (result != udR_Success)
    goto epilogue;

  result = udR_File_SocketError;
  if (send(pFile->sock, pFile->recvBuffer, len, 0) == SOCKET_ERROR)
  {
    // On error, first try closing and re-opening the socket before giving up
    udFileHandler_HTTPCloseSocket(pFile);
    udFileHandler_HTTPOpenSocket(pFile);
    if (send(pFile->sock, pFile->recvBuffer, len, 0) == SOCKET_ERROR)
      goto epilogue;
  }
  
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udDebugPrintf("Error sending request:\n%s\n--end--\n", pFile->recvBuffer);
  return result;
}


// ----------------------------------------------------------------------------
// Receive a response for a GET packet, parsing the string header before 
// delivering the payload
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPRecvGET(udFile_HTTP *pFile, void *pBuffer, size_t bufferLength)
{
  udResult result;
  int bytesReceived;      // Number of bytes received from this packet
  int code;
  size_t headerLength; // length of the response header, before any payload
  const char *s;
  bool closeConnection = false;
  int64_t contentLength;

  result = udFileHandler_HTTPOpenSocket(pFile);
  if (result != udR_Success)
    goto epilogue;
  
  result = udR_File_OpenFailure; // For now, all failures will be generic file failure

  bytesReceived = recv(pFile->sock, pFile->recvBuffer, sizeof(pFile->recvBuffer), 0);
  if (bytesReceived == SOCKET_ERROR)
  {
    udFileHandler_HTTPCloseSocket(pFile);
    udFileHandler_HTTPOpenSocket(pFile);
    bytesReceived = recv(pFile->sock, pFile->recvBuffer, sizeof(pFile->recvBuffer), 0);
    if (bytesReceived == SOCKET_ERROR)
      goto epilogue;
  }

  while (udStrstr(pFile->recvBuffer, bytesReceived, "\r\n\r\n", &headerLength) == nullptr && (size_t)bytesReceived < sizeof(pFile->recvBuffer))
  {
    int extra = recv(pFile->sock, pFile->recvBuffer + bytesReceived, sizeof(pFile->recvBuffer) - bytesReceived, 0);
    if (extra == SOCKET_ERROR)
      goto epilogue;
    bytesReceived += extra;
  }

  // First, check the top line for HTTP version and error code
  code = 0;
  sscanf(pFile->recvBuffer, "HTTP/1.1 %d", &code);
  if ((code != 200 && code != 206) || (headerLength == (size_t)bytesReceived)) // if headerLength is bytesReceived, never found the \r\n\r\n
    goto epilogue;

  headerLength += 4;
  pFile->recvBuffer[headerLength-1] = 0; // null terminate the header part
  //udDebugPrintf("Received:\n%s--end--\n", pFile->recvBuffer);

  // Check for a request from the server to close the connection after dealing with this
  closeConnection = udStrstr(pFile->recvBuffer, headerLength, "Connection: close") != nullptr;

  s = udStrstr(pFile->recvBuffer, headerLength, "Content-Length:");
  if (!s)
    goto epilogue;
  contentLength = udStrAtoi64(s + 15);

  if (!pBuffer)
  {
    // Parsing response to the HEAD to get size of overall file
    pFile->length = contentLength;
  }
  else
  {
    // Parsing response to a GET
    if (contentLength != (int64_t)bufferLength)
      udDebugPrintf("contentLength=%lld bufferLength=%lld\n", contentLength, bufferLength);

    bytesReceived -= (int)headerLength;
    memcpy(pBuffer, pFile->recvBuffer + headerLength, bytesReceived);

    while (bytesReceived < bufferLength)
    {
      int extra = recv(pFile->sock, ((char*)pBuffer) + bytesReceived, int(bufferLength - bytesReceived), 0);
      if (extra == SOCKET_ERROR)
        goto epilogue;
      bytesReceived += extra;
    }
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
  struct hostent *hp;
  
  size_t actualHeaderLen;

  // Automatically fail if trying to write to files on http
  result = udR_File_OpenFailure;
  if (flags & (udFOF_Write|udFOF_Create))
    goto epilogue;

  result = udR_MemoryAllocationFailure;
  pFile = udAllocType(udFile_HTTP);
  if (!pFile)
    goto epilogue;
  memset(pFile, 0, sizeof(*pFile));
  pFile->url.Construct();
  pFile->wsInitialised = false;
  pFile->sock = INVALID_SOCKET;

  result = pFile->url.SetURL(pFilename);
  if (result != udR_Success)
    goto epilogue;

  result = udR_File_OpenFailure;
  if (!udStrEqual(pFile->url.GetScheme(), "http"))
    goto epilogue;

#if UDPLATFORM_WINDOWS
  if (WSAStartup(MAKEWORD(2, 2), &pFile->wsaData))
  {
    udDebugPrintf("WSAStartup failed\n");
    goto epilogue;
  }
  pFile->wsInitialised = true;
#endif

  result = udR_File_OpenFailure;
  udDebugPrintf("Resolving %s to ip address...", pFile->url.GetDomain());
  hp = gethostbyname(pFile->url.GetDomain());
  if (!hp)
  {
	udDebugPrintf("gethostbyname failed to resolve url domain\n");
    goto epilogue;
  }
  else
  {
	udDebugPrintf("success\n");
  }
  pFile->server.sin_addr.s_addr = *((unsigned long*)hp->h_addr); // TODO: This is bad. use h_addr_list instead
  pFile->server.sin_family = AF_INET;
  pFile->server.sin_port = htons((u_short) pFile->url.GetPort());

  result = udR_Failure_;
  actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPHeaderString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain());
  if (actualHeaderLen < 0)
    goto epilogue;
  result = udR_File_SocketError;
  udDebugPrintf("Sending:\n%s", pFile->recvBuffer);
  result = udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen);
  if (result != udR_Success)
    goto epilogue;

  result = udFileHandler_HTTPRecvGET(pFile, nullptr, 0);
  if (result != udR_Success)
    goto epilogue;

  pFile->fpRead = udFileHandler_HTTPSeekRead;
  pFile->fpClose = udFileHandler_HTTPClose;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  udFileHandler_HTTPClose((udFile**)&pFile);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of SeekReadHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSeekRead(udFile *pBaseFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, udFileSeekWhence seekWhence, size_t *pActualRead)
{
  udResult result;
  udFile_HTTP *pFile = static_cast<udFile_HTTP *>(pBaseFile);
  size_t offset = pFile->currentOffset;

  switch (seekWhence)
  {
    case udFSW_SeekSet: offset = seekOffset; break;
    case udFSW_SeekCur: offset = pFile->currentOffset + seekOffset; break;
    case udFSW_SeekEnd: offset = pFile->length + seekOffset; break;
  }
  //udDebugPrintf("\nSeekRead: %lld bytes at offset %lld\n", bufferLength, offset);
  size_t actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPGetString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain(), offset, offset + bufferLength-1);
  
  result = udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen);
  if (result != udR_Success)
    goto epilogue;

  result = udFileHandler_HTTPRecvGET(pFile, pBuffer, bufferLength);
  pFile->currentOffset = offset + bufferLength;
  if (pActualRead)
    *pActualRead = bufferLength;

epilogue:

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of CloseHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPClose(udFile **ppFile)
{
  udResult result;
  udFile_HTTP *pFile = nullptr;

  result = udR_InvalidParameter_;
  if (ppFile == nullptr)
    goto epilogue;

  pFile = static_cast<udFile_HTTP *>(*ppFile);
  if (pFile)
  {
    udFileHandler_HTTPCloseSocket(pFile);
#if UDPLATFORM_WINDOWS
    if (pFile->wsInitialised)
    {
      WSACleanup();
      pFile->wsInitialised = false;
    }
#endif
    udFree(pFile);
  }

  result = udR_Success;

epilogue:
  return result;
}


