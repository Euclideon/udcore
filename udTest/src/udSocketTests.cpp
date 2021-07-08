#include "gtest/gtest.h"
#include "udThread.h"
#include "udSocket.h"
#include "udStringUtil.h"

// Emscripten requires more work for sockets
#if !UDPLATFORM_EMSCRIPTEN
uint8_t clientSend[] = "ClientSendData";
uint8_t serverSend[] = "ServerSendData";

#define UDSOCKETTEST_CERTIFICATE_PRIVATE_KEY "-----BEGIN PRIVATE KEY-----\n\
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDY1f2s/EeofcuL\n\
FshdPoInwR30ZWAuKFl0dCcMB74P4zuKucVgcil26sjKjFLAJwFH+d2iOXUmgy7/\n\
dzz5dJxUESA8yLlOL4LX3HBlKKQ17WQkjjAxiOpxLq5QzaHtoH9nR963yc6igT6I\n\
vGyNwKD8cApGNbMpBk0+0Ke1Qxcw65+dXg9J123rHKik2JQissjO5XjeYsp7kL8N\n\
wDtkfRvxAz4dDxSILGIL0GqlOgDgcduDkljreqvSaTb6mmsnKiXABhk/FiTinCGz\n\
NyIAJ5+0rgPAGHi3bONUQGdaCt7OzkD+o/4rxCQsuMPm0KptnGJnAKFHw0jL0lIU\n\
HvyJsa/DAgMBAAECggEAHD4DBHz3eGKqGDunbT2vBi6JEEQD+v8WZ6yZSV/YyYj3\n\
QGJh6uXzsKFz9a3BOXXbHNzWmUKSl6mOfYeyUWt20RxJ7qDWQfC+Qg9cNFDO0pdQ\n\
69H5RPpoTsWdvriJ1sGI1pLt11JZr6DV3ElueigUz8xoCi0EYNuJRB05Osq6Qoyw\n\
oqbVfblr9oPEiAx1jKYnHrYdpgBdlxe/hL4JZlD5APhflmdaF8oM/MCpQ2uQ1X9n\n\
ZIiSpvsTAVtZ58t+T2ZAIu4UR1IN4+tf4QYWNrhZJAn4Bz8nAZyd8L1SO58S9HKc\n\
TdduuSZ7Z6cGWMqNZIc+ypyzGYD2pd8tl9i0ql3QAQKBgQDwJb3NG5OOT3IEZCfQ\n\
UdWXAwhZt9uIBNaUBE7vBrIrx8J7t/gMOxs5p4f5P1Sjs4u+a02S17M6N3e7ESCp\n\
1psk6bb+LFSuE0oOjBRjfq0V1cDPDyCxVANE1onLH0QifZUP0ebSusW3jfvNVV48\n\
FIHXDL8C5hhkVVnebLuS8nCnAQKBgQDnJk6uAqGiMG2b+xUgJ12uaVZkmvOZF8v2\n\
1qbbeJfav+JzvHRZLnjm7Zcyp7sCc6ts3yri4bPzRZRLl707HB5Any4FFAFeKIjn\n\
WsfV38/0vuxdM8BomFPMos3LOgRwZTXPgv/4zLqg+svmBNLvdI5UHTXuVJb5XwXn\n\
U7Scmkx6wwKBgQDF5PQRr6Xa4hD9GWPNwYIXnYImHOKlpgcFlr9NLeFpPoY/2Yxo\n\
19RJoIrmlI+1DuIbmuUkGugxE0BxQV3/V0AmHQqVTgbCJFckwb6TdvI/ShRHDRpN\n\
xwOimICYiD1nhsFtdfWWth70ceaMgMrVC7krc+97/g0fsU4LguLX5z16AQKBgFfI\n\
o47QLwRCcg4Pz9kTOi+3j3jLpAGbGPnYUSv+Y2VPBDhB9Mm9rWq+BnLVYl8vAIZr\n\
RoU9hDK6VPRUXygoqJCQI1EIZPCWYk/CmSvvQLHJJEjNE6BlYUXQ+mjY0sDAnyv8\n\
qyuYzLPAK1lisZ0A3eCx6z5k36U64ioVDv1+V9czAoGAOMrs/FdzHkna7MCgAzOr\n\
Vc+teD6M9yB4i0g3D5n72TuM4izk03Qz1fXfBDCeUPb7CvXF8Oco/4baSjoz+62U\n\
+8jdUnx/AOSac2LauzWhziOPiDz4WGN3BfsHcDQN+xOxZLrII/yP9ky8YKPuJeGj\n\
hJUnrbVwYTWLHnkV2tC3txE=\n\
-----END PRIVATE KEY-----"
#define UDSOCKETTEST_CERTIFICATE_PUBLIC_KEY "-----BEGIN CERTIFICATE-----\n\
MIIDpDCCAowCCQDkMz5PqaQQAzANBgkqhkiG9w0BAQsFADCBkzELMAkGA1UEBhMC\n\
QVUxEzARBgNVBAgMClF1ZWVuc2xhbmQxETAPBgNVBAcMCEJyaXNiYW5lMRowGAYD\n\
VQQKDBFFdWNsaWRlb24gUHR5IEx0ZDEOMAwGA1UECwwFVmF1bHQxDTALBgNVBAMM\n\
BHBmb3gxITAfBgkqhkiG9w0BCQEWEnBmb3hAZXVjbGlkZW9uLmNvbTAeFw0xODAz\n\
MDIwNTIxMDRaFw0xOTAzMDIwNTIxMDRaMIGTMQswCQYDVQQGEwJBVTETMBEGA1UE\n\
CAwKUXVlZW5zbGFuZDERMA8GA1UEBwwIQnJpc2JhbmUxGjAYBgNVBAoMEUV1Y2xp\n\
ZGVvbiBQdHkgTHRkMQ4wDAYDVQQLDAVWYXVsdDENMAsGA1UEAwwEcGZveDEhMB8G\n\
CSqGSIb3DQEJARYScGZveEBldWNsaWRlb24uY29tMIIBIjANBgkqhkiG9w0BAQEF\n\
AAOCAQ8AMIIBCgKCAQEA2NX9rPxHqH3LixbIXT6CJ8Ed9GVgLihZdHQnDAe+D+M7\n\
irnFYHIpdurIyoxSwCcBR/ndojl1JoMu/3c8+XScVBEgPMi5Ti+C19xwZSikNe1k\n\
JI4wMYjqcS6uUM2h7aB/Z0fet8nOooE+iLxsjcCg/HAKRjWzKQZNPtCntUMXMOuf\n\
nV4PSddt6xyopNiUIrLIzuV43mLKe5C/DcA7ZH0b8QM+HQ8UiCxiC9BqpToA4HHb\n\
g5JY63qr0mk2+pprJyolwAYZPxYk4pwhszciACeftK4DwBh4t2zjVEBnWgrezs5A\n\
/qP+K8QkLLjD5tCqbZxiZwChR8NIy9JSFB78ibGvwwIDAQABMA0GCSqGSIb3DQEB\n\
CwUAA4IBAQAfpS4WzhLEvz0DZ1bbxUrJUxCCJn9l34s2iOJsA7H+2cYamXzO1CsS\n\
xdFpf8oFttL78E03e8frY33hoYmieaRCXvHd6CfiubsDMjqnP4w3kaydr0/G311p\n\
r9HxcGTdjbEfUUnstY93SWtFym2qoZrsXEqnRXC7xyTeQXpqYeB6FwbWIejbgJEd\n\
qQyvoRRqLKJNtbn67YHXHcayLfTWPKKoMUbcHtuNRERMnNA1VJ83d51qf83T0zQh\n\
zLRT2lr+z1PYAYtrGHEUVhc17FlBo0gIkQYigsPguiJculBgzM6EDR9H8eiMcVPT\n\
CkUe0VXaLRTOp2gffJHHdMxYN0ZfuKs3\n\
-----END CERTIFICATE-----"

