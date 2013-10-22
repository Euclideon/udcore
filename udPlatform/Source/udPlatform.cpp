#include "udPlatform.h"


uint64_t udCreateThread(udThreadStart *threadStarter, void *threadData)
{
#ifdef UDPLATFORM_WINDOWS
  DWORD threadId = 0;
  CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)threadStarter, threadData, 0, &threadId);
  return (uint64_t)threadId;
#else
  pthread_t t;
  typedef void *(*PTHREAD_START_ROUTINE)(void *);
  pthread_create(&t, NULL, (PTHREAD_START_ROUTINE)threadStarter, threadData);
  return t;
#endif
}

void udDestroyThread(uint64_t threadHandle)
{
#ifdef UDPLATFORM_WINDOWS
  CloseHandle((HANDLE)threadHandle);
#else
  // TODO: FIgure out which pthread function is *most* equivalent
#endif
}

