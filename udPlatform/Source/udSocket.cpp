#include "udSocket.h"
#include "udPlatformUtil.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

#if UDPLATFORM_WINDOWS
#include <windows.h>
#include <Wincrypt.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#endif

#if UDPLATFORM_OSX
#include <Security/Security.h>
#endif

#ifndef INVALID_SOCKET //Some platforms don't have these defined
typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif //INVALID_SOCKET

static struct
{
  int loadCount = 0; //ref count for loaded certs; must be zero
  mbedtls_entropy_context entropy;
  mbedtls_x509_crt certificateChain;
} g_sharedSocketData;

struct udSocket
{
  SOCKET basicSocket;
  bool isServer;
  bool isSecure;

  struct
  {
    mbedtls_net_context socketContext; //The actual socket
    mbedtls_ctr_drbg_context ctr_drbg; //The encryption context
    mbedtls_ssl_context ssl; //The socket to encryption context
    mbedtls_ssl_config conf;

    //Additional server things
    mbedtls_x509_crt certificateServer;
    mbedtls_pk_context publicKey;
  } tlsClient;
};

struct udSocketSet
{
  fd_set set;
  SOCKET highestSocketHandle;
};

bool udSocket_LoadCACerts()
{
  mbedtls_entropy_init(&g_sharedSocketData.entropy);
  mbedtls_x509_crt_init(&g_sharedSocketData.certificateChain);

  bool wasLoaded = false;

#if UDPLATFORM_WINDOWS
  HCERTSTORE store = CertOpenSystemStoreA(0, "Root");
  PCCERT_CONTEXT cert = nullptr;

  if (store != nullptr)
  {
    wasLoaded = true;

    cert = CertEnumCertificatesInStore(store, cert);
    while (cert != nullptr)
    {
      mbedtls_x509_crt_parse_der(&g_sharedSocketData.certificateChain, (unsigned char *)cert->pbCertEncoded, cert->cbCertEncoded);
      cert = CertEnumCertificatesInStore(store, cert);
    }
    CertCloseStore(store, 0);
  }
#elif UDPLATFORM_OSX
  CFMutableDictionaryRef search;
  CFArrayRef result;
  SecKeychainRef keychain;
  SecCertificateRef item;
  CFDataRef dat;

  // Load keychain
  if (SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain", &keychain) != errSecSuccess)
    return false;

  // Search for certificates
  search = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
  CFDictionarySetValue(search, kSecClass, kSecClassCertificate);
  CFDictionarySetValue(search, kSecMatchLimit, kSecMatchLimitAll);
  CFDictionarySetValue(search, kSecReturnRef, kCFBooleanTrue);
  CFDictionarySetValue(search, kSecMatchSearchList, CFArrayCreate(NULL, (const void **)&keychain, 1, NULL));

  if (SecItemCopyMatching(search, (CFTypeRef*)&result) == errSecSuccess)
  {
    CFIndex n = CFArrayGetCount(result);
    for (CFIndex i = 0; i < n; i++)
    {
      item = (SecCertificateRef)CFArrayGetValueAtIndex(result, i);

      // Get certificate in DER format
      dat = SecCertificateCopyData(item);
      if (dat)
      {
        mbedtls_x509_crt_parse_der(&g_sharedSocketData.certificateChain, (unsigned char*)CFDataGetBytePtr(dat), CFDataGetLength(dat));
        CFRelease(dat);
      }
    }

    wasLoaded = true;
  }

  CFRelease(keychain);
#else
  //These are the recommended places from the Go-Lang documentation
  const char *certFiles[] = {
    "/etc/ssl/certs/ca-certificates.crt",                // Debian/Ubuntu/Gentoo etc.
    "/etc/pki/tls/certs/ca-bundle.crt",                  // Fedora/RHEL 6
    "/etc/ssl/ca-bundle.pem",                            // OpenSUSE
    "/etc/pki/tls/cacert.pem",                           // OpenELEC
    "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", // CentOS/RHEL 7
  };

  const char *certFolders[] = {
    "/etc/ssl/certs",               // SLES10/SLES11
    "/system/etc/security/cacerts", // Android
    "/usr/local/share/certs",       // FreeBSD
    "/etc/pki/tls/certs",           // Fedora/RHEL
    "/etc/openssl/certs",           // NetBSD
  };

  for (size_t i = 0; i < UDARRAYSIZE(certFiles) && !wasLoaded; ++i)
  {
    if (udFileExists(certFiles[i]) == udR_Success)
    {
      mbedtls_x509_crt_parse_file(&g_sharedSocketData.certificateChain, certFiles[i]);
      wasLoaded = true;
    }
  }

  udFindDir *pDir = nullptr;
  for (size_t i = 0; i < UDARRAYSIZE(certFolders) && !wasLoaded; ++i)
  {
    if (udOpenDir(&pDir, certFolders[i]) == udR_Success)
      continue;

    do
    {
      mbedtls_x509_crt_parse_file(&g_sharedSocketData.certificateChain, certFiles[i]);
      wasLoaded = true;
    } while (udReadDir(pDir) == udR_Success);

    udCloseDir(&pDir);
  }
#endif

  return wasLoaded;
}

bool udSocket_InitSystem()
{
  //LOAD CA CERTIFICATES
  if (g_sharedSocketData.loadCount == 0)
  {
    if (!udSocket_LoadCACerts())
      return false;

#if UDPLATFORM_WINDOWS
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  }
  ++g_sharedSocketData.loadCount; //Increase refcount

  return true;
}

bool udSocket_DeinitSystem()
{
  --g_sharedSocketData.loadCount;
  if (g_sharedSocketData.loadCount == 0)
  {
    mbedtls_entropy_free(&g_sharedSocketData.entropy);
    mbedtls_x509_crt_free(&g_sharedSocketData.certificateChain);

#if UDPLATFORM_WINDOWS
    return (WSACleanup() == 0);
#endif
  }

  return true;
}

int udSocket_GetErrorCode()
{
#if UDPLATFORM_WINDOWS
  return WSAGetLastError();
#else
  return errno;
#endif
}

bool udSocket_IsValidSocket(udSocket *pSocket)
{
  if (pSocket->isSecure)
    return (pSocket->tlsClient.socketContext.fd != INVALID_SOCKET && pSocket->tlsClient.socketContext.fd != 0);
  else
    return (pSocket->basicSocket != INVALID_SOCKET);
}


static void udSocketMBEDDebug(void * /*pUserData*/, int /*level*/, const char *file, int line, const char *str)
{
  udDebugPrintf("%s:%04d: %s\n", file, line, str);
}

bool udSocket_Open(udSocket **ppSocket, const char *pAddress, uint32_t port, udSocketConnectionFlags flags, const char *pPrivateKey /*= nullptr*/, const char *pPublicCertificate /*= nullptr*/)
{
  if (ppSocket == nullptr)
    return false;

  udDebugPrintf("Socket init (%s:%d) flags=%d", pAddress, port, flags);
  bool openSuccess = false;
  int retVal;

  udSocket *pSocket = udAllocType(udSocket, 1, udAF_Zero);
  addrinfo *pOutAddr = nullptr;

  bool isServer = (flags & udSCFIsServer) > 0;
  bool isSecure = (flags & udSCFUseTLS) > 0;

  pSocket->isServer = isServer;
  pSocket->isSecure = isSecure;

  if (isSecure)
  {
    //Init everything
    mbedtls_net_init(&pSocket->tlsClient.socketContext);
    mbedtls_ssl_init(&pSocket->tlsClient.ssl);
    mbedtls_ssl_config_init(&pSocket->tlsClient.conf);
    mbedtls_ctr_drbg_init(&pSocket->tlsClient.ctr_drbg);
    mbedtls_x509_crt_init(&pSocket->tlsClient.certificateServer);

    //Set up encryption things
    retVal = mbedtls_ctr_drbg_seed(&pSocket->tlsClient.ctr_drbg, mbedtls_entropy_func, &g_sharedSocketData.entropy, nullptr, 0);
    if (retVal != 0)
    {
      udDebugPrintf(" failed! mbedtls_ctr_drbg_seed returned %d\n", retVal);
      goto epilogue;
    }

    //Setup the port string
    char portStr[6];
    udSprintf(portStr, 6, "%d", port);

    //Branch for server/client differences
    if (isServer)
    {
      if (pPrivateKey == nullptr || pPublicCertificate == nullptr)
      {
        udDebugPrintf(" failed! Certificate and private key cannot be null if running a secure server!\n");
        goto epilogue;
      }

      //Set up server certificate
      retVal = mbedtls_x509_crt_parse(&pSocket->tlsClient.certificateServer, (const unsigned char *)pPublicCertificate, udStrlen(pPublicCertificate)+1);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_x509_crt_parse returned %d\n", retVal);
        goto epilogue;
      }

      //Set up public key
      mbedtls_pk_init(&pSocket->tlsClient.publicKey);
      retVal = mbedtls_pk_parse_key(&pSocket->tlsClient.publicKey, (const unsigned char *)pPrivateKey, udStrlen(pPrivateKey)+1, NULL, 0);
      if (retVal != 0)
      {
        udDebugPrintf(" failed!  mbedtls_pk_parse_key returned %d\n", retVal);
        goto epilogue;
      }

      //Bind
      udDebugPrintf(" TLS Socket Bind on %s:%s", pAddress, portStr);
      retVal = mbedtls_net_bind(&pSocket->tlsClient.socketContext, pAddress, portStr, MBEDTLS_NET_PROTO_TCP);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_net_bind returned %d\n", retVal);
        goto epilogue;
      }

      //Set up config stuff for server
      retVal = mbedtls_ssl_config_defaults(&pSocket->tlsClient.conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_config_defaults returned %d\n", retVal);
        goto epilogue;
      }

      //Link certificate chains
      mbedtls_ssl_conf_ca_chain(&pSocket->tlsClient.conf, &g_sharedSocketData.certificateChain, NULL);
      retVal = mbedtls_ssl_conf_own_cert(&pSocket->tlsClient.conf, &pSocket->tlsClient.certificateServer, &pSocket->tlsClient.publicKey);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_conf_own_cert returned %d\n", retVal);
        goto epilogue;
      }
    }
    else //Is Client
    {
      //Connect
      retVal = mbedtls_net_connect(&pSocket->tlsClient.socketContext, pAddress, portStr, MBEDTLS_NET_PROTO_TCP);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_net_connect returned %d\n", retVal);
        goto epilogue;
      }

      //Config stuff
      retVal = mbedtls_ssl_config_defaults(&pSocket->tlsClient.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_config_defaults returned %d\n", retVal);
        goto epilogue;
      }

      //TODO: Has to be removed before we ship...
      mbedtls_ssl_conf_authmode(&pSocket->tlsClient.conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    //Required
    mbedtls_ssl_conf_rng(&pSocket->tlsClient.conf, mbedtls_ctr_drbg_random, &pSocket->tlsClient.ctr_drbg);
    mbedtls_ssl_conf_dbg(&pSocket->tlsClient.conf, udSocketMBEDDebug, stdout);

    retVal = mbedtls_ssl_setup(&pSocket->tlsClient.ssl, &pSocket->tlsClient.conf);
    if (retVal != 0)
    {
      udDebugPrintf(" failed! mbedtls_ssl_setup returned %d\n", retVal);
      goto epilogue;
    }

    if (!isServer)
    {
      retVal = mbedtls_ssl_set_hostname(&pSocket->tlsClient.ssl, pAddress);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_set_hostname returned %d\n", retVal);
        goto epilogue;
      }

      mbedtls_ssl_set_bio(&pSocket->tlsClient.ssl, &pSocket->tlsClient.socketContext, mbedtls_net_send, mbedtls_net_recv, NULL);

      do
      {
        retVal = mbedtls_ssl_handshake(&pSocket->tlsClient.ssl);
        if (retVal == 0)
          break;

        if (retVal != MBEDTLS_ERR_SSL_WANT_READ && retVal != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          udDebugPrintf(" failed! mbedtls_ssl_handshake returned -0x%x\n", -retVal);
          goto epilogue;
        }
      } while (retVal != 0);
    }
  }
  else
  {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char buffer[6];
    udSprintf(buffer, sizeof(buffer), "%d", port);
    retVal = getaddrinfo(pAddress, buffer, &hints, &pOutAddr);
    if (retVal != 0)
      goto epilogue;

    pSocket->basicSocket = socket(pOutAddr->ai_family, pOutAddr->ai_socktype, pOutAddr->ai_protocol);

    if (!udSocket_IsValidSocket(pSocket))
      goto epilogue;

    if (isServer)
    {
#if !UDPLATFORM_WINDOWS
      // Allow SOCKET to reuse an address that *was* bound and is *not* currently in use.
      // By default, Windows does this.
      // NOTE: Running this on Windows will allow multiple sockets to be bound to the
      // same address which is completely different behaviour.
      {
        int enable = 1;
        retVal = setsockopt(pSocket->basicSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        if (retVal == SOCKET_ERROR)
          goto epilogue;
      }
#endif

      pSocket->isServer = true;

      retVal = bind(pSocket->basicSocket, pOutAddr->ai_addr, (int)pOutAddr->ai_addrlen);
      if (retVal == SOCKET_ERROR)
        goto epilogue;

      retVal = listen(pSocket->basicSocket, SOMAXCONN);
      if (retVal == SOCKET_ERROR)
        goto epilogue;
    }
    else
    {
      retVal = connect(pSocket->basicSocket, pOutAddr->ai_addr, (int)pOutAddr->ai_addrlen);
      if (retVal == SOCKET_ERROR)
        goto epilogue;
    }
  }

  openSuccess = true;

epilogue:
  if (pOutAddr != nullptr)
    freeaddrinfo(pOutAddr);

  if (!openSuccess)
    udSocket_Close(&pSocket);

  *ppSocket = pSocket;

  udDebugPrintf("\t...Connection %s!\n", openSuccess ? "Success" : "Failed");
  return openSuccess;
}