const int32_t BigTestSize = 1024 * 1024 * 40; //40MB; Anything over a few MB will cause send to fail for SSL

uint32_t udSocketTestsServerThread(void *data)
{
  udSocket *pListenSocket = (udSocket*)data;
  udSocket *pSocket = nullptr;
  EXPECT_TRUE(udSocket_ServerAcceptClient(pListenSocket, &pSocket));

  uint8_t recv[UDARRAYSIZE(clientSend)];
  EXPECT_EQ(udR_Success, udSocket_ReceiveData(pSocket, recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(clientSend, recv, sizeof(clientSend)));

  EXPECT_EQ(udR_Success, udSocket_SendData(pSocket, serverSend, sizeof(serverSend)));

  udSocket_Close(&pSocket);
  EXPECT_EQ(nullptr, pSocket);

  return 0;
}

uint32_t udSocketTestsSocketSetServerThread(void *data)
{
  udSocket *pListenSocket = (udSocket*)data;
  udSocket *pSocket = nullptr;
  udSocketSet *pReadSet = nullptr;
  udSocketSet *pWriteSet = nullptr;
  udSocketSet *pErrorSet = nullptr;
  udSocketSet_Create(&pReadSet);
  udSocketSet_Create(&pWriteSet);
  udSocketSet_Create(&pErrorSet);

  EXPECT_TRUE(udSocket_ServerAcceptClient(pListenSocket, &pSocket));

  int selectState = 0;
  do
  {
    udSocketSet_EmptySet(pReadSet);
    udSocketSet_EmptySet(pWriteSet);
    udSocketSet_EmptySet(pErrorSet);

    udSocketSet_AddSocket(pReadSet, pSocket);
    udSocketSet_AddSocket(pWriteSet, pSocket);
    udSocketSet_AddSocket(pErrorSet, pSocket);

    selectState = udSocketSet_Select(100, pReadSet, pWriteSet, pErrorSet);
  } while (!udSocketSet_IsInSet(pReadSet, pSocket));

  // Read and Write
  EXPECT_LT(0, selectState);

  EXPECT_TRUE(udSocketSet_IsInSet(pReadSet, pSocket));
  EXPECT_TRUE(udSocketSet_IsInSet(pWriteSet, pSocket));
  EXPECT_FALSE(udSocketSet_IsInSet(pErrorSet, pSocket));

  uint8_t recv[UDARRAYSIZE(clientSend)];
  EXPECT_EQ(udR_Success, udSocket_ReceiveData(pSocket, recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(clientSend, recv, sizeof(clientSend)));

  selectState = 0;
  do
  {
    udSocketSet_EmptySet(pReadSet);
    udSocketSet_EmptySet(pWriteSet);
    udSocketSet_EmptySet(pErrorSet);

    udSocketSet_AddSocket(pReadSet, pSocket);
    udSocketSet_AddSocket(pWriteSet, pSocket);
    udSocketSet_AddSocket(pErrorSet, pSocket);

    selectState = udSocketSet_Select(100, pReadSet, pWriteSet, pErrorSet);
  } while (!udSocketSet_IsInSet(pWriteSet, pSocket));

  // Read and Write
  EXPECT_LT(0, selectState);

  EXPECT_FALSE(udSocketSet_IsInSet(pReadSet, pSocket));
  EXPECT_TRUE(udSocketSet_IsInSet(pWriteSet, pSocket));
  EXPECT_FALSE(udSocketSet_IsInSet(pErrorSet, pSocket));

  EXPECT_EQ(udR_Success, udSocket_SendData(pSocket, serverSend, sizeof(serverSend)));

  udSocketSet_Destroy(&pReadSet);
  udSocketSet_Destroy(&pWriteSet);
  udSocketSet_Destroy(&pErrorSet);

  udSocket_Close(&pSocket);
  EXPECT_EQ(nullptr, pSocket);

  // Additional destruction of non-existent objects
  udSocket_Close(&pSocket);
  udSocket_Close(nullptr);

  return 0;
}

TEST(udSocket, ValidationTests)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  udSocket *sockets[2];
  udThread *pServerThread;
  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[0], "127.0.0.1", 40404, udSCF_IsServer));
  EXPECT_EQ(udR_Success, udThread_Create(&pServerThread, udSocketTestsServerThread, sockets[0]));

  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[1], "127.0.0.1", 40404));
  EXPECT_EQ(udR_Success, udSocket_SendData(sockets[1], clientSend, sizeof(clientSend)));

  uint8_t recv[UDARRAYSIZE(serverSend)];

  EXPECT_EQ(udR_Success, udSocket_ReceiveData(sockets[1], recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(serverSend, recv, sizeof(serverSend)));

  EXPECT_EQ(udR_Success, udThread_Join(pServerThread));
  udThread_Destroy(&pServerThread);

  udSocket_Close(&sockets[1]);
  EXPECT_EQ(nullptr, sockets[1]);
  udSocket_Close(&sockets[0]);
  EXPECT_EQ(nullptr, sockets[0]);

  udSocket_DeinitSystem();
}

