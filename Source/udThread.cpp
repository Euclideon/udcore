#include "udThread.h"

#include <atomic>

#if UDPLATFORM_WINDOWS
//
// SetThreadName function taken from https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; // Must be 0x1000.
  LPCSTR szName; // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
static void SetThreadName(DWORD dwThreadID, const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}
#else
#include <sched.h>
#include <pthread.h>
#include <errno.h>

#if UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# include <dispatch/dispatch.h>
#else
# include <semaphore.h>
#endif

void udThread_MsToTimespec(struct timespec *pTimespec, int waitMs)
{
  if (pTimespec == nullptr)
    return;

  clock_gettime(CLOCK_REALTIME, pTimespec);
  pTimespec->tv_sec += waitMs / 1000;
  pTimespec->tv_nsec += long(waitMs % 1000) * 1000000L;

  pTimespec->tv_sec += (pTimespec->tv_nsec / 1000000000L);
  pTimespec->tv_nsec %= 1000000000L;
}
#endif

#define DEBUG_CACHE 0
#define MAX_CACHED_THREADS 16
#define CACHE_WAIT_SECONDS 360
static volatile udThread *s_pCachedThreads[MAX_CACHED_THREADS ? MAX_CACHED_THREADS : 1];

struct udThread
{
  // First element of structure is GUARANTEED to be the operating system thread handle
# if UDPLATFORM_WINDOWS
  HANDLE handle;
# else
  pthread_t t;
# endif
  udThreadStart threadStarter;
  void *pThreadData;
  udSemaphore *pCacheSemaphore; // Semaphore is non-null only while on the cached thread list
  std::atomic<int32_t> refCount;
};

#if UDPLATFORM_WINDOWS
using udThreadReturnType = unsigned long;
udThreadReturnType udThreadReturnTypeConvert(uint32_t ret) { return static_cast<udThreadReturnType>(ret); }
#else
using udThreadReturnType = void *;
udThreadReturnType udThreadReturnTypeConvert(uint32_t ret) { return reinterpret_cast<udThreadReturnType>(ret); }
#endif

// ----------------------------------------------------------------------------
static udThreadReturnType udThread_Bootstrap(udThread *pThread)
{
  uint32_t threadReturnValue;
  bool reclaimed = false; // Set to true when the thread is reclaimed by the cache system
  UDASSERT(pThread->threadStarter, "No starter function");
  do
  {
    if constexpr (DEBUG_CACHE)
    {
      if (reclaimed)
        udDebugPrintf("Successfully reclaimed thread %p\n", (void *)pThread);
    }

    threadReturnValue = pThread->threadStarter ? pThread->threadStarter(pThread->pThreadData) : 0;

    pThread->threadStarter = nullptr;
    udInterlockedExchangePointer(&pThread->pThreadData, nullptr);
    reclaimed = false;

    if (pThread->refCount == 1)
    {
      // Instead of letting this thread go to waste, see if we can cache it to be recycled
      for (int slotIndex = 0; slotIndex < MAX_CACHED_THREADS; ++slotIndex)
      {
        if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], pThread, nullptr) == nullptr)
        {
          if constexpr (DEBUG_CACHE)
            udDebugPrintf("Making thread %p available for cache (slot %d)\n", (void *)pThread, slotIndex);

          // Successfully added to the cache, now wait to see if anyone wants to dance
          int timeoutWakeup = udWaitSemaphore(pThread->pCacheSemaphore, CACHE_WAIT_SECONDS * 1000);
          if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], nullptr, pThread) == pThread)
          {
            if constexpr (DEBUG_CACHE)
              udDebugPrintf("Allowing thread %p to die\n", (void *)pThread);
          }
          else
          {
            if constexpr (DEBUG_CACHE)
              udDebugPrintf("Reclaiming thread %p\n", (void *)pThread);

            // If it was woken via timeout, AND it was reclaimed, there's an outstanding semaphore ref
            if (timeoutWakeup)
              udWaitSemaphore(pThread->pCacheSemaphore);
            reclaimed = true;
          }
          break;
        }
      }
    }
  } while (reclaimed && pThread->threadStarter);