bool udSocket_Close(udSocket **ppSocket)
{
  if (*ppSocket == nullptr)
    return true;

  if ((*ppSocket)->isSecure)
  {
    udSocket *pSocket = *ppSocket;
    mbedtls_net_free(&pSocket->tlsClient.socketContext);
    mbedtls_ssl_free(&pSocket->tlsClient.ssl);
    mbedtls_ssl_config_free(&pSocket->tlsClient.conf);
    mbedtls_ctr_drbg_free(&pSocket->tlsClient.ctr_drbg);

    mbedtls_x509_crt_free(&pSocket->tlsClient.certificateServer);
    mbedtls_pk_free(&pSocket->tlsClient.publicKey);
  }
  else
  {
#if UDPLATFORM_WINDOWS
    closesocket((*ppSocket)->basicSocket);
#else
    close((*ppSocket)->basicSocket);
#endif
  }

  udFree(*ppSocket);
  *ppSocket = nullptr;

  return true;
}

int64_t udSocket_SendData(udSocket *pSocket, const uint8_t *pBytes, int64_t totalBytes)
{
  if (pSocket == nullptr || pBytes == nullptr || totalBytes == 0)
    return false;

  if (pSocket->isSecure)
  {
    int status = mbedtls_ssl_write(&pSocket->tlsClient.ssl, pBytes, totalBytes);

    if (status < 0)
      return -1; //TODO: this is really important to close socket somehow

    return totalBytes;
  }
  else
  {
    return send(pSocket->basicSocket, (const char*)pBytes, (int)totalBytes, 0);
  }
}