uint8_t udSocketTests_BigTestVal(int i)
{
  return (uint8_t)(((i * 1103515245) + 12345) & 0x7fffffff);
}

uint32_t udSocketTestsServerThread_BigTest(void *data)
{
  udSocket *pListenSocket = (udSocket *)data;
  udSocket *pSocket = nullptr;

  EXPECT_TRUE(udSocket_ServerAcceptClient(pListenSocket, &pSocket));

  const int TotalReadBuffer = 1024 * 1024 * 10; //10MB

  uint8_t *pRecv = udAllocType(uint8_t, TotalReadBuffer, udAF_None); //2MB
  int64_t totalRead = 0;
  int64_t currentRead = 0;

  while (totalRead < BigTestSize)
  {
    EXPECT_EQ(udR_Success, udSocket_ReceiveData(pSocket, pRecv, TotalReadBuffer, &currentRead));

    for (int i = 0; i < currentRead; ++i)
      EXPECT_EQ(pRecv[i], udSocketTests_BigTestVal((int32_t)(i + totalRead)));

    totalRead += currentRead;
  }

  udSocket_Close(&pSocket);
  EXPECT_EQ(nullptr, pSocket);

  udFree(pRecv);
  return 0;
}

TEST(udSocket, BigReadWriteTest)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  uint8_t *pData = udAllocType(uint8_t, BigTestSize, udAF_None);
 
  udSocket *sockets[2];
  udThread *pServerThread;

  ASSERT_EQ(udR_Success, udSocket_Open(&sockets[0], "127.0.0.1", 42624, udSCF_UseTLS | udSCF_IsServer, UDSOCKETTEST_CERTIFICATE_PRIVATE_KEY, UDSOCKETTEST_CERTIFICATE_PUBLIC_KEY));
  EXPECT_EQ(udR_Success, udThread_Create(&pServerThread, udSocketTestsServerThread_BigTest, sockets[0]));

  ASSERT_EQ(udR_Success, udSocket_Open(&sockets[1], "127.0.0.1", 42624, udSCF_UseTLS));

  for (int i = 0; i < BigTestSize; ++i)
    pData[i] = udSocketTests_BigTestVal(i);

  int64_t actualWritten = 0;
  EXPECT_EQ(udR_Success, udSocket_SendData(sockets[1], pData, BigTestSize, &actualWritten));
  EXPECT_EQ(BigTestSize, (int32_t)actualWritten);

  EXPECT_EQ(udR_Success, udThread_Join(pServerThread));
  udThread_Destroy(&pServerThread);

  udSocket_Close(&sockets[1]);
  udSocket_Close(&sockets[0]);

  udFree(pData);
  udSocket_DeinitSystem();
}