#if !UDPLATFORM_WINDOWS
  // Thread won't be joined, detach thread so resources are cleaned up
  if (pThread->refCount == 1)
    pthread_detach(pThread->t);
#endif

  // Call to destroy here will decrement reference count, and only destroy if
  // the original creator of the thread didn't take a reference themselves
  if (pThread)
    udThread_Destroy(&pThread);

  return udThreadReturnTypeConvert(threadReturnValue);
}


// ****************************************************************************
udResult udThread_Create(udThread **ppThread, udThreadStart threadStarter, void *pThreadData, udThreadCreateFlags /*flags*/, const char *pThreadName)
{
  udResult result;
  udThread *pThread = nullptr;
  int slotIndex;
  udUnused(pThreadName);

  UD_ERROR_NULL(threadStarter, udR_InvalidParameter);
  for (slotIndex = 0; pThread == nullptr && slotIndex < MAX_CACHED_THREADS; ++slotIndex)
  {
    pThread = const_cast<udThread*>(s_pCachedThreads[slotIndex]);
    if (udInterlockedCompareExchangePointer(&s_pCachedThreads[slotIndex], nullptr, pThread) != pThread)
      pThread = nullptr;
  }
  if (pThread)
  {
    pThread->threadStarter = threadStarter;
    udInterlockedExchangePointer(&pThread->pThreadData, pThreadData);
    udIncrementSemaphore(pThread->pCacheSemaphore);
  }
  else
  {
    pThread = udAllocType(udThread, 1, udAF_Zero);
    UD_ERROR_NULL(pThread, udR_MemoryAllocationFailure);
    pThread->pCacheSemaphore = udCreateSemaphore();
    if constexpr (DEBUG_CACHE)
      udDebugPrintf("Creating udThread %p\n", (void *)pThread);

    pThread->threadStarter = threadStarter;
    pThread->pThreadData = pThreadData;
    pThread->refCount = 1;
# if UDPLATFORM_WINDOWS
    pThread->handle = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)udThread_Bootstrap, pThread, 0, NULL);
#else
    typedef void *(*PTHREAD_START_ROUTINE)(void *);
    pthread_create(&pThread->t, NULL, (PTHREAD_START_ROUTINE)udThread_Bootstrap, pThread);
#endif
  }
#if UDPLATFORM_WINDOWS
  if (pThreadName)
    SetThreadName(GetThreadId(pThread->handle), pThreadName);
#elif (UDPLATFORM_LINUX || UDPLATFORM_ANDROID)
  if (pThreadName)
    pthread_setname_np(pThread->t, pThreadName);
#endif

  if (ppThread)
  {
    // Since we're returning a handle, increment the ref count because the caller is now expected to destroy it
    *ppThread = pThread;
    ++pThread->refCount;
  }
  result = udR_Success;

epilogue:
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
#else
    udUnused(priority);
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
    if (pThread && (--pThread->refCount) == 0)
    {
      udDestroySemaphore(&pThread->pCacheSemaphore);
#if UDPLATFORM_WINDOWS
      CloseHandle(pThread->handle);
#endif
      udFree(pThread);
    }
  }
}

// ****************************************************************************
// Author: Dave Pevreal, July 2018
void udThread_DestroyCached()
{
  while (1)
  {
    bool anyThreadsLeft = false;
    for (int slotIndex = 0; slotIndex < MAX_CACHED_THREADS; ++slotIndex)
    {
      volatile udThread *pThread = udInterlockedExchangePointer(&s_pCachedThreads[slotIndex], nullptr);
      if (pThread)
      {
        udIncrementSemaphore(pThread->pCacheSemaphore);
        anyThreadsLeft = true;
      }
    }
    if (anyThreadsLeft)
      udYield();
    else
      break;
  }
}