int64_t udSocket_ReceiveData(udSocket *pSocket, uint8_t *pBytes, int64_t bufferSize)
{
  if (pSocket == nullptr || pBytes == nullptr || bufferSize == 0 || pSocket->isServer)
    return 0;

  if (pSocket->isSecure)
    return mbedtls_ssl_read(&pSocket->tlsClient.ssl, pBytes, bufferSize);
  else
    return recv(pSocket->basicSocket, (char*)pBytes, (int)bufferSize, 0);
}

bool udSocket_ServerAcceptClient(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address /*= nullptr*/)
{
  if (ppClientSocket == nullptr || pServerSocket == nullptr || !pServerSocket->isServer)
    return false;

  *ppClientSocket = nullptr;

  uint8_t clientIP[64];
  size_t clientBytesReturned;
  if (pIPv4Address != nullptr)
    *pIPv4Address = 0;

  if (pServerSocket->isSecure)
  {
    mbedtls_net_context clientContext;

    //Accept the client
    int retVal = mbedtls_net_accept(&pServerSocket->tlsClient.socketContext, &clientContext, clientIP, sizeof(clientIP), &clientBytesReturned);
    if (retVal != 0)
    {
      udDebugPrintf("Failed somehow to accept tls client?\n");
      return false;
    }

    if(pIPv4Address != nullptr && clientBytesReturned == 4)
      *pIPv4Address = (clientIP[0] << 24) | (clientIP[1] << 16) | (clientIP[2] << 8) | (clientIP[3]);

    (*ppClientSocket) = udAllocType(udSocket, 1, udAF_Zero);
    udSocket *pClientSocket = *ppClientSocket;

    pClientSocket->isSecure = true;
    pClientSocket->tlsClient.socketContext = clientContext;

    //Handle SSL
    /* Make sure memory references are valid */
    mbedtls_ssl_init(&pClientSocket->tlsClient.ssl);

    udDebugPrintf("  Setting up SSL/TLS data");

    //Copy the config?
    retVal = mbedtls_ssl_setup(&pClientSocket->tlsClient.ssl, &pServerSocket->tlsClient.conf);
    if (retVal != 0)
    {
      udDebugPrintf("  failed! mbedtls_ssl_setup returned -0x%04x\n", -retVal);
      goto sslfail;
    }

    mbedtls_ssl_set_bio(&pClientSocket->tlsClient.ssl, &pClientSocket->tlsClient.socketContext, mbedtls_net_send, mbedtls_net_recv, NULL);

    //Handshake

    do
    {
      retVal = mbedtls_ssl_handshake(&pClientSocket->tlsClient.ssl);
      if (retVal == 0)
        break;

      if (retVal != MBEDTLS_ERR_SSL_WANT_READ && retVal != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
        udDebugPrintf(" failed! mbedtls_ssl_handshake returned -0x%x\n", -retVal);
        goto sslfail;
      }
    } while (retVal != 0);

    udDebugPrintf("  Client accepted and secured\n");
    return true;

  sslfail:
    //Cleanup

    return false;
  }
  else
  {
    sockaddr_storage clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(pServerSocket->basicSocket, (sockaddr*)&clientAddr, &clientAddrSize);

    if (clientSocket != INVALID_SOCKET)
    {
      (*ppClientSocket) = udAllocType(udSocket, 1, udAF_Zero);
      (*ppClientSocket)->basicSocket = clientSocket;

      if (pIPv4Address != nullptr)
      {
        sockaddr_in *pAddrV4 = (sockaddr_in*)&clientAddr;
        memcpy(clientIP, &pAddrV4->sin_addr.s_addr, sizeof(pAddrV4->sin_addr.s_addr));
        *pIPv4Address = (clientIP[0] << 24) | (clientIP[1] << 16) | (clientIP[2] << 8) | (clientIP[3]);
      }

      return true;
    }
  }

  return false;
}