TEST(udSocket, SecureValidationTests)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  udSocket *sockets[2];
  udThread *pServerThread;
  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[0], "127.0.0.1", 40404, udSCF_UseTLS | udSCF_IsServer, UDSOCKETTEST_CERTIFICATE_PRIVATE_KEY, UDSOCKETTEST_CERTIFICATE_PUBLIC_KEY));
  EXPECT_EQ(udR_Success, udThread_Create(&pServerThread, udSocketTestsServerThread, sockets[0]));

  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[1], "127.0.0.1", 40404, udSCF_UseTLS | udSCF_DisableVerification));
  EXPECT_EQ(udR_Success, udSocket_SendData(sockets[1], clientSend, sizeof(clientSend)));

  uint8_t recv[UDARRAYSIZE(serverSend)];

  EXPECT_EQ(udR_Success, udSocket_ReceiveData(sockets[1], recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(serverSend, recv, sizeof(serverSend)));

  EXPECT_EQ(udR_Success, udThread_Join(pServerThread));
  udThread_Destroy(&pServerThread);

  udSocket_Close(&sockets[1]);
  EXPECT_EQ(nullptr, sockets[1]);
  udSocket_Close(&sockets[0]);
  EXPECT_EQ(nullptr, sockets[0]);

  udSocket_DeinitSystem();
}

