#ifndef UDPLATFORM_H
#define UDPLATFORM_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, August 2013
//
// Platform and architecture specific definitions
//

#include "udResult.h"

// An abstraction layer for common functions that differ on various platforms
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN64) || defined(__amd64__) || defined(__arm64__) || defined(__aarch64__)
//64-bit code
#  define UD_64BIT      (1)
#  define UD_32BIT      (0)
#  define UD_WORD_SHIFT 6 // 6 bits for 64 bit pointer
#  define UD_WORD_BITS  64
#  define UD_WORD_BYTES 8
#  define UD_WORD_MAX   0x7fffffffffffffffLL
typedef signed long long udIWord;
typedef unsigned long long udUWord;
#elif defined(_WIN32) || defined(__i386__) || defined(__arm__) || defined(__native_client__) || defined(EMSCRIPTEN)
//32-bit code
#  define UD_64BIT      (0)
#  define UD_32BIT      (1)
#  define UD_WORD_SHIFT 5 // 5 bits for 32 bit pointer
#  define UD_WORD_BITS  32
#  define UD_WORD_BYTES 4
#  define UD_WORD_MAX   0x7fffffffL
typedef signed long udIWord;
typedef unsigned long udUWord;
#else
#  error "Unknown architecture (32/64 bit)"
#endif

#if defined(__native_client__)
#  include <limits.h>
#  include <string.h>
#  define UDPLATFORM_NACL 1
#  define USE_GLES
#elif defined(EMSCRIPTEN)
#  include <limits.h>
#  include <memory.h>
#  include <stddef.h>
#  define UDPLATFORM_EMSCRIPTEN 1
#  if not defined(__EMSCRIPTEN_PTHREADS__)
#    error "PTHREADS are not being used!"
#  endif
#elif defined(_MSC_VER) || defined(__MINGW32__)
#  include <memory.h>
#  define UDPLATFORM_WINDOWS 1
#  include <winapifamily.h>
#  if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
#    define UDPLATFORM_UWP 1
#  endif
#elif defined(__ANDROID__)
#  include <limits.h>
#  include <memory.h>
#  include <stddef.h>
#  define UDPLATFORM_ANDROID 1
#elif defined(__linux__) // TODO: Work out best tag to detect linux here
#  include <limits.h>
#  include <memory.h>
#  include <stddef.h>
#  define UDPLATFORM_LINUX 1
#elif defined(__APPLE__)
#  include "TargetConditionals.h"
#  include <limits.h>
#  include <memory.h>
#  include <stddef.h>
#  if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#    define UDPLATFORM_IOS_SIMULATOR 1
#  elif TARGET_OS_IPHONE
#    define UDPLATFORM_IOS 1
#  else
#    define UDPLATFORM_OSX 1
#  endif
#else
#  error "Unknown platform"
#endif

#ifndef UDPLATFORM_WINDOWS
#  define UDPLATFORM_WINDOWS 0
#endif

#ifndef UDPLATFORM_UWP
#  define UDPLATFORM_UWP 0
#endif

#ifndef UDPLATFORM_ANDROID
#  define UDPLATFORM_ANDROID 0
#endif

#ifndef UDPLATFORM_LINUX
#  define UDPLATFORM_LINUX 0
#endif

#ifndef UDPLATFORM_OSX
#  define UDPLATFORM_OSX 0
#endif

#ifndef UDPLATFORM_IOS_SIMULATOR
#  define UDPLATFORM_IOS_SIMULATOR 0
#endif

#ifndef UDPLATFORM_IOS
#  define UDPLATFORM_IOS 0
#endif

#ifndef UDPLATFORM_NACL
#  define UDPLATFORM_NACL 0
#endif

#ifndef UDPLATFORM_EMSCRIPTEN
#  define UDPLATFORM_EMSCRIPTEN 0
#endif

#if defined(_DEBUG)
#  define UD_DEBUG   1
#  define UD_RELEASE 0
#else
#  define UD_DEBUG   0
#  define UD_RELEASE 1
#endif

