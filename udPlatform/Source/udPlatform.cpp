#include "udPlatform.h"
#include <stdlib.h>
#include <string.h>

uint64_t udCreateThread(udThreadStart *threadStarter, void *threadData)
{
#if UDPLATFORM_WINDOWS
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
#if UDPLATFORM_WINDOWS
  CloseHandle((HANDLE)threadHandle);
#else
  // TODO: FIgure out which pthread function is *most* equivalent
#endif
}

#if __MEMORY_DEBUG__  
# if defined(_MSC_VER)
#   pragma warning(disable:4530) //  C++ exception handler used, but unwind semantics are not enabled. 
# endif
#include <map>

size_t gAddressToBreakOnAllocation = (size_t)-1;
size_t gAllocationCount = 0;
size_t gAllocationCountToBreakOn = (size_t)-1;

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

void udMemoryOutputLeaks()
{
  if (pMemoryTrackingMap)
  {
    if (pMemoryTrackingMap->size() > 0)
    {
      udDebugPrintf("%d Allocations\n", uint32_t(gAllocationCount)); 

      udDebugPrintf("%d Memory leaks detected\n", pMemoryTrackingMap->size());
      for (MemTrackMap::iterator memIt = pMemoryTrackingMap->begin(); memIt != pMemoryTrackingMap->end(); ++memIt)
      {
        const MemTrack &track = memIt->second;
        udDebugPrintf("%s(%d): Allocation 0x%x%x Address %p, size %u\n", track.pFile, track.line, uint32_t(track.allocationNumber >> 32), uint32_t(track.allocationNumber), track.pMemory, track.size);
      }
    }
    delete pMemoryTrackingMap;
    pMemoryTrackingMap = NULL;
  }
}

void udMemoryOutputAllocInfo(void *pAlloc)
{  
  const MemTrack &track = (*pMemoryTrackingMap)[size_t(pAlloc)];
  udDebugPrintf("%s(%d): Allocation 0x%x%x Address %p, size %u\n", track.pFile, track.line, uint32_t(track.allocationNumber >> 32), uint32_t(track.allocationNumber), track.pMemory, track.size);
}

static void DebugTrackMemoryAlloc(void *pMemory, size_t size, const char * pFile, int line)
{
  if (gAddressToBreakOnAllocation == (uint64_t)pMemory || gAllocationCount == gAllocationCountToBreakOn) 
  { 
    udDebugPrintf("Allocation 0x%x%x address %p, at File %s, line %d", uint32_t(gAllocationCount >> 32), uint32_t(gAllocationCount), pMemory, pFile, line); 
    __debugbreak(); 
  } 

  if (!pMemoryTrackingMap)
  {
    pMemoryTrackingMap = new MemTrackMap;
  }

#if UDASSERT_ON
  size_t sizeOfMap = pMemoryTrackingMap->size();  
#endif
  MemTrack track = { pMemory, size, pFile, line, gAllocationCount };  

  (*pMemoryTrackingMap)[size_t(pMemory)] = track; 

  ++gAllocationCount;  

  UDASSERT(pMemoryTrackingMap->size() > sizeOfMap, "map didn't grow")
}

static void DebugTrackMemoryFree(void *pMemory, const char * pFile, int line)   
{
  if (pMemoryTrackingMap)
  {
    MemTrackMap::iterator it = pMemoryTrackingMap->find(size_t(pMemory));
    if (it == pMemoryTrackingMap->end())
    {
      udDebugPrintf("Error freeing address %p at File %s, line %d, did not find a matching allocation", pMemory, pFile, line); 
      __debugbreak(); 
      return;
    }
    UDASSERT(it->second.pMemory == (pMemory), "Pointers didn't match");

# if UDASSERT_ON
    size_t sizeOfMap = pMemoryTrackingMap->size(); 
# endif
    pMemoryTrackingMap->erase(it); 

    UDASSERT(pMemoryTrackingMap->size() < sizeOfMap, "map didn't shrink");
  }
}

#else
# define DebugTrackMemoryAlloc(pMemory, size, pFile, line)
# define DebugTrackMemoryFree(pMemory, pFile, line)
#endif // __MEMORY_DEBUG__



void *_udAlloc(size_t size IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
  void *pMemory = malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

void *_udAllocAligned(size_t size, size_t alignment IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
#if defined(_MSC_VER)
  void *pMemory = _aligned_malloc(size, alignment);
#elif defined(__GNUC__)
  if (alignment < sizeof(size_t))
  {
    alignment = sizeof(size_t);
  }
  void *pAllocation;
  int err = posix_memalign(&pAllocation, alignment, size + alignment);
  if (err != 0)
  {
	  return nullptr;
  }

  size_t *pSizeHeader = (size_t *)pAllocation + alignment - sizeof(size_t);
  *pSizeHeader = size;
  void *pMemory = (uint8_t*)pAllocation + alignment;
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
#elif defined(__GNUC__)
  if (!pMemory)
  {
    pMemory = _udAllocAligned(size, alignment IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line));
  }
  else
  {
    void *pNewMem = _udAllocAligned(size, alignment IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line));

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
  DebugTrackMemoryFree(*ppMemory, pFile, line);

  free(*ppMemory);

  *ppMemory = NULL;
}


void *operator new (size_t size, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  void *pMemory = malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;

}

void *operator new[] (size_t size, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  UDASSERT(false, "operator new [] not supported yet");

  void *pMemory = malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

void operator delete (void *pMemory, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  DebugTrackMemoryFree(pMemory, pFile, line);

  free(pMemory);
}

void operator delete [](void *pMemory, udMemoryOverload udUnusedParam(memoryOverload) IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  UDASSERT(false, "operator delete [] not supported yet");

  DebugTrackMemoryFree(pMemory, pFile, line);

  free(pMemory);
}
