#include "udPlatformUtil.h"
#include "udThread.h"
#include <stdlib.h>
#include <string.h>

#if UDPLATFORM_WINDOWS
# include <crtdbg.h>
#elif UDPLATFORM_OSX
# include <sys/sysctl.h>
#endif

#if !UDPLATFORM_WINDOWS
# if defined(__i386__) || defined(__amd64__)
#   include <cpuid.h>
# endif
#endif

#define __BREAK_ON_MEMORY_ALLOCATION_FAILURE 0
#define DebugTrackMemoryAlloc(pMemory, size, pFile, line) udUnused(pMemory); udUnused(size); udUnused(pFile); udUnused(line);
#define DebugTrackMemoryFree(pMemory, pFile, line) udUnused(pMemory); udUnused(pFile); udUnused(line);


// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2022
void *udMemDup(const void *pMemory, size_t size, size_t additionalBytes /*= 0*/, udAllocationFlags flags /*= udAF_None*/, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
  void *pDuplicated = udAlloc(size + additionalBytes, udAF_None, location);

  if (pDuplicated)
  {
    memcpy(pDuplicated, pMemory, size);

    if (additionalBytes && (flags & udAF_Zero))
      memset(udAddBytes(pDuplicated, size), 0, additionalBytes);
  }
  else if constexpr (__BREAK_ON_MEMORY_ALLOCATION_FAILURE)
  {
    udDebugPrintf("udMemDup failure, %zu", size + additionalBytes);
    __debugbreak();
  }

  return pDuplicated;
}