TEST(udSocket, SocketSetValidationTests)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  udSocket *sockets[2];
  udThread *pServerThread;
  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[0], "127.0.0.1", 40404, udSCF_IsServer));
  EXPECT_EQ(udR_Success, udThread_Create(&pServerThread, udSocketTestsSocketSetServerThread, sockets[0]));

  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[1], "127.0.0.1", 40404));
  EXPECT_EQ(udR_Success, udSocket_SendData(sockets[1], clientSend, sizeof(clientSend)));

  uint8_t recv[UDARRAYSIZE(serverSend)];

  EXPECT_EQ(udR_Success, udSocket_ReceiveData(sockets[1], recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(serverSend, recv, sizeof(serverSend)));

  EXPECT_EQ(udR_Success, udThread_Join(pServerThread));
  udThread_Destroy(&pServerThread);

  udSocket_Close(&sockets[1]);
  EXPECT_EQ(nullptr, sockets[1]);
  udSocket_Close(&sockets[0]);
  EXPECT_EQ(nullptr, sockets[0]);

  udSocket_DeinitSystem();
}

TEST(udSocket, SecureSocketSetValidationTests)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  udSocket *sockets[2];
  udThread *pServerThread;
  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[0], "127.0.0.1", 40404, udSCF_UseTLS | udSCF_IsServer, UDSOCKETTEST_CERTIFICATE_PRIVATE_KEY, UDSOCKETTEST_CERTIFICATE_PUBLIC_KEY));
  EXPECT_EQ(udR_Success, udThread_Create(&pServerThread, udSocketTestsSocketSetServerThread, sockets[0]));

  EXPECT_EQ(udR_Success, udSocket_Open(&sockets[1], "127.0.0.1", 40404, udSCF_UseTLS| udSCF_DisableVerification));
  EXPECT_EQ(udR_Success, udSocket_SendData(sockets[1], clientSend, sizeof(clientSend)));

  uint8_t recv[UDARRAYSIZE(serverSend)];

  EXPECT_EQ(udR_Success, udSocket_ReceiveData(sockets[1], recv, sizeof(recv)));
  EXPECT_EQ(0, memcmp(serverSend, recv, sizeof(serverSend)));

  EXPECT_EQ(udR_Success, udThread_Join(pServerThread));
  udThread_Destroy(&pServerThread);

  udSocket_Close(&sockets[1]);
  EXPECT_EQ(nullptr, sockets[1]);
  udSocket_Close(&sockets[0]);
  EXPECT_EQ(nullptr, sockets[0]);

  udSocket_DeinitSystem();
}

TEST(udSocket, RealWebsiteSecureTest)
{
  EXPECT_EQ(udR_Success, udSocket_InitSystem());

  udSocket *pSockets = nullptr;
  EXPECT_EQ(udR_Success, udSocket_Open(&pSockets, "www.euclideon.com", 443, udSCF_UseTLS));

  const char *pRequest = "GET /favicon.ico HTTP/1.1\r\nHost: www.euclideon.com\r\nUser-Agent: HTTPTestAgent\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: en-us,en;q=0.5\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n";

  EXPECT_EQ(udR_Success, udSocket_SendData(pSockets, (const uint8_t*)pRequest, udStrlen(pRequest)+1));

  uint8_t recv[8192];

  int64_t recvCount = 0;
  int64_t totalRecv = 0;

  do
  {
    recvCount = 0;
    EXPECT_EQ(udR_Success, udSocket_ReceiveData(pSockets, &recv[totalRecv], sizeof(recv) - totalRecv, &recvCount));
    totalRecv += recvCount;

    // Avoid calling udSocket_ReceiveData after receiving all of the data, which results in a failure
    size_t index = 0;
    if (udStrstr((const char *)recv, totalRecv, "\r\n\r\n", &index) != nullptr)
    {
      // favicon is 1150 bytes, and \r\n\r\n is 4 bytes.
      if ((totalRecv - (int64_t)index) == (1150 + 4))
        break;
    }
  } while (recvCount > 0);

  udSocket_Close(&pSockets);
  EXPECT_EQ(nullptr, pSockets);

  udSocket_DeinitSystem();
}
#endif