#if UDPLATFORM_WINDOWS
#  define udU64L(x)      x##ULL
#  define udI64L(x)      x##LL
#  define UDFORCE_INLINE __forceinline
#elif UDPLATFORM_NACL || UDPLATFORM_EMSCRIPTEN
#  define udU64L(x)      x##ULL
#  define udI64L(x)      x##LL
#  define UDFORCE_INLINE inline
#else
#  define udU64L(x)      x##UL
#  define udI64L(x)      x##L
#  define UDFORCE_INLINE inline
#endif

#if UDPLATFORM_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Intrin.h>
#  include <Windows.h>
#  if UD_32BIT
template <typename T, typename U>
inline T *udInterlockedExchangePointer(T *volatile *dest, U *exchange)
{
  return (T *)_InterlockedExchange((volatile long *)dest, (long)exchange);
}
template <typename T, typename U>
inline T *udInterlockedCompareExchangePointer(T *volatile *dest, U *exchange, U *comparand) { return (T *)_InterlockedCompareExchange((volatile long *)dest, (long)exchange, (long)comparand); }
#  else  // UD_32BIT
template <typename T, typename U>
inline T *udInterlockedExchangePointer(T *volatile *dest, U *exchange)
{
  return (T *)_InterlockedExchangePointer((volatile PVOID *)dest, (PVOID)exchange);
}
template <typename T, typename U>
inline T *udInterlockedCompareExchangePointer(T *volatile *dest, U *exchange, U *comparand) { return (T *)_InterlockedCompareExchangePointer((volatile PVOID *)dest, (PVOID)exchange, (PVOID)comparand); }
#  endif // UD_32BIT
#  define udSleep(x)    Sleep(x)
#  define udYield()     SwitchToThread()
#  define UDTHREADLOCAL __declspec(thread)

#elif UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID || UDPLATFORM_EMSCRIPTEN
#  include <cstddef> // Required for std::nullptr_t below
#  include <sched.h>
#  include <unistd.h>
#  if UDPLATFORM_LINUX && !defined(__clang__) && __GNUC__ < 5
// We're just trying to ignore pedantic warnings on CentOS7 GCC due to function pointers being used in this function below
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-pedantic"
#  endif
template <typename T, typename U>
inline T *udInterlockedExchangePointer(T *volatile *dest, U *exchange)
{
  return (T *)__sync_lock_test_and_set((void *volatile *)dest, (void *)exchange);
}
#  if UDPLATFORM_LINUX && !defined(__clang__) && __GNUC__ < 5
#    pragma GCC diagnostic pop
#  endif
template <typename T, typename U>
inline T *udInterlockedCompareExchangePointer(T *volatile *dest, U *exchange, U *comparand)
{
  return (T *)__sync_val_compare_and_swap((void *volatile *)dest, (void *)comparand, (void *)exchange);
}
#  define udSleep(x) usleep((x)*1000)
#  define udYield(x) sched_yield()
#  if defined(__INTELLISENSE__)
#    define UDTHREADLOCAL
#  else
#    define UDTHREADLOCAL __thread
#  endif

#else
#  error Unknown platform
#endif

// nullptr helpers
template <typename T>
inline T *udInterlockedExchangePointer(T *volatile *dest, std::nullptr_t) { return udInterlockedExchangePointer(dest, (T *)nullptr); }
template <typename T, typename U>
inline T *udInterlockedCompareExchangePointer(T *volatile *dest, std::nullptr_t, U *comparand) { return udInterlockedCompareExchangePointer(dest, (U *)nullptr, comparand); }
template <typename T, typename U>
inline T *udInterlockedCompareExchangePointer(T *volatile *dest, U *exchange, std::nullptr_t) { return udInterlockedCompareExchangePointer(dest, exchange, (U *)nullptr); }

template <typename T>
T udMax(T a, T b) { return (a > b) ? a : b; }
template <typename T>
T udMin(T a, T b) { return (a < b) ? a : b; }
template <typename T, typename U>
T udMax(T a, U b) { return (a > (T)b) ? a : (T)b; }
template <typename T, typename U>
T udMin(T a, U b) { return (a < (T)b) ? a : (T)b; }

#define UDALIGN_POWEROF2(x, b) (((x) + (b)-1) & -(b))

