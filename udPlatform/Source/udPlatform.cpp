#include "udPlatform.h"
#include <stdlib.h>
#include <string.h>
#if UDPLATFORM_LINUX
#include <pthread.h>
#include <semaphore.h>
#endif

// ***************************************************************************************
udThreadHandle udCreateThread(udThreadStart *threadStarter, void *threadData)
{
#if UDPLATFORM_WINDOWS
  return (udThreadHandle)CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)threadStarter, threadData, 0, NULL);
#elif UDPLATFORM_LINUX
  pthread_t t;
  typedef void *(*PTHREAD_START_ROUTINE)(void *);
  pthread_create(&t, NULL, (PTHREAD_START_ROUTINE)threadStarter, threadData);
  return t;
#else
#error Unknown platform
#endif
}

// ***************************************************************************************
void udDestroyThread(udThreadHandle *pThreadHandle)
{
  if (pThreadHandle)
  {
#if UDPLATFORM_WINDOWS
    CloseHandle((HANDLE)(*pThreadHandle));
#elif UDPLATFORM_LINUX
    // TODO: FIgure out which pthread function is *most* equivalent
#else
#   error Unknown platform
#endif
    *pThreadHandle = 0;
  }
}

// ***************************************************************************************
udSemaphore *udCreateSemaphore(int maxValue, int initialValue)
{
#if UDPLATFORM_WINDOWS
  HANDLE handle = CreateSemaphore(NULL, initialValue, maxValue, NULL);
  return (udSemaphore *)handle;
#elif UDPLATFORM_LINUX
  sem_t *sem = (sem_t *)udAlloc(sizeof(sem_t));
  (void)maxValue;
  if (sem)
    sem_init(sem, 0, initialValue);
  return (udSemaphore*)sem;
#else
# error Unknown platform
#endif
}

// ***************************************************************************************
void udDestroySemaphore(udSemaphore **ppSemaphore)
{
  if (ppSemaphore && *ppSemaphore)
  {
#if UDPLATFORM_WINDOWS
    HANDLE semHandle = (HANDLE)(*ppSemaphore);
    *ppSemaphore = NULL;
    CloseHandle(semHandle);
#elif UDPLATFORM_LINUX
    sem_t *sem = (sem_t*)(*ppSemaphore);
    sem_destroy(sem);
    udFree(sem);
    *ppSemaphore = nullptr;
#else
#  error Unknown platform
#endif
  }
}

// ***************************************************************************************
void udIncrementSemaphore(udSemaphore *pSemaphore, int count)
{
  if (pSemaphore)
  {
#if UDPLATFORM_WINDOWS
    ReleaseSemaphore((HANDLE)pSemaphore, count, nullptr);
#elif UDPLATFORM_LINUX
    while (count-- > 0)
      sem_post((sem_t*)pSemaphore);
#else
#   error Unknown platform
#endif
  }
}

// ***************************************************************************************
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs)
{
  if (pSemaphore)
  {
#if UDPLATFORM_WINDOWS
    return WaitForSingleObject((HANDLE)pSemaphore, waitMs);
#elif UDPLATFORM_LINUX
    struct  timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = waitMs * 1000;
    return sem_timedwait((sem_t*)pSemaphore, &ts);
#else
#   error Unknown platform
#endif
  }
  return -1;
}

// ***************************************************************************************
udMutex *udCreateMutex()
{
#if UDPLATFORM_WINDOWS
  HANDLE handle = CreateMutex(NULL, FALSE, NULL);
  return (udMutex *)handle;
#elif UDPLATFORM_LINUX
  pthread_mutex_t *mutex = (pthread_mutex_t *)udAlloc(sizeof(pthread_mutex_t));
  if (mutex)
    pthread_mutex_init(mutex, NULL);
  return (udMutex*)mutex;
#else
#error Unknown platform
#endif
}

// ***************************************************************************************
void udDestroyMutex(udMutex **ppMutex)
{
  if (ppMutex && *ppMutex)
  {
#if UDPLATFORM_WINDOWS
    HANDLE mutexHandle = (HANDLE)(*ppMutex);
    *ppMutex = NULL;
    CloseHandle(mutexHandle);
#elif UDPLATFORM_LINUX
    pthread_mutex_t *mutex = (pthread_mutex_t *)(*ppMutex);
    pthread_mutex_destroy(mutex);
    udFree(mutex);
    *ppMutex = nullptr;
#else
#  error Unknown platform
#endif
  }
}

