#ifndef udSocket_h__
#define udSocket_h__

#include "udMath.h"

// This module is based heavily on the one from lorCore, http://github.com/LORgames/lorcore/

struct udSocket;
struct udSocketSet;

enum udSocketConnectionFlags
{
  udSCFNone = 0,
  udSCFIsServer = 1 << 0,
  udSCFUseTLS = 1 << 1
};
inline udSocketConnectionFlags operator |(udSocketConnectionFlags a, udSocketConnectionFlags b) { return (udSocketConnectionFlags)(((int)a) | ((int)b)); }

// Initialisation
udResult udSocket_InitSystem();
void udSocket_DeinitSystem();

// API for each socket
udResult udSocket_Open(udSocket **ppSocket, const char *pAddress, uint32_t port, udSocketConnectionFlags flags = udSCFNone, const char *pPrivateKey = nullptr, const char *pPublicCertificate = nullptr);
void udSocket_Close(udSocket **ppSocket);
bool udSocket_IsValidSocket(udSocket *pSocket);

udResult udSocket_SendData(udSocket *pSocket, const uint8_t *pBytes, int64_t totalBytes, int64_t *pActualSent = nullptr);
udResult udSocket_ReceiveData(udSocket *pSocket, uint8_t *pBytes, int64_t bufferSize, int64_t *pActualReceived = nullptr);

bool udSocket_ServerAcceptClient(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address = nullptr);

// API for wrapping select
void udSocketSet_Create(udSocketSet **ppSocketSet);
void udSocketSet_Destroy(udSocketSet **ppSocketSet);
void udSocketSet_EmptySet(udSocketSet *pSocketSet);
void udSocketSet_AddSocket(udSocketSet *pSocketSet, udSocket *pSocket);
bool udSocketSet_IsInSet(udSocketSet *pSocketSet, udSocket *pSocket);
int udSocketSet_Select(size_t timeoutMilliseconds, udSocketSet *pReadSocketSet, udSocketSet *pWriteSocketSet = nullptr, udSocketSet *pExceptionSocketSet = nullptr);

#endif // udSocket_h__
