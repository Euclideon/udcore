#include "udSocket.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udCrypto.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

#include <atomic>

#if UDPLATFORM_WINDOWS
# include <windows.h>
# include <Wincrypt.h>
# include <winsock2.h>
# include <Ws2tcpip.h>
# pragma comment(lib, "ws2_32.lib")
# pragma comment(lib, "Crypt32.lib")
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
# include <unistd.h> /* Needed for close() */
# if UDPLATFORM_EMSCRIPTEN
#  include <sys/select.h>
#  include <errno.h>
# else
#  include <sys/errno.h>
# endif
#endif

#if UDPLATFORM_OSX
# include <Security/Security.h>
#endif

#ifndef INVALID_SOCKET //Some platforms don't have these defined
  typedef int SOCKET;
# define INVALID_SOCKET  (SOCKET)(~0)
# define SOCKET_ERROR            (-1)
#endif //INVALID_SOCKET

struct udSocketSharedData
{
  static std::atomic<int32_t> loadCount; // ref count for loaded certs; must be zero
  static std::atomic<int32_t> initialised; // set to 1 once initialisation is complete
  static mbedtls_entropy_context entropy;
  static mbedtls_x509_crt certificateChain;
};

std::atomic<int32_t> udSocketSharedData::loadCount(0);
std::atomic<int32_t> udSocketSharedData::initialised(0);
mbedtls_entropy_context udSocketSharedData::entropy;
mbedtls_x509_crt udSocketSharedData::certificateChain;

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

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
udResult udSocket_LoadCACerts()
{
  udResult result;
  bool certParsed = false;

  mbedtls_entropy_init(&udSocketSharedData::entropy);
  mbedtls_x509_crt_init(&udSocketSharedData::certificateChain);

  // Open a scope to prevent various initialisation warnings
  {
#if UDPLATFORM_WINDOWS
# if UDPLATFORM_UWP
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, "Root");
# else
    HCERTSTORE store = CertOpenSystemStoreA(0, "Root");
# endif
    UD_ERROR_NULL(store, udR_Failure);
    for (PCCERT_CONTEXT cert = CertEnumCertificatesInStore(store, nullptr); cert; cert = CertEnumCertificatesInStore(store, cert))
    {
      if (mbedtls_x509_crt_parse_der(&udSocketSharedData::certificateChain, (unsigned char *)cert->pbCertEncoded, cert->cbCertEncoded) == 0)
        certParsed = true;
    }
    CertCloseStore(store, 0);
#elif UDPLATFORM_OSX
    CFMutableDictionaryRef search;
    CFArrayRef cfResult;
    SecKeychainRef keychain;
    SecCertificateRef item;
    CFDataRef dat;

    // Load keychain
    UD_ERROR_IF(SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain", &keychain) != errSecSuccess, udR_Failure);

    // Search for certificates
    search = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
    CFDictionarySetValue(search, kSecClass, kSecClassCertificate);
    CFDictionarySetValue(search, kSecMatchLimit, kSecMatchLimitAll);
    CFDictionarySetValue(search, kSecReturnRef, kCFBooleanTrue);
    CFDictionarySetValue(search, kSecMatchSearchList, CFArrayCreate(NULL, (const void **)&keychain, 1, NULL));

    if (SecItemCopyMatching(search, (CFTypeRef*)&cfResult) == errSecSuccess)
    {
      CFIndex n = CFArrayGetCount(cfResult);
      for (CFIndex i = 0; i < n; i++)
      {
        item = (SecCertificateRef)CFArrayGetValueAtIndex(cfResult, i);

        // Get certificate in DER format
        dat = SecCertificateCopyData(item);
        if (dat)
        {
          if (mbedtls_x509_crt_parse_der(&udSocketSharedData::certificateChain, (unsigned char*)CFDataGetBytePtr(dat), CFDataGetLength(dat)) == 0)
            certParsed = true;
          CFRelease(dat);
        }
      }
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

    for (size_t i = 0; i < UDARRAYSIZE(certFiles) && !certParsed; ++i)
    {
      if (udFileExists(certFiles[i]) == udR_Success)
      {
        if (mbedtls_x509_crt_parse_file(&udSocketSharedData::certificateChain, certFiles[i]) == 0)
          certParsed = true;
      }
    }

    udFindDir *pDir = nullptr;
    for (size_t i = 0; i < UDARRAYSIZE(certFolders) && !certParsed; ++i)
    {
      if (udOpenDir(&pDir, certFolders[i]) == udR_Success)
        continue;

      do
      {
        if (mbedtls_x509_crt_parse_file(&udSocketSharedData::certificateChain, certFiles[i]) == 0)
          certParsed = true;
      } while (udReadDir(pDir) == udR_Success);

      udCloseDir(&pDir);
    }
#endif
  }
  UD_ERROR_IF(!certParsed, udR_Success); // TODO: Consider if NothingToDo is more appropriate
  result = udR_Success;

epilogue:
  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
udResult udSocket_InitSystem()
{
  udResult result;

  // LOAD CA CERTIFICATES
  int32_t loadCount = udSocketSharedData::loadCount++;
  if (loadCount == 0)
  {
    UD_ERROR_CHECK(udCrypto_Init());
    UD_ERROR_CHECK(udSocket_LoadCACerts());

#if UDPLATFORM_WINDOWS
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
    udSocketSharedData::initialised = 1;
  }
  else
  {
    // If another thread has begun initialisation, wait for it to complete
    while (!udSocketSharedData::initialised.load())
      udSleep(1);
  }

  result = udR_Success;
epilogue:

  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocket_DeinitSystem()
{
  int32_t loadCount = --udSocketSharedData::loadCount;
  if (loadCount == 0)
  {
    udSocketSharedData::initialised = 0;
    mbedtls_entropy_free(&udSocketSharedData::entropy);
    mbedtls_x509_crt_free(&udSocketSharedData::certificateChain);
    udCrypto_Deinit();

#if UDPLATFORM_WINDOWS
    WSACleanup();
#endif
  }
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
int udSocket_GetErrorCode()
{
#if UDPLATFORM_WINDOWS
  return WSAGetLastError();
#else
  return errno;
#endif
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
bool udSocket_IsValidSocket(udSocket *pSocket)
{
  if (!pSocket)
    return false;
  else if (pSocket->isSecure)
    return (pSocket->tlsClient.socketContext.fd != INVALID_SOCKET && pSocket->tlsClient.socketContext.fd != 0);
  else
    return (pSocket->basicSocket != INVALID_SOCKET);
}


// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
static void udSocketmbedDebug(void * /*pUserData*/, int /*level*/, const char *file, int line, const char *str)
{
  udDebugPrintf("%s:%04d: %s\n", file, line, str);
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
udResult udSocket_Open(udSocket **ppSocket, const char *pAddress, uint32_t port, udSocketConnectionFlags flags, const char *pPrivateKey /*= nullptr*/, const char *pPublicCertificate /*= nullptr*/)
{
  udResult result;
  udSocket *pSocket = nullptr;
  addrinfo *pOutAddr = nullptr;
  int retVal;

  udDebugPrintf("Socket init (%s:%d) flags=%d", pAddress, port, flags);
  UD_ERROR_NULL(ppSocket, udR_InvalidParameter);
  pSocket = udAllocType(udSocket, 1, udAF_Zero);
  UD_ERROR_NULL(pSocket, udR_MemoryAllocationFailure);

  pSocket->isServer = (flags & udSCF_IsServer) > 0;
  pSocket->isSecure = (flags & udSCF_UseTLS) > 0;

  if (pSocket->isSecure)
  {
    //Init everything
    mbedtls_net_init(&pSocket->tlsClient.socketContext);
    mbedtls_ssl_init(&pSocket->tlsClient.ssl);
    mbedtls_ssl_config_init(&pSocket->tlsClient.conf);
    mbedtls_ctr_drbg_init(&pSocket->tlsClient.ctr_drbg);
    mbedtls_x509_crt_init(&pSocket->tlsClient.certificateServer);

    //Set up encryption things
    retVal = mbedtls_ctr_drbg_seed(&pSocket->tlsClient.ctr_drbg, mbedtls_entropy_func, &udSocketSharedData::entropy, nullptr, 0);
    if (retVal != 0)
    {
      udDebugPrintf(" failed! mbedtls_ctr_drbg_seed returned %d\n", retVal);
      UD_ERROR_SET(udR_InternalCryptoError);
    }

    // Setup the port string
    char portStr[6];
    udSprintf(portStr, "%d", port);

    // Branch for server/client differences
    if (pSocket->isServer)
    {
      if (pPrivateKey == nullptr || pPublicCertificate == nullptr)
      {
        udDebugPrintf(" failed! Certificate and private key cannot be null if running a secure server!\n");
        UD_ERROR_SET(udR_InvalidConfiguration);
     }

      // Set up server certificate
      retVal = mbedtls_x509_crt_parse(&pSocket->tlsClient.certificateServer, (const unsigned char *)pPublicCertificate, udStrlen(pPublicCertificate)+1);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_x509_crt_parse returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // Set up public key
      mbedtls_pk_init(&pSocket->tlsClient.publicKey);
      retVal = mbedtls_pk_parse_key(&pSocket->tlsClient.publicKey, (const unsigned char *)pPrivateKey, udStrlen(pPrivateKey)+1, NULL, 0);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_pk_parse_key returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // Bind
      udDebugPrintf(" TLS Socket Bind on %s:%s", pAddress, portStr);
      retVal = mbedtls_net_bind(&pSocket->tlsClient.socketContext, pAddress, portStr, MBEDTLS_NET_PROTO_TCP);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_net_bind returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // Set up config stuff for server
      retVal = mbedtls_ssl_config_defaults(&pSocket->tlsClient.conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_config_defaults returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // Link certificate chains
      mbedtls_ssl_conf_ca_chain(&pSocket->tlsClient.conf, &udSocketSharedData::certificateChain, NULL);
      retVal = mbedtls_ssl_conf_own_cert(&pSocket->tlsClient.conf, &pSocket->tlsClient.certificateServer, &pSocket->tlsClient.publicKey);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_conf_own_cert returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }
    }
    else // Is Client
    {
      // Connect
      retVal = mbedtls_net_connect(&pSocket->tlsClient.socketContext, pAddress, portStr, MBEDTLS_NET_PROTO_TCP);
      UD_ERROR_IF(retVal == MBEDTLS_ERR_NET_SOCKET_FAILED || retVal == MBEDTLS_ERR_NET_CONNECT_FAILED, udR_SocketError);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_net_connect returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // Config stuff
      retVal = mbedtls_ssl_config_defaults(&pSocket->tlsClient.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_config_defaults returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
      }

      // TODO: Has to be removed before we ship...
      mbedtls_ssl_conf_authmode(&pSocket->tlsClient.conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    //Required
    mbedtls_ssl_conf_rng(&pSocket->tlsClient.conf, mbedtls_ctr_drbg_random, &pSocket->tlsClient.ctr_drbg);
    mbedtls_ssl_conf_dbg(&pSocket->tlsClient.conf, udSocketmbedDebug, stdout);

    retVal = mbedtls_ssl_setup(&pSocket->tlsClient.ssl, &pSocket->tlsClient.conf);
    if (retVal != 0)
    {
      udDebugPrintf(" failed! mbedtls_ssl_setup returned %d\n", retVal);
      UD_ERROR_SET(udR_InternalCryptoError);
    }

    if (!pSocket->isServer)
    {
      retVal = mbedtls_ssl_set_hostname(&pSocket->tlsClient.ssl, pAddress);
      if (retVal != 0)
      {
        udDebugPrintf(" failed! mbedtls_ssl_set_hostname returned %d\n", retVal);
        UD_ERROR_SET(udR_InternalCryptoError);
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
          UD_ERROR_SET(udR_InternalCryptoError);
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
    udSprintf(buffer, "%d", port);
    retVal = getaddrinfo(pAddress, buffer, &hints, &pOutAddr);
    UD_ERROR_IF(retVal != 0, udR_SocketError);

    pSocket->basicSocket = socket(pOutAddr->ai_family, pOutAddr->ai_socktype, pOutAddr->ai_protocol);
    UD_ERROR_IF(!udSocket_IsValidSocket(pSocket), udR_SocketError);

    if (pSocket->isServer)
    {
#if !UDPLATFORM_WINDOWS
      // Allow SOCKET to reuse an address that *was* bound and is *not* currently in use.
      // By default, Windows does this.
      // NOTE: Running this on Windows will allow multiple sockets to be bound to the
      // same address which is completely different behaviour.
      {
        int enable = 1;
        retVal = setsockopt(pSocket->basicSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        UD_ERROR_IF(retVal == SOCKET_ERROR, udR_SocketError);
      }
#endif
      retVal = bind(pSocket->basicSocket, pOutAddr->ai_addr, (int)pOutAddr->ai_addrlen);
      UD_ERROR_IF(retVal == SOCKET_ERROR, udR_SocketError);

      retVal = listen(pSocket->basicSocket, SOMAXCONN);
      UD_ERROR_IF(retVal == SOCKET_ERROR, udR_SocketError);
    }
    else
    {
      retVal = connect(pSocket->basicSocket, pOutAddr->ai_addr, (int)pOutAddr->ai_addrlen);
      UD_ERROR_IF(retVal == SOCKET_ERROR, udR_SocketError);
    }
  }

  *ppSocket = pSocket;
  pSocket = nullptr;
  result = udR_Success;

epilogue:
  if (pOutAddr != nullptr)
    freeaddrinfo(pOutAddr);
  if (pSocket)
    udSocket_Close(&pSocket);

  udDebugPrintf("\t...Connection %s!\n", udResultAsString(result));
  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocket_Close(udSocket **ppSocket)
{
  if (ppSocket && *ppSocket)
  {
    udSocket *pSocket = *ppSocket;
    *ppSocket = nullptr;

    if (pSocket->isSecure)
    {
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
      closesocket(pSocket->basicSocket);
#else
      close(pSocket->basicSocket);
#endif
    }
    udFree(pSocket);
  }
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
udResult udSocket_SendData(udSocket *pSocket, const uint8_t *pBytes, int64_t totalBytes, int64_t *pActualSent /* = nullptr*/)
{
  udResult result;

  int64_t actualSent = 0;
  int64_t currentSend = 0;

  UD_ERROR_NULL(pSocket, udR_InvalidParameter);
  UD_ERROR_NULL(pBytes, udR_InvalidParameter);
  UD_ERROR_IF(totalBytes == 0, udR_Success); // Quietly succeed at doing nothing

  while (actualSent < totalBytes)
  {
    if (pSocket->isSecure)
      currentSend = mbedtls_ssl_write(&pSocket->tlsClient.ssl, &pBytes[actualSent], totalBytes - actualSent);
    else
      currentSend = send(pSocket->basicSocket, (const char *)&pBytes[actualSent], (int)(totalBytes - actualSent), 0);

    //TODO: Specifically handle the MBED errors

    UD_ERROR_IF(currentSend < 0, udR_SocketError); //TODO: this is really important to close socket somehow

    actualSent += currentSend;
  }

  if (pActualSent)
    *pActualSent = actualSent;

  //If the caller doesn't want the actual bytes sent, it must match exactly
  UD_ERROR_IF(!pActualSent && int64_t(actualSent) != totalBytes, udR_SocketError);

  result = udR_Success;

epilogue:
  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
udResult udSocket_ReceiveData(udSocket *pSocket, uint8_t *pBytes, int64_t bufferSize, int64_t *pActualReceived /* = nullptr*/)
{
  udResult result;
  int64_t actualReceived;

  UD_ERROR_NULL(pSocket, udR_InvalidParameter);
  UD_ERROR_NULL(pBytes, udR_InvalidParameter);
  UD_ERROR_IF(pSocket->isServer, udR_InvalidConfiguration);
  UD_ERROR_IF(bufferSize == 0, udR_Success); // Quietly succeed at doing nothing

  if (pSocket->isSecure)
    actualReceived = mbedtls_ssl_read(&pSocket->tlsClient.ssl, pBytes, bufferSize);
  else
    actualReceived = recv(pSocket->basicSocket, (char*)pBytes, (int)bufferSize, 0);

  UD_ERROR_IF(actualReceived < 0, udR_SocketError);

  //If the caller doesn't want the actual bytes recv, it must match the buffer size exactly
  UD_ERROR_IF(!pActualReceived && actualReceived != bufferSize, udR_SocketError);
  if (pActualReceived)
    *pActualReceived = actualReceived;
  result = udR_Success;

epilogue:
  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
bool udSocket_ServerAcceptClientPartA(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address /*= nullptr*/)
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
    if (mbedtls_net_accept(&pServerSocket->tlsClient.socketContext, &clientContext, clientIP, sizeof(clientIP), &clientBytesReturned) != 0)
      return false;

    if (pIPv4Address != nullptr && clientBytesReturned == 4)
      *pIPv4Address = (clientIP[0] << 24) | (clientIP[1] << 16) | (clientIP[2] << 8) | (clientIP[3]);

    (*ppClientSocket) = udAllocType(udSocket, 1, udAF_Zero);
    udSocket *pClientSocket = *ppClientSocket;

    pClientSocket->isSecure = true;
    pClientSocket->tlsClient.socketContext = clientContext;

    //Handle SSL
    /* Make sure memory references are valid */
    mbedtls_ssl_init(&pClientSocket->tlsClient.ssl);

    return (mbedtls_ssl_setup(&pClientSocket->tlsClient.ssl, &pServerSocket->tlsClient.conf) == 0);
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

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
bool udSocket_ServerAcceptClientPartB(udSocket *pClientSocket)
{
  if (!pClientSocket->isServer && pClientSocket->isSecure)
  {
    int retVal = 0;
    mbedtls_ssl_set_bio(&pClientSocket->tlsClient.ssl, &pClientSocket->tlsClient.socketContext, mbedtls_net_send, mbedtls_net_recv, NULL);

    //Handshake
    do
    {
      retVal = mbedtls_ssl_handshake(&pClientSocket->tlsClient.ssl);
      if (retVal == 0)
        break;

      if (retVal != MBEDTLS_ERR_SSL_WANT_READ && retVal != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
        udDebugPrintf("udSocket_ServerAcceptClientPartB Failed- mbedtls_ssl_handshake returned -0x%x\n", -retVal);
        return false;
      }
    } while (retVal != 0);
  }

  return true;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
bool udSocket_ServerAcceptClient(udSocket *pServerSocket, udSocket **ppClientSocket, uint32_t *pIPv4Address /*= nullptr*/)
{
  bool result = udSocket_ServerAcceptClientPartA(pServerSocket, ppClientSocket, pIPv4Address);

  if (result)
    result = udSocket_ServerAcceptClientPartB(*ppClientSocket);

  return result;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocketSet_Create(udSocketSet **ppSocketSet)
{
  if (ppSocketSet == nullptr)
    return;

  *ppSocketSet = udAllocType(udSocketSet, 1, udAF_Zero);
  udSocketSet_EmptySet(*ppSocketSet);
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocketSet_Destroy(udSocketSet **ppSocketSet)
{
  if (ppSocketSet != nullptr && *ppSocketSet != nullptr)
    udFree(*ppSocketSet);
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocketSet_EmptySet(udSocketSet *pSocketSet)
{
  if (pSocketSet == nullptr)
    return;

  FD_ZERO(&pSocketSet->set);
  pSocketSet->highestSocketHandle = 0;
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
void udSocketSet_AddSocket(udSocketSet *pSocketSet, udSocket *pSocket)
{
  if (pSocket == nullptr || pSocketSet == nullptr)
    return;

  SOCKET socketHandle = pSocket->isSecure ? pSocket->tlsClient.socketContext.fd : pSocket->basicSocket;

  pSocketSet->highestSocketHandle = udMax(socketHandle, pSocketSet->highestSocketHandle);
  FD_SET(socketHandle, &pSocketSet->set);
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
bool udSocketSet_IsInSet(udSocketSet *pSocketSet, udSocket *pSocket)
{
  if (pSocketSet == nullptr || pSocket == nullptr)
    return false;

  SOCKET socketHandle = pSocket->isSecure ? pSocket->tlsClient.socketContext.fd : pSocket->basicSocket;

  if (pSocketSet->highestSocketHandle < socketHandle)
    return false;

  return (FD_ISSET(socketHandle, &pSocketSet->set) != 0);
}

// --------------------------------------------------------------------------
// Author: Paul Fox, October 2018
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