// ****************************************************************************
udResult udThread_Join(udThread *pThread, int waitMs)
{
  if (!pThread)
    return udR_InvalidParameter;

#if UDPLATFORM_WINDOWS
  UDCOMPILEASSERT(INFINITE == UDTHREAD_WAIT_INFINITE, "Infinite constants don't match");

  DWORD result = WaitForSingleObject(pThread->handle, (DWORD)waitMs);
  if (result)
  {
    if (result == WAIT_TIMEOUT)
      return udR_Timeout;

    return udR_Failure;
  }
#elif UDPLATFORM_LINUX
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    int result = pthread_join(pThread->t, nullptr);
    if (result)
    {
      if (result == EINVAL)
        return udR_InvalidParameter;

      return udR_Failure;
    }
  }
  else
  {
    timespec ts;
    udThread_MsToTimespec(&ts, waitMs);

    int result = pthread_timedjoin_np(pThread->t, nullptr, &ts);
    if (result)
    {
      if (result == ETIMEDOUT)
        return udR_Timeout;

      if (result == EINVAL)
        return udR_InvalidParameter;

      return udR_Failure;
    }
  }
#else
  udUnused(waitMs);
  int result = pthread_join(pThread->t, nullptr);
  if (result)
  {
    if (result == EINVAL)
      return udR_InvalidParameter;

    return udR_Failure;
  }
#endif

  return udR_Success;
}

struct udSemaphore
{
#if UDPLATFORM_WINDOWS
  HANDLE handle;
#elif UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  dispatch_semaphore_t handle;
#else
  sem_t handle;
#endif
};

// ****************************************************************************
// Author: Samuel Surtees, August 2017
udSemaphore *udCreateSemaphore()
{
  udResult result;
  udSemaphore *pSemaphore = udAllocType(udSemaphore, 1, udAF_None);
  UD_ERROR_NULL(pSemaphore, udR_MemoryAllocationFailure);
#if UDPLATFORM_WINDOWS
  pSemaphore->handle = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
  UD_ERROR_NULL(pSemaphore->handle, udR_Failure);
#elif UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  pSemaphore->handle = dispatch_semaphore_create(0);
  UD_ERROR_NULL(pSemaphore->handle, udR_Failure);
#else
  UD_ERROR_IF(sem_init(&pSemaphore->handle, 0, 0) == -1, udR_Failure);
#endif

  result = udR_Success;

epilogue:
  return pSemaphore;
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
void udDestroySemaphore(udSemaphore **ppSemaphore)
{
  if (ppSemaphore == nullptr || *ppSemaphore == nullptr)
    return;

  udSemaphore *pSemaphore = (*(udSemaphore*volatile*)ppSemaphore);
  if (udInterlockedCompareExchangePointer(ppSemaphore, nullptr, pSemaphore) != pSemaphore)
    return;

#if UDPLATFORM_WINDOWS
  CloseHandle(pSemaphore->handle);
#elif UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  dispatch_release(pSemaphore->handle);
#else
  sem_destroy(&pSemaphore->handle);
#endif
  udFree(pSemaphore);
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
void udIncrementSemaphore(udSemaphore *pSemaphore, int count)
{
  if (pSemaphore == nullptr)
    return;

#if UDPLATFORM_WINDOWS
  ReleaseSemaphore(pSemaphore->handle, count, nullptr);
#elif UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  while (count-- > 0)
    dispatch_semaphore_signal(pSemaphore->handle);
#else
  while (count-- > 0)
    sem_post(&pSemaphore->handle);
#endif
}

// ****************************************************************************
// Author: Samuel Surtees, August 2017
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs)
{
  if (pSemaphore == nullptr)
    return -1;

#if UDPLATFORM_WINDOWS
  return WaitForSingleObject(pSemaphore->handle, waitMs);
#elif UDPLATFORM_OSX || UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  dispatch_time_t timeout = DISPATCH_TIME_FOREVER;
  if (waitMs != UDTHREAD_WAIT_INFINITE)
    timeout = dispatch_time(DISPATCH_TIME_NOW, waitMs * NSEC_PER_SEC / 1000);

  return (int)dispatch_semaphore_wait(pSemaphore->handle, timeout);
#else
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    return sem_wait(&pSemaphore->handle);
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

    return sem_timedwait(&pSemaphore->handle, &ts);
  }
#endif
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
udConditionVariable *udCreateConditionVariable()
{
#if UDPLATFORM_WINDOWS
  CONDITION_VARIABLE *pCondition = udAllocType(CONDITION_VARIABLE, 1, udAF_None);
  InitializeConditionVariable(pCondition);
#else
  pthread_cond_t *pCondition = udAllocType(pthread_cond_t, 1, udAF_None);
  pthread_cond_init(pCondition, NULL);
#endif

  return (udConditionVariable*)pCondition;
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
void udDestroyConditionVariable(udConditionVariable **ppConditionVariable)
{
  udConditionVariable *pCondition = *ppConditionVariable;
  *ppConditionVariable = nullptr;

  // Windows doesn't have a clean-up function
#if !UDPLATFORM_WINDOWS
  pthread_cond_destroy((pthread_cond_t *)pCondition);
#endif

  udFree(pCondition);
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
void udSignalConditionVariable(udConditionVariable *pConditionVariable, int count)
{
  while (count-- > 0)
  {
#if UDPLATFORM_WINDOWS
    CONDITION_VARIABLE *pCondition = (CONDITION_VARIABLE*)pConditionVariable;
    WakeConditionVariable(pCondition);
#else
    pthread_cond_t *pCondition = (pthread_cond_t*)pConditionVariable;
    pthread_cond_signal(pCondition);
#endif
  }
}

// ****************************************************************************
// Author: Samuel Surtees, September 2017
int udWaitConditionVariable(udConditionVariable *pConditionVariable, udMutex *pMutex, int waitMs)
{
#if UDPLATFORM_WINDOWS
  CONDITION_VARIABLE *pCondition = (CONDITION_VARIABLE*)pConditionVariable;
  CRITICAL_SECTION *pCriticalSection = (CRITICAL_SECTION*)pMutex;
  BOOL retVal = SleepConditionVariableCS(pCondition, pCriticalSection, (waitMs == UDTHREAD_WAIT_INFINITE ? INFINITE : waitMs));
  return (retVal == TRUE ? 0 : 1); // This isn't (!retVal) for clarity.
#else
  pthread_cond_t *pCondition = (pthread_cond_t*)pConditionVariable;
  pthread_mutex_t *pMutexInternal = (pthread_mutex_t*)pMutex;
  int retVal = 0;
  if (waitMs == UDTHREAD_WAIT_INFINITE)
  {
    retVal = pthread_cond_wait(pCondition, pMutexInternal);
  }
  else
  {
    struct timespec ts;
    udThread_MsToTimespec(&ts, waitMs);
    retVal = pthread_cond_timedwait(pCondition, pMutexInternal, &ts);
  }

  return retVal;
#endif
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
udRWLock *udCreateRWLock()
{
#if UDPLATFORM_WINDOWS
  SRWLOCK *pRWLock = udAllocType(SRWLOCK, 1, udAF_None);
  if (pRWLock)
    InitializeSRWLock(pRWLock);
  return (udRWLock*)pRWLock;
#else
  pthread_rwlock_t *pRWLock = (pthread_rwlock_t *)udAlloc(sizeof(pthread_rwlock_t));
  if (pRWLock)
    pthread_rwlock_init(pRWLock, NULL);
  return (udRWLock*)pRWLock;
#endif
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
void udDestroyRWLock(udRWLock **ppRWLock)
{
  if (ppRWLock && *ppRWLock)
  {
#if UDPLATFORM_WINDOWS
    // Windows doesn't have a clean-up function
    udFree(*ppRWLock);
#else
    pthread_rwlock_t *pRWLock = (pthread_rwlock_t *)(*ppRWLock);
    if (udInterlockedCompareExchangePointer(ppRWLock, nullptr, pRWLock) == (udRWLock*)pRWLock)
    {
      pthread_rwlock_destroy(pRWLock);
      udFree(pRWLock);
    }
#endif
  }
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
int udReadLockRWLock(udRWLock *pRWLock)
{
  if (pRWLock)
  {
#if UDPLATFORM_WINDOWS
    SRWLOCK *pLock = (SRWLOCK*)pRWLock;
    AcquireSRWLockShared(pLock);
    return 0; // Can't fail
#else
    pthread_rwlock_t *pLock = (pthread_rwlock_t*)pRWLock;
    return pthread_rwlock_rdlock(pLock);
#endif
  }
  return 0;
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
int udWriteLockRWLock(udRWLock *pRWLock)
{
  if (pRWLock)
  {
#if UDPLATFORM_WINDOWS
    SRWLOCK *pLock = (SRWLOCK*)pRWLock;
    AcquireSRWLockExclusive(pLock);
    return 0; // Can't fail
#else
    pthread_rwlock_t *pLock = (pthread_rwlock_t*)pRWLock;
    return pthread_rwlock_wrlock(pLock);
#endif
  }
  return 0;
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
void udReadUnlockRWLock(udRWLock *pRWLock)
{
  if (pRWLock)
  {
#if UDPLATFORM_WINDOWS
    SRWLOCK *pLock = (SRWLOCK*)pRWLock;
    ReleaseSRWLockShared(pLock);
#else
    pthread_rwlock_t *pLock = (pthread_rwlock_t*)pRWLock;
    pthread_rwlock_unlock(pLock);
#endif
  }
}

// ****************************************************************************
// Author: Samuel Surtees, March 2019
void udWriteUnlockRWLock(udRWLock *pRWLock)
{
  if (pRWLock)
  {
#if UDPLATFORM_WINDOWS
    SRWLOCK *pLock = (SRWLOCK*)pRWLock;
    ReleaseSRWLockExclusive(pLock);
#else
    pthread_rwlock_t *pLock = (pthread_rwlock_t*)pRWLock;
    pthread_rwlock_unlock(pLock);
#endif
  }
}

// ****************************************************************************
udMutex *udCreateMutex()
{
#if UDPLATFORM_WINDOWS
  CRITICAL_SECTION *pCriticalSection = udAllocType(CRITICAL_SECTION, 1, udAF_None);
  if (pCriticalSection)
    InitializeCriticalSection(pCriticalSection);
  return (udMutex *)pCriticalSection;
#else
  pthread_mutex_t *mutex = (pthread_mutex_t *)udAlloc(sizeof(pthread_mutex_t));
  if (mutex)
  {
    // Initialise the mutex to be recursive, this allows the same thread to lock multiple times
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
  }
  return (udMutex*)mutex;
#endif
}

// ****************************************************************************
void udDestroyMutex(udMutex **ppMutex)
{
  if (ppMutex && *ppMutex)
  {
#if UDPLATFORM_WINDOWS
    CRITICAL_SECTION *pCriticalSection = (CRITICAL_SECTION*)(*ppMutex);
    *ppMutex = NULL;
    DeleteCriticalSection(pCriticalSection);
    udFree(pCriticalSection);
#else
    pthread_mutex_t *mutex = (pthread_mutex_t *)(*ppMutex);
    pthread_mutex_destroy(mutex);
    udFree(mutex);
    *ppMutex = nullptr;
#endif
  }
}

// ****************************************************************************
udMutex *udLockMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    EnterCriticalSection((CRITICAL_SECTION*)pMutex);
#else
    pthread_mutex_lock((pthread_mutex_t *)pMutex);
#endif
  }
  return pMutex;
}

// ****************************************************************************
void udReleaseMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    LeaveCriticalSection((CRITICAL_SECTION*)pMutex);
#else
    pthread_mutex_unlock((pthread_mutex_t *)pMutex);
#endif
  }
}


