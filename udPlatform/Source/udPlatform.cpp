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

#if __MEMORY_DEBUG__
#pragma warning(disable:4530) //  C++ exception handler used, but unwind semantics are not enabled. 
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
        udDebugPrintf("Allocation 0x%x%x Address %p, size %u, file %s, line %d\n", uint32_t(track.allocationNumber >> 32), uint32_t(track.allocationNumber), track.pFile, track.size, track.pFile, track.line);
      }
    }
    delete pMemoryTrackingMap;
  }
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
  MemTrackMap::iterator it = pMemoryTrackingMap->find(size_t(pMemory));
  if (it == pMemoryTrackingMap->end())
  {
    udDebugPrintf("Error freeing address %p at File %s, line %d, did not find a matching allocation", pMemory, pFile, line); 
    __debugbreak(); 
    return;
  }
  UDASSERT(it->second.pMemory == (pMemory), "Pointers didn't match");

#if UDASSERT_ON
  size_t sizeOfMap = pMemoryTrackingMap->size(); 
#endif
  pMemoryTrackingMap->erase(it); 

  UDASSERT(pMemoryTrackingMap->size() < sizeOfMap, "map didn't shrink");
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
  void *pMemory = _aligned_malloc(size, alignment);

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

  pMemory = _aligned_realloc(pMemory, size, alignment);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  
  return pMemory;
}

void _udFree(void **ppMemory IF_MEMORY_DEBUG(,const char * pFile) IF_MEMORY_DEBUG(,int line))
{
  DebugTrackMemoryFree(*ppMemory, pFile, line);

  free(*ppMemory);

  *ppMemory = NULL;
}


void *operator new (size_t size, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  udUnused(memoryOverload);
  void *pMemory = malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;

}

void *operator new[] (size_t size, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  UDASSERT(false, "operator new [] not supported yet");

  udUnused(memoryOverload);

  void *pMemory = malloc(size);

  DebugTrackMemoryAlloc(pMemory, size, pFile, line);

  return pMemory;
}

void operator delete (void *pMemory, udMemoryOverload memoryOverload  IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  udUnused(memoryOverload);

  DebugTrackMemoryFree(pMemory, pFile, line);

  free(pMemory);
}

void operator delete [](void *pMemory, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line))
{
  UDASSERT(false, "operator delete [] not supported yet");
  udUnused(memoryOverload);

  DebugTrackMemoryFree(pMemory, pFile, line);

  free(pMemory);
}