#define UD_DEFAULT_ALIGNMENT (8)
// ----------------------------------------------------------------------------
// Author: David Ely
void *udAlloc(size_t size, udAllocationFlags flags /*= udAF_None*/, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
#if defined(_MSC_VER)
  void *pMemory = (flags & udAF_Zero) ? _aligned_recalloc_dbg(nullptr, size, 1, UD_DEFAULT_ALIGNMENT, location.file_name(), location.line()) : _aligned_malloc_dbg(size, UD_DEFAULT_ALIGNMENT, location.file_name(), location.line());
#else
  void *pMemory = (flags & udAF_Zero) ? calloc(size, 1) : malloc(size);
#endif // defined(_MSC_VER)

  DebugTrackMemoryAlloc(pMemory, size, location.file_name(), location.line());

  if constexpr (__BREAK_ON_MEMORY_ALLOCATION_FAILURE)
  {
    if (!pMemory)
    {
      udDebugPrintf("udAlloc failure, %zu", size);
      __debugbreak();
    }
  }

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags /*= udAF_None*/, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
#if defined(_MSC_VER)
  void *pMemory =  (flags & udAF_Zero) ? _aligned_recalloc_dbg(nullptr, size, 1, alignment, location.file_name(), location.line()) : _aligned_malloc_dbg(size, alignment, location.file_name(), location.line());

  if constexpr (__BREAK_ON_MEMORY_ALLOCATION_FAILURE)
  {
    if (!pMemory)
    {
      udDebugPrintf("udAllocAligned failure, %zu", size);
      __debugbreak();
    }
  }

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
#else
# error "Unsupported platform!"
#endif
  DebugTrackMemoryAlloc(pMemory, size, location.file_name(), location.line());

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *udRealloc(void *pMemory, size_t size, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
  DebugTrackMemoryFree(pMemory, location.file_name(), location.line());
#if defined(_MSC_VER)
  pMemory =  _aligned_realloc_dbg(pMemory, size, UD_DEFAULT_ALIGNMENT, location.file_name(), location.line());
#else
  pMemory = realloc(pMemory, size);
#endif // defined(_MSC_VER)

  if constexpr (__BREAK_ON_MEMORY_ALLOCATION_FAILURE)
  {
    if (!pMemory)
    {
      udDebugPrintf("udRealloc failure, %zu", size);
      __debugbreak();
    }
  }

  DebugTrackMemoryAlloc(pMemory, size, location.file_name(), location.line());

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void *udReallocAligned(void *pMemory, size_t size, size_t alignment, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
  DebugTrackMemoryFree(pMemory, location.file_name(), location.line());
#if defined(_MSC_VER)
  pMemory = _aligned_realloc_dbg(pMemory, size, alignment, location.file_name(), location.line());

  if constexpr (__BREAK_ON_MEMORY_ALLOCATION_FAILURE)
  {
    if (!pMemory)
    {
      udDebugPrintf("udReallocAligned failure, %zu", size);
      __debugbreak();
    }
  }
#elif defined(__GNUC__)
  if (!pMemory)
  {
    pMemory = udAllocAligned(size, alignment, udAF_None, location);
  }
  else
  {
    void *pNewMem = udAllocAligned(size, alignment, udAF_None, location);

    size_t *pSize = (size_t*)((uint8_t*)pMemory - sizeof(size_t));
    memcpy(pNewMem, pMemory, *pSize);
    udFree(pMemory, location);

    return pNewMem;
  }
#else
# error "Unsupported platform!"
#endif
  DebugTrackMemoryAlloc(pMemory, size, location.file_name(), location.line());

  return pMemory;
}

// ----------------------------------------------------------------------------
// Author: David Ely
void udFreeInternal(void * pMemory, const std::source_location location /*= MEMORY_DEBUG_LOCATION()*/)
{
  DebugTrackMemoryFree(pMemory, location.file_name(), location.line());
#if defined(_MSC_VER)
  _aligned_free_dbg(pMemory);
#else
  free(pMemory);
#endif // defined(_MSC_VER)
}

// ----------------------------------------------------------------------------
// Author: David Ely
udResult udGetTotalPhysicalMemory(uint64_t *pTotalMemory)
{
  if (pTotalMemory == nullptr)
    return udR_InvalidParameter;

#if UDPLATFORM_WINDOWS
  MEMORYSTATUSEX memorySatusEx;
  memorySatusEx.dwLength = sizeof(memorySatusEx);
  BOOL result = GlobalMemoryStatusEx(&memorySatusEx);
  if (result)
  {
    *pTotalMemory = memorySatusEx.ullTotalPhys;
    return udR_Success;
  }

#elif UDPLATFORM_LINUX
  // see http://nadeausoftware.com/articles/2012/09/c_c_tip_how_get_physical_memory_size_system for
  // explanation.

# if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    *pTotalMemory = (uint64_t)sysconf(_SC_PHYS_PAGES) * (uint64_t)sysconf(_SC_PAGESIZE);
    return udR_Success;
# endif

#elif UDPLATFORM_OSX
  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;

  size_t len = sizeof(*pTotalMemory);
  if (sysctl(mib, 2, pTotalMemory, &len, NULL, 0) == 0)
    return udR_Success;

#endif

  // All platforms that fail or don't support the function exit here
  *pTotalMemory = 0;
  return udR_Failure;
}

#if ((defined(_WIN32) || defined(_WIN64)) && (_M_IX86 || _M_X64)) || defined(__i386__) || defined(__amd64__)
# define UD_HASCPUID
#endif

class udCPUFeatureDetection
{
public:
  udCPUFeatureDetection()
  {
    DetectFeatures();
  }

  static void DetectFeatures();

private:
#if defined(UD_HASCPUID)
  static void cpuid(int out[4], int eax, int ecx);
#endif
};

static udCPUFeatureDetection s_cpuFeatureDetectionStartup;
static bool s_udCPUSupportsAVX = false;
static bool s_udCPUSupportsAVX2 = false;

bool udCPUSupportsAVX()
{
  udCPUFeatureDetection::DetectFeatures();
  return s_udCPUSupportsAVX;
}

bool udCPUSupportsAVX2()
{
  udCPUFeatureDetection::DetectFeatures();
  return s_udCPUSupportsAVX2;
}

void udCPUFeatureDetection::DetectFeatures()
{
  static bool s_udCPUFeaturesDetected = false;
  if (s_udCPUFeaturesDetected)
    return;

#if defined(UD_HASCPUID)
  int info[4] = {};

  // Get highest valid function ID
  cpuid(info, 0, 0);
  int nIds = info[0];

  // Get flags for function 0x00000001
  if (nIds >= 0x00000001)
  {
    cpuid(info, 0x00000001, 0);
    s_udCPUSupportsAVX = (info[2] & (1 << 28)) != 0;
  }

  // Get flags for function 0x00000007
  if (nIds >= 0x00000007)
  {
    cpuid(info, 0x00000007, 0);
    s_udCPUSupportsAVX2 = (info[1] & (1 << 5)) != 0;
  }
#endif

  s_udCPUFeaturesDetected = true;
}

#if defined(UD_HASCPUID)
void udCPUFeatureDetection::cpuid(int out[4], int eax, int ecx)
{
#if UDPLATFORM_WINDOWS
  __cpuidex(out, eax, ecx);
#else
  __cpuid_count(eax, ecx, out[0], out[1], out[2], out[3]);
#endif
}
#endif