// ***************************************************************************************
void udLockMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    WaitForSingleObject((HANDLE)pMutex, INFINITE);
#elif UDPLATFORM_LINUX
    pthread_mutex_lock((pthread_mutex_t *)pMutex);
#else
#   error Unknown platform
#endif
  }
}

// ***************************************************************************************
void udReleaseMutex(udMutex *pMutex)
{
  if (pMutex)
  {
#if UDPLATFORM_WINDOWS
    ReleaseMutex((HANDLE)pMutex);
#elif UDPLATFORM_LINUX
    pthread_mutex_unlock((pthread_mutex_t *)pMutex);
#else
#error Unknown platform
#endif
  }
}


#if __MEMORY_DEBUG__  
# if defined(_MSC_VER)
#   pragma warning(disable:4530) //  C++ exception handler used, but unwind semantics are not enabled. 
# endif
#include <map>

size_t gAddressToBreakOnAllocation = (size_t)-1;
size_t gAllocationCount = 0;
size_t gAllocationCountToBreakOn = (size_t)-1;
size_t gAddressToBreakOnFree = (size_t)-1;

struct MemTrack
{
  void *pMemory;
  size_t size;
  const char *pFile;
  int line;
  size_t allocationNumber;
};

typedef std::map<size_t, MemTrack> MemTrackMap;

static MemTrackMap *pMemoryTrackingMap;
#if UDPLATFORM_WINDOWS
static HANDLE memoryTrackingMutex;
#endif

void udMemoryDebugTrackingInit()
{
#if UDPLATFORM_WINDOWS
  memoryTrackingMutex = CreateMutexA(NULL, false,  "DebugMemoryTrackingMutex");
  if (!memoryTrackingMutex)
  {
    PRINT_ERROR_STRING("Failed to create memory tracking mutex %d", GetLastError());
  }
#endif

  if (!pMemoryTrackingMap)
  {
    pMemoryTrackingMap = new MemTrackMap;
  }

}

void udMemoryDebugTrackingDeinit()
{
  UDASSERT(pMemoryTrackingMap, "pMemoryTrackingMap is NULL");

#if UDPLATFORM_WINDOWS
  uint32_t result = WaitForSingleObject(memoryTrackingMutex, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    PRINT_ERROR_STRING("WaitForSingleObject returned an error %d", result);
    return;
  }
#endif

  delete pMemoryTrackingMap;
  pMemoryTrackingMap = NULL;

#if UDPLATFORM_WINDOWS    
  uint32_t bResult = ReleaseMutex(memoryTrackingMutex);
  if (!bResult)
  {
    PRINT_ERROR_STRING("ReleaseMutex returned an error %d", GetLastError());
  }

  CloseHandle(memoryTrackingMutex);
#endif
}

void udMemoryOutputLeaks()
{
  if (pMemoryTrackingMap)
  {
#if UDPLATFORM_WINDOWS
    uint32_t result = WaitForSingleObject(memoryTrackingMutex, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
      PRINT_ERROR_STRING("WaitForSingleObject returned an error %d", result);
      return;
    }
#endif
    if (pMemoryTrackingMap->size() > 0)
    {
      udDebugPrintf("%d Allocations\n", uint32_t(gAllocationCount)); 

      udDebugPrintf("%d Memory leaks detected\n", pMemoryTrackingMap->size());
      for (MemTrackMap::iterator memIt = pMemoryTrackingMap->begin(); memIt != pMemoryTrackingMap->end(); ++memIt)
      {
        const MemTrack &track = memIt->second;
        udDebugPrintf("%s(%d): Allocation 0x%p Address 0x%p, size %u\n", track.pFile, track.line, (void*)track.allocationNumber, track.pMemory, track.size);
      }
    }

#if UDPLATFORM_WINDOWS    
    uint32_t bResult = ReleaseMutex(memoryTrackingMutex);
    if (!bResult)
    {
      PRINT_ERROR_STRING("ReleaseMutex returned an error %d", GetLastError());
    }
#endif
  }
}