#ifdef __MEMORY_DEBUG__
#  define IF_MEMORY_DEBUG(x, y) x, y
#else
#  define IF_MEMORY_DEBUG(x, y) nullptr, 0
#endif //  __MEMORY_DEBUG__

#if __cplusplus >= 201103L || _MSC_VER >= 1700
#  define UDCPP11 1
#else
#  define UDCPP11 0
#endif

#if __cplusplus >= 201402L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || _MSC_VER >= 1900
#  define UDCPP14 1
#else
#  define UDCPP14 0
#endif

#if __cplusplus >= 201704L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201704L)
#  define UDCPP17 1
#else
#  define UDCPP17 0
#endif

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  define UDCPP20 1
#else
#  define UDCPP20 0
#endif

#if UDCPP17
#  define UD_NO_DISCARD [[nodiscard]]
#else
#  define UD_NO_DISCARD
#endif

#if UDCPP20
#  define UD_NO_DISCARD_(reason) [[nodiscard(reason)]]
#else
#  define UD_NO_DISCARD_(reason)
#endif

#if defined(__clang__) || defined(__GNUC__)
#  if !UDCPP11 && !defined(nullptr)
#    define nullptr NULL
#  endif // !UDCPP11 && !defined(nullptr)
#endif   // defined(__clang__) || defined(__GNUC__)

#if UDCPP11 && !defined(_MSC_VER)
#  include <cstddef>
using std::nullptr_t;
#endif //!defined(_MSC_VER)

#if UDPLATFORM_LINUX || UDPLATFORM_NACL || UDPLATFORM_EMSCRIPTEN
#  include <alloca.h>
#elif UDPLATFORM_UWP
#  include <malloc.h>
#endif

#if defined(_MSC_VER) && !defined(_M_ARM64)
UDFORCE_INLINE void udMemset32(void *pDest, uint32_t val, size_t size)
{
  __stosd((unsigned long *)pDest, val, size);
}
#else
UDFORCE_INLINE void udMemset32(void *pDest, uint32_t val, size_t size)
{
  uint32_t *p = (uint32_t *)pDest;
  while (size--)
    *p++ = val;
}
#endif

UDFORCE_INLINE void *udSetZero(void *pMemory, size_t size)
{
  memset(pMemory, 0, size);
  return pMemory;
}

enum udAllocationFlags
{
  udAF_None = 0,
  udAF_Zero = 1
};

// Inline of operator to allow flags to be combined and retain type-safety
inline udAllocationFlags operator|(udAllocationFlags a, udAllocationFlags b) { return (udAllocationFlags)(int(a) | int(b)); }

void *_udMemDup(const void *pMemory, size_t size, size_t additionalBytes, udAllocationFlags flags, const char *pFile, int line);
#define udMemDup(pMemory, size, additionalBytes, flags) _udMemDup(pMemory, size, additionalBytes, flags, IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udAlloc(size_t size, udAllocationFlags flags, const char *pFile, int line);
#define udAlloc(size) _udAlloc(size, udAF_None, IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udAllocAligned(size_t size, size_t alignment, udAllocationFlags flags, const char *pFile, int line);
#define udAllocAligned(size, alignment, flags) _udAllocAligned(size, alignment, flags, IF_MEMORY_DEBUG(__FILE__, __LINE__))

#define udAllocFlags(size, flags)       _udAlloc(size, flags, IF_MEMORY_DEBUG(__FILE__, __LINE__))
#define udAllocType(type, count, flags) (type *)_udAlloc(sizeof(type) * (count), flags, IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udRealloc(void *pMemory, size_t size, const char *pFile, int line);
#define udRealloc(pMemory, size)            _udRealloc(pMemory, size, IF_MEMORY_DEBUG(__FILE__, __LINE__))
#define udReallocType(pMemory, type, count) (type *)_udRealloc(pMemory, sizeof(type) * (count), IF_MEMORY_DEBUG(__FILE__, __LINE__))

void *_udReallocAligned(void *pMemory, size_t size, size_t alignment, const char *pFile, int line);
#define udReallocAligned(pMemory, size, alignment) _udReallocAligned(pMemory, size, alignment, IF_MEMORY_DEBUG(__FILE__, __LINE__))

void _udFreeInternal(void *pMemory, const char *pFile, int line);
template <typename T>
void _udFree(T *&pMemory, const char *pFile, int line)
{
  void *pActualPtr = (void *)pMemory;
  if (pActualPtr && udInterlockedCompareExchangePointer((void **)&pMemory, nullptr, pActualPtr) == pActualPtr)
  {
    _udFreeInternal((void *)pActualPtr, pFile, line);
  }
}
#define udFree(pMemory) _udFree(pMemory, IF_MEMORY_DEBUG(__FILE__, __LINE__))

// A secure free will overwrite the memory (to the size specified) with a random 32-bit
// constant. For example cryptographic functions use this to overwrite key data before freeing
template <typename T>
void _udFreeSecure(T *&pMemory, size_t size, const char *pFile, int line)
{
  void *pActualPtr = (void *)pMemory;
  if (pActualPtr && udInterlockedCompareExchangePointer((void **)&pMemory, nullptr, pActualPtr) == pActualPtr)
  {
    // Use a simple random value just to avoid filling sensitive memory with a constant
    // that can be used to identify the locations in memory where sensitive data was stored
    int randVal = rand();
    size_t i = size & ~3;
    if (i)
      udMemset32((void *)pActualPtr, randVal, i >> 2); // Fill 32-bits at a time first for performance reasons
    // Fill the last odd bytes
    while (i < size)
      ((uint8_t *)pActualPtr)[i++] = (uint8_t)randVal;
    _udFreeInternal((void *)pActualPtr, pFile, line);
  }
}
#define udFreeSecure(pMemory, size) _udFreeSecure(pMemory, size, IF_MEMORY_DEBUG(__FILE__, __LINE__))

// Wrapper for alloca with flags. Note flags is OR'd with udAF_None to avoid a cppcat warning
#define udAllocStack(type, count, flags) ((flags & udAF_Zero) ? (type *)udSetZero(alloca(sizeof(type) * (count)), sizeof(type) * (count)) : (type *)alloca(sizeof(type) * (count)))
#define udFreeStack(pMemory)

// TODO: Remove these
#define udMemoryDebugTrackingInit()
#define udMemoryOutputLeaks()
#define udMemoryOutputAllocInfo(pAlloc)
#define udMemoryDebugTrackingDeinit()
#define udMemoryDebugLogMemoryStats()
#define udValidateHeap()

#if UDPLATFORM_WINDOWS
#  define udMemoryBarrier() MemoryBarrier()
#else
#  define udMemoryBarrier() __sync_synchronize()
#endif

#define udUnused(x) (void)x

#if defined(_MSC_VER)
#  define __FUNC_NAME__ __FUNCTION__
#elif defined(__GNUC__)
#  define __FUNC_NAME__ __PRETTY_FUNCTION__
#else
#  pragma message("This platform hasn't setup up __FUNC_NAME__")
#  define __FUNC_NAME__ "unknown"
#endif

#if defined(__GNUC__)
#  define UD_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#  if UD_GCC_VERSION < 50000
#    pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  endif
#endif

#if defined(__GNUC__)
#  define UD_PRINTF_FORMAT_FUNC(fmtIndex) __attribute((format(printf, fmtIndex, fmtIndex + 1)))
#else
#  define UD_PRINTF_FORMAT_FUNC(fmtIndex)
#endif

#define MAKE_FOURCC(a, b, c, d) ((((uint32_t)(a)) << 0) | (((uint32_t)(b)) << 8) | (((uint32_t)(c)) << 16) | (((uint32_t)(d)) << 24))

template <typename T, size_t N>
constexpr size_t udLengthOf(T (&)[N])
{
  return N;
}
#define UDARRAYSIZE udLengthOf

// Get physical memory available if possible, otherwise outputs 0 to *pTotalMemory and returns udR_Failure
udResult udGetTotalPhysicalMemory(uint64_t *pTotalMemory);

// CPU Feature tests
bool udCPUSupportsAVX();
bool udCPUSupportsAVX2();

#include "udDebug.h"

#endif // UDPLATFORM_H