//SELECT API
void udSocketSet_Create(udSocketSet **ppSocketSet)
{
  if (ppSocketSet == nullptr)
    return;

  *ppSocketSet = udAllocType(udSocketSet, 1, udAF_Zero);
  udSocketSet_EmptySet(*ppSocketSet);
}

void udSocketSet_Destroy(udSocketSet **ppSocketSet)
{
  if (ppSocketSet != nullptr && *ppSocketSet != nullptr)
    udFree(*ppSocketSet);
}

void udSocketSet_EmptySet(udSocketSet *pSocketSet)
{
  if (pSocketSet == nullptr)
    return;

  FD_ZERO(&pSocketSet->set);
  pSocketSet->highestSocketHandle = 0;
}

void udSocketSet_AddSocket(udSocketSet *pSocketSet, udSocket *pSocket)
{
  if (pSocket == nullptr || pSocketSet == nullptr)
    return;

  SOCKET socketHandle = pSocket->isSecure ? pSocket->tlsClient.socketContext.fd : pSocket->basicSocket;

  pSocketSet->highestSocketHandle = udMax(socketHandle, pSocketSet->highestSocketHandle);
  FD_SET(socketHandle, &pSocketSet->set);
}

bool udSocketSet_IsInSet(udSocketSet *pSocketSet, udSocket *pSocket)
{
  if (pSocketSet == nullptr || pSocket == nullptr)
    return false;

  SOCKET socketHandle = pSocket->isSecure ? pSocket->tlsClient.socketContext.fd : pSocket->basicSocket;

  if (pSocketSet->highestSocketHandle < socketHandle)
    return false;

  return (FD_ISSET(socketHandle, &pSocketSet->set) != 0);
}

int udSocketSet_Select(size_t timeoutMilliseconds, udSocketSet *pReadSocketSet, udSocketSet *pWriteSocketSet, udSocketSet *pExceptSocketSet)
{
  struct timeval tv;
  tv.tv_sec = (int32_t)(timeoutMilliseconds / 1000);
  tv.tv_usec = (int32_t)(timeoutMilliseconds % 1000);

  SOCKET nfds = 0;
  fd_set *pReadSet = nullptr;
  fd_set *pWriteSet = nullptr;
  fd_set *pExceptSet = nullptr;

  if (pReadSocketSet != nullptr)
  {
    nfds = pReadSocketSet->highestSocketHandle;
    pReadSet = &pReadSocketSet->set;
  }

  if (pWriteSocketSet != nullptr)
  {
    nfds = udMax(nfds, pWriteSocketSet->highestSocketHandle);
    pWriteSet = &pWriteSocketSet->set;
  }

  if (pExceptSocketSet != nullptr)
  {
    nfds = udMax(nfds, pExceptSocketSet->highestSocketHandle);
    pExceptSet = &pExceptSocketSet->set;
  }

  return select((int32_t)nfds + 1, pReadSet, pWriteSet, pExceptSet, &tv);
}