void udMemoryOutputAllocInfo(void *pAlloc)
{
#if UDPLATFORM_WINDOWS
  uint32_t result = WaitForSingleObject(memoryTrackingMutex, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    PRINT_ERROR_STRING("WaitForSingleObject returned an error %d", result);
    return;
  }
#endif

  const MemTrack &track = (*pMemoryTrackingMap)[size_t(pAlloc)];
  udDebugPrintf("%s(%d): Allocation 0x%p Address 0x%p, size %u\n", track.pFile, track.line, (void*)track.allocationNumber, track.pMemory, track.size);

#if UDPLATFORM_WINDOWS    
  uint32_t bResult = ReleaseMutex(memoryTrackingMutex);
  if (!bResult)
  {
    PRINT_ERROR_STRING("ReleaseMutex returned an error %d", GetLastError());
  }
#endif
}

static void DebugTrackMemoryAlloc(void *pMemory, size_t size, const char * pFile, int line)
{
  if (gAddressToBreakOnAllocation == (uint64_t)pMemory || gAllocationCount == gAllocationCountToBreakOn) 
  { 
    udDebugPrintf("Allocation 0x%p address 0x%p, at File %s, line %d", (void*)gAllocationCount, pMemory, pFile, line); 
    __debugbreak(); 
  } 

#if UDPLATFORM_WINDOWS
  uint32_t result = WaitForSingleObject(memoryTrackingMutex, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    PRINT_ERROR_STRING("WaitForSingleObject returned an error %d", result);
    return;
  }
#endif

#if UDASSERT_ON
  size_t sizeOfMap = pMemoryTrackingMap->size();  
#endif
  MemTrack track = { pMemory, size, pFile, line, gAllocationCount };  

  (*pMemoryTrackingMap)[size_t(pMemory)] = track; 

  ++gAllocationCount;  

  UDASSERT(pMemoryTrackingMap->size() > sizeOfMap, "map didn't grow") // I think this is incorrect as the map may not need to grow if its reusing a slot that has been freed.

#if UDPLATFORM_WINDOWS    
  uint32_t bResult = ReleaseMutex(memoryTrackingMutex);
  if (!bResult)
  {
    PRINT_ERROR_STRING("ReleaseMutex returned an error %d", GetLastError());
  }
#endif
}

static void DebugTrackMemoryFree(void *pMemory, const char * pFile, int line)   
{
# if UDASSERT_ON
  size_t sizeOfMap; 
# endif


  if (gAddressToBreakOnFree == (uint64_t)pMemory) 
  { 
    udDebugPrintf("Allocation 0x%p address 0x%p, at File %s, line %d", (void*)gAllocationCount, pMemory, pFile, line); 
    __debugbreak(); 
  } 

#if UDPLATFORM_WINDOWS
  uint32_t result = WaitForSingleObject(memoryTrackingMutex, INFINITE);
  if (result != WAIT_OBJECT_0)
  {
    PRINT_ERROR_STRING("WaitForSingleObject returned an error %d", result);
    return;
  }
#endif

  if (pMemoryTrackingMap)
  {
    MemTrackMap::iterator it = pMemoryTrackingMap->find(size_t(pMemory));
    if (it == pMemoryTrackingMap->end())
    {
      udDebugPrintf("Error freeing address %p at File %s, line %d, did not find a matching allocation", pMemory, pFile, line); 
      __debugbreak(); 
      goto epilogue;
    }
    UDASSERT(it->second.pMemory == (pMemory), "Pointers didn't match");

# if UDASSERT_ON
    sizeOfMap = pMemoryTrackingMap->size(); 
# endif
    pMemoryTrackingMap->erase(it); 

    UDASSERT(pMemoryTrackingMap->size() < sizeOfMap, "map didn't shrink");
  }

epilogue:

#if UDPLATFORM_WINDOWS    
 uint32_t bResult = ReleaseMutex(memoryTrackingMutex);
  if (!bResult)
  {
    PRINT_ERROR_STRING("ReleaseMutex returned an error %d", GetLastError());
  }
#else
;
#endif
}

#else
# define DebugTrackMemoryAlloc(pMemory, size, pFile, line)
# define DebugTrackMemoryFree(pMemory, pFile, line)
#endif // __MEMORY_DEBUG__


