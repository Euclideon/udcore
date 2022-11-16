#ifndef UDSOCKET_H
#define UDSOCKET_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Paul Fox, October 2018
//
// TCP and TLS socket implementation
//

#include "udMath.h"

// This module is based heavily on the one from lorCore, http://github.com/LORgames/lorcore/

struct udSocket;
struct udSocketSet;

enum udSocketConnectionFlags
{
  udSCF_None = 0,
  udSCF_IsServer = 1 << 0,
  udSCF_UseTLS = 1 << 1
};
inline udSocketConnectionFlags operator|(udSocketConnectionFlags a, udSocketConnectionFlags b) { return (udSocketConnectionFlags)(((int)a) | ((int)b)); }

// Initialisation
udResult udSocket_InitSystem();
void udSocket_DeinitSystem();

// API for each socket
udResult udSocket_Open(udSocket **ppSocket, const char *pAddress, uint32_t port, udSocketConnectionFlags flags = udSCF_None, const char *pPrivateKey = nullptr, const char *pPublicCertificate = nullptr);
void udSocket_Close(udSocket **ppSocket);
bool udSocket_IsValidSocket(udSocket *pSocket);

udResult udSocket_SendData(udSocket *pSocket, const uint8_t *pBytes, int64_t totalBytes, int64_t *pActualSent = nullptr);
udResult udSocket_ReceiveData(udSocket *pSocket, uint8_t *pBytes, int64_t bufferSize, int64_t *pActualReceived = nullptr);

// These functions accept a client socket from a server socket. PartA can only be done from a single thread but PartB (for the same client socket) can be farmed out in parallel
bool udSocket_ServerAcceptClientPartA(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address = nullptr);
bool udSocket_ServerAcceptClientPartB(udSocket *pClientSocket);

// This helper internally does PartA and PartB
bool udSocket_ServerAcceptClient(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address = nullptr);

// API for wrapping select
void udSocketSet_Create(udSocketSet **ppSocketSet);
void udSocketSet_Destroy(udSocketSet **ppSocketSet);
void udSocketSet_EmptySet(udSocketSet *pSocketSet);
void udSocketSet_AddSocket(udSocketSet *pSocketSet, udSocket *pSocket);
bool udSocketSet_IsInSet(udSocketSet *pSocketSet, udSocket *pSocket);
int udSocketSet_Select(size_t timeoutMilliseconds, udSocketSet *pReadSocketSet, udSocketSet *pWriteSocketSet = nullptr, udSocketSet *pExceptionSocketSet = nullptr);

#endif // UDSOCKET_H
