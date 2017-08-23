#include "udThread.h"

#if UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#endif

static udThreadCreateCallback *s_pThreadCreateCallback;

struct udThread
{
  // First element of structure is GUARANTEED to be the operating system thread handle
# if UDPLATFORM_WINDOWS
  HANDLE handle;
# elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
  pthread_t t;
# else
#   error Unknown platform
# endif
  udThreadStart *pThreadStarter;
  void *pThreadData;
};

// ****************************************************************************
void udThread_SetCreateCallback(udThreadCreateCallback *pThreadCreateCallback)
{
  s_pThreadCreateCallback = pThreadCreateCallback;
}

// ----------------------------------------------------------------------------
static uint32_t udThread_Bootstrap(udThread *pThread)
{
  if (s_pThreadCreateCallback)
    (*s_pThreadCreateCallback)(pThread, true);

  uint32_t threadReturnValue = pThread->pThreadStarter(pThread->pThreadData);

  if (s_pThreadCreateCallback)
    (*s_pThreadCreateCallback)(pThread, false);

  return threadReturnValue;
}


// ****************************************************************************
udResult udThread_Create(udThread **ppThread, udThreadStart *pThreadStarter, void *pThreadData, udThreadCreateFlags /*flags*/)
{
  udResult result;
  udThread *pThread = nullptr;

  pThread = udAllocType(udThread, 1, udAF_Zero);
  UD_ERROR_NULL(pThread, udR_MemoryAllocationFailure);
  pThread->pThreadStarter = pThreadStarter;
  pThread->pThreadData = pThreadData;
#if UDPLATFORM_WINDOWS
  pThread->handle = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)udThread_Bootstrap, pThread, 0, NULL);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
  typedef void *(*PTHREAD_START_ROUTINE)(void *);
  pthread_create(&pThread->t, NULL, (PTHREAD_START_ROUTINE)udThread_Bootstrap, pThread);
#else
#error Unknown platform
#endif

  *ppThread = pThread;
  pThread = nullptr;
  result = udR_Success;

epilogue:
  udFree(pThread);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, November 2014
void udThread_SetPriority(udThread *pThread, udThreadPriority priority)
{
  if (pThread)
  {
#if UDPLATFORM_WINDOWS
    switch (priority)
    {
    case udTP_Lowest:   SetThreadPriority(pThread->handle, THREAD_PRIORITY_LOWEST); break;
    case udTP_Low:      SetThreadPriority(pThread->handle, THREAD_PRIORITY_BELOW_NORMAL); break;
    case udTP_Normal:   SetThreadPriority(pThread->handle, THREAD_PRIORITY_NORMAL); break;
    case udTP_High:     SetThreadPriority(pThread->handle, THREAD_PRIORITY_ABOVE_NORMAL); break;
    case udTP_Highest:  SetThreadPriority(pThread->handle, THREAD_PRIORITY_HIGHEST); break;
    }
#elif UDPLATFORM_LINUX
    int policy = sched_getscheduler(0);
    int lowest = sched_get_priority_min(policy);
    int highest = sched_get_priority_max(policy);
    int pthreadPrio = (priority * (highest - lowest) / udTP_Highest) + lowest;
    pthread_setschedprio(pThread->t, pthreadPrio);
#elif UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    udUnused(priority);
#else
#   error Unknown platform
#endif
  }
}

// ****************************************************************************
void udThread_Destroy(udThread **ppThread)
{
  if (ppThread)
  {
    udThread *pThread = *ppThread;
    *ppThread = nullptr;
    if (pThread)
    {
#if UDPLATFORM_WINDOWS
      CloseHandle(pThread->handle);
#endif
      udFree(pThread);
    }
  }
}

// ****************************************************************************
udResult udThread_Join(udThread *pThread, int waitMs)
{
  if (!pThread)
    return udR_InvalidParameter_;

#if UDPLATFORM_WINDOWS
  UDCOMPILEASSERT(INFINITE == UDTHREAD_WAIT_INFINITE, "Infinite constants don't match");

  DWORD result = WaitForSingleObject(pThread->handle, (DWORD)waitMs);
  if (result)
  {
    if (result == WAIT_TIMEOUT)
      return udR_Timeout;

    return udR_Failure_;
  }
#elif UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
  udUnused(waitMs);
  int result = pthread_join(pThread->t, nullptr);
  if (result)
  {
    if (result == EINVAL)
      return udR_InvalidParameter_;

    return udR_Failure_;
  }
#else
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    int result = pthread_join(pThread->t, nullptr);
    if (result)
    {
      if (result == EINVAL)
        return udR_InvalidParameter_;

      return udR_Failure_;
    }
  }
  else
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += waitMs / 1000;
    ts.tv_nsec += long(waitMs % 1000) * 1000000L;

    int result = pthread_timedjoin_np(pThread->t, nullptr, &ts);
    if (result)
    {
      if (result == ETIMEDOUT)
        return udR_Timeout;

      if (result == EINVAL)
        return udR_InvalidParameter_;

      return udR_Failure_;
    }
  }
#endif

  return udR_Success;
}

// ****************************************************************************
udSemaphore *udCreateSemaphore()
{
#if UDPLATFORM_WINDOWS
  HANDLE handle = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
  return (udSemaphore *)handle;
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
  sem_t *sem = (sem_t *)udAlloc(sizeof(sem_t));
  if (sem)
  {
    int result = sem_init(sem, 0, 0);
    if (result == -1)
      return nullptr;
  }
  return (udSemaphore*)sem;
#else
# error Unknown platform
#endif
}

// ****************************************************************************
void udDestroySemaphore(udSemaphore **ppSemaphore)
{
  if (ppSemaphore && *ppSemaphore)
  {
#if UDPLATFORM_WINDOWS
    HANDLE semHandle = (HANDLE)(*ppSemaphore);
    *ppSemaphore = NULL;
    CloseHandle(semHandle);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    sem_t *sem = (sem_t*)(*ppSemaphore);
    sem_destroy(sem);
    udFree(sem);
    *ppSemaphore = nullptr;
#else
#  error Unknown platform
#endif
  }
}

// ****************************************************************************
void udIncrementSemaphore(udSemaphore *pSemaphore, int count)
{
  if (pSemaphore)
  {
#if UDPLATFORM_WINDOWS
    ReleaseSemaphore((HANDLE)pSemaphore, count, nullptr);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    while (count-- > 0)
      sem_post((sem_t*)pSemaphore);
#else
#   error Unknown platform
#endif
  }
}

// ****************************************************************************
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs)
{
  if (pSemaphore)
  {
#if UDPLATFORM_WINDOWS
    return WaitForSingleObject((HANDLE)pSemaphore, waitMs);
#elif UDPLATFORM_LINUX
    if (waitMs == UDTHREAD_WAIT_INFINITE)
    {
      return sem_wait((sem_t*)pSemaphore);
    }
    else
    {
      struct  timespec ts;
      if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return -1;

      ts.tv_sec += waitMs / 1000;
      ts.tv_nsec += long(waitMs % 1000) * 1000000L;

      ts.tv_sec += (ts.tv_nsec / 1000000000L);
      ts.tv_nsec %= 1000000000L;

      return sem_timedwait((sem_t*)pSemaphore, &ts);
    }
#elif UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    return sem_wait((sem_t*)pSemaphore);  // TODO: Need to find out timedwait equiv for NACL
#else
#   error Unknown platform
#endif
  }
  return -1;
}

// ****************************************************************************
udMutex *udCreateMutex()
{
#if UDPLATFORM_WINDOWS
  HANDLE handle = CreateMutex(NULL, FALSE, NULL);
  return (udMutex *)handle;
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
  pthread_mutex_t *mutex = (pthread_mutex_t *)udAlloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return (udMutex*)mutex;
#else
#error Unknown platform
#endif
}

// ****************************************************************************
void udDestroyMutex(udMutex **ppMutex)
{
  if (ppMutex && *ppMutex)
  {
#if UDPLATFORM_WINDOWS
    HANDLE mutexHandle = (HANDLE)(*ppMutex);
    *ppMutex = NULL;
    CloseHandle(mutexHandle);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    pthread_mutex_t *mutex = (pthread_mutex_t *)(*ppMutex);
    pthread_mutex_destroy(mutex);
    udFree(mutex);
    *ppMutex = nullptr;
#else
#  error Unknown platform
#endif
  }
}

// ****************************************************************************
void udLockMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    WaitForSingleObject((HANDLE)pMutex, INFINITE);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    pthread_mutex_lock((pthread_mutex_t *)pMutex);
#else
#   error Unknown platform
#endif
  }
}

// ****************************************************************************
void udReleaseMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    ReleaseMutex((HANDLE)pMutex);
#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
    pthread_mutex_unlock((pthread_mutex_t *)pMutex);
#else
#error Unknown platform
#endif
  }
}