void *_udAlloc(size_t size, udAllocationFlags flags IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
  void *pMemory = (flags & udAF_Zero) ? calloc(size, 1) : malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("_udAlloc failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  return pMemory;
}

void *_udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
#if defined(_MSC_VER)
  void *pMemory =  (flags & udAF_Zero) ? _aligned_recalloc(nullptr, size, 1, alignment) : _aligned_malloc(size, alignment);

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("_udAllocAligned failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE

#elif defined(__GNUC__)
  if (alignment < sizeof(size_t))
  {
    alignment = sizeof(size_t);
  }
  void *pMemory;
  int err = posix_memalign(&pMemory, alignment, size + alignment);
  if (err != 0)
  {
	  return nullptr;
  }

  if (flags & udAF_Zero)
  {
    memset(pMemory, 0, size);
  }
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

void *_udRealloc(void *pMemory, size_t size IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
#if __MEMORY_DEBUG__
  if (pMemory)
  {
    DebugTrackMemoryFree(pMemory, pFile, line);
  }
#endif  
  pMemory = realloc(pMemory, size);

#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("_udRealloc failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);


  return pMemory;
}

void *_udReallocAligned(void *pMemory, size_t size, size_t alignment IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
#if __MEMORY_DEBUG__
  if (pMemory)
  {
    DebugTrackMemoryFree(pMemory, pFile, line);
  }
#endif

#if defined(_MSC_VER)
  pMemory = _aligned_realloc(pMemory, size, alignment);
#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("_udReallocAligned failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
#elif defined(__GNUC__)
  if (!pMemory)
  {
    pMemory = _udAllocAligned(size, alignment, udAF_None IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line));
  }
  else
  {
    void *pNewMem = _udAllocAligned(size, alignment, udAF_None IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line));

    size_t *pSize = (size_t*)((uint8_t*)pMemory - sizeof(size_t));
    memcpy(pNewMem, pMemory, *pSize);
    _udFree(&pMemory IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line));

    return pNewMem;
  }
#endif
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  
  return pMemory;
}

void _udFree(void **ppMemory IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
  if (*ppMemory)
  {
    DebugTrackMemoryFree(*ppMemory, pFile, line);
    free(*ppMemory);
    *ppMemory = NULL;
  }
}


void *operator new (size_t size, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  void *pMemory = malloc(size);
#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("operator new failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);
  return pMemory;
}

void *operator new[] (size_t size, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  void *pMemory = malloc(size);
#if __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  if (!pMemory)
  {
    udDebugPrintf("operator new[] failure, %llu", size);
    __debugbreak();
  }
#endif // __BREAK_ON_MEMORY_ALLOCATION_FAILURE
  DebugTrackMemoryAlloc(pMemory, size, pFile, line);
  return pMemory;
}

void operator delete (void *pMemory, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  if (pMemory)
  {
    DebugTrackMemoryFree(pMemory, pFile, line);
    free(pMemory);
  }
}

void operator delete [](void *pMemory, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  if (pMemory)
  {
    UDASSERT(false, "operator delete [] not supported yet");
    DebugTrackMemoryFree(pMemory, pFile, line);
    free(pMemory);
  }
}




udResult udGetTotalPhysicalMemory(uint64_t *pTotalMemory)
{
#if UDPLATFORM_WINDOWS
  MEMORYSTATUSEX memorySatusEx;
  memorySatusEx.dwLength = sizeof(memorySatusEx);
  BOOL result = GlobalMemoryStatusEx(&memorySatusEx);
  if (result)
  {
    *pTotalMemory = memorySatusEx.ullTotalPhys;
    return udR_Success;
  }

  *pTotalMemory = 0;
  return udR_Failure_;

#elif UDPLATFORM_LINUX

// see http://nadeausoftware.com/articles/2012/09/c_c_tip_how_get_physical_memory_size_system for 
// explanation.

#if !defined(_SC_PHYS_PAGES)
#error "_SC_PHYS_PAGES is not defined"
#endif

#if !defined(_SC_PAGESIZE)
#error "_SC_PAGESIZE is not defined"
#endif

  *pTotalMemory = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGESIZE);
  return udR_Success;

#else
  *pTotalMemory = 0;
  return udR_Success;
#endif
}