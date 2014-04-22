#ifndef UDPLATFORM_H
#define UDPLATFORM_H

#include "udResult.h"

// This must be set on all platforms for large files to work
#if (_FILE_OFFSET_BITS != 64)
#error "_FILE_OFFSET_BITS not defined to 64"
#endif

#if defined(_MSC_VER)
# define _CRT_SECURE_NO_WARNINGS
# define fseeko _fseeki64
# define ftello _ftelli64
# if !defined(_OFF_T_DEFINED)
    typedef __int64 _off_t;
    typedef _off_t off_t;
#   define _OFF_T_DEFINED
# endif //_OFF_T_DEFINED
#elif defined(__linux__)
# if !defined(_LARGEFILE_SOURCE )
  // This must be set for linux to expose fseeko and ftello 
# error "_LARGEFILE_SOURCE  not defined"
#endif

#endif



// An abstraction layer for common functions that differ on various platforms
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN64) || defined(__amd64__)
  //64-bit code
# define UD_64BIT
# define UD_WORD_SHIFT  6   // 6 bits for 64 bit pointer
# define UD_WORD_BITS   64
# define UD_WORD_BYTES  8
# define UD_WORD_MAX    0x7fffffffffffffffLL
  typedef signed long long udIWord;
  typedef unsigned long long udUWord;
#elif  defined(_WIN32) || defined(__i386__)  || defined(__arm__)
   //32-bit code
# define UD_32BIT
# define UD_WORD_SHIFT  5   // 5 bits for 32 bit pointer  
# define UD_WORD_BITS   32
# define UD_WORD_BYTES  4
# define UD_WORD_MAX    0x7fffffffL
  typedef signed long udIWord;
  typedef unsigned long udUWord;
#else
# error "Unknown architecture (32/64 bit)"
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
# define UDPLATFORM_WINDOWS (1)
# define UDPLATFORM_LINUX   (0)
#elif defined(__linux__) // TODO: Work out best tag to detect linux here
# define UDPLATFORM_WINDOWS (0)
# define UDPLATFORM_LINUX   (1)
#else
# define UDPLATFORM_WINDOWS (0)
# define UDPLATFORM_LINUX   (0)
# error "Unknown platform"
#endif

#if defined(_DEBUG)
# define UD_DEBUG   (1)
# define UD_RELEASE (0)
#else
# define UD_DEBUG   (0)
# define UD_RELEASE (1)
#endif


#if UDPLATFORM_WINDOWS
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#include <Intrin.h>
inline int32_t udInterlockedPreIncrement(volatile int32_t *p)  { return _InterlockedIncrement((long*)p); }
inline int32_t udInterlockedPostIncrement(volatile int32_t *p) { return _InterlockedIncrement((long*)p) - 1; }
inline int32_t udInterlockedPreDecrement(volatile int32_t *p) { return _InterlockedDecrement((long*)p); }
inline int32_t udInterlockedPostDecrement(volatile int32_t *p) { return _InterlockedDecrement((long*)p) + 1; }
inline int32_t udInterlockedExchange(volatile int32_t *dest, int32_t exchange) { return _InterlockedExchange((volatile long*)dest, exchange); }
inline int32_t udInterlockedCompareExchange(volatile int32_t *dest, int32_t exchange, int32_t comparand) { return _InterlockedCompareExchange((volatile long*)dest, exchange, comparand); }
# if defined(UD_32BIT)
inline void *udInterlockedCompareExchangePointer(void ** volatile dest, void *exchange, void *comparand) { return (void*)_InterlockedCompareExchange((volatile long *)dest, (long)exchange, (long)comparand); }
# else // defined(UD_32BIT)
inline void *udInterlockedCompareExchangePointer(void ** volatile dest, void *exchange, void *comparand) { return _InterlockedCompareExchangePointer(dest, exchange, comparand); }
# endif // defined(UD_32BIT)
# define udSleep(x) Sleep(x)

#elif UDPLATFORM_LINUX
#include <unistd.h>
inline long udInterlockedPreIncrement(volatile int32_t *p)  { return __sync_add_and_fetch(p, 1); }
inline long udInterlockedPostIncrement(volatile int32_t *p) { return __sync_fetch_and_add(p, 1); }
inline long udInterlockedPreDecrement(volatile int32_t *p)  { return __sync_add_and_fetch(p, -1); }
inline long udInterlockedPostDecrement(volatile int32_t *p) { return __sync_fetch_and_add(p, -1); }
inline long udInterlockedExchange(volatile int32_t *dest, int32_t exchange) { return __sync_lock_test_and_set(dest, exchange); }
inline long udInterlockedCompareExchange(volatile int32_t *dest, int32_t exchange, int32_t comparand) { return __sync_val_compare_and_swap(dest, comparand, exchange); }
inline void *udInterlockedCompareExchangePointer(void ** volatile dest, void *exchange, void *comparand) { return __sync_val_compare_and_swap(dest, comparand, exchange); }
# define udSleep(x) usleep((x)*1000)

#else
#error Unknown platform
#endif
// Helpers to perform various interlocked functions based on the platform-wrapped primitives
inline long udInterlockedAdd(volatile int32_t *p, int32_t amount) { int32_t prev, after; do { prev = *p; after = prev + amount; } while (udInterlockedCompareExchange(p, after, prev) != prev); return after; }
inline long udInterlockedMin(volatile int32_t *dest, int32_t newValue) { for (;;) { int32_t oldValue = *dest; if (oldValue < newValue) return oldValue; if (udInterlockedCompareExchange(dest, newValue, oldValue) == oldValue) return newValue; } }
inline long udInterlockedMax(volatile int32_t *dest, int32_t newValue) { for (;;) { int32_t oldValue = *dest; if (oldValue > newValue) return oldValue; if (udInterlockedCompareExchange(dest, newValue, oldValue) == oldValue) return newValue; } }

class udInterlockedInt32
{
public:
  // Get the value
  int32_t Get()                 { return m_value; }
  // Set a new value, returning the previous value
  int32_t Set(int32_t v)        { return udInterlockedExchange(&m_value, v); }
  // Set to the minimum of the existing or new value
  void SetMin(int32_t v)        { udInterlockedMin(&m_value, v); }
  // Set to the maximum of the existing or new value
  void SetMax(int32_t v)        { udInterlockedMax(&m_value, v); }
  // Increment operators
  int32_t operator++()          { return udInterlockedPreIncrement(&m_value);       }
  int32_t operator++(int)       { return udInterlockedPostIncrement(&m_value);      }
  int32_t operator--()          { return udInterlockedPreDecrement(&m_value);       }
  int32_t operator--(int)       { return udInterlockedPostDecrement(&m_value);      }
protected:
  volatile int32_t m_value;
};


// Minimalist MOST BASIC cross-platform thread support
struct udSemaphore;
struct udMutex;
typedef uint64_t udThreadHandle;

typedef uint32_t (udThreadStart)(void *data);
udThreadHandle udCreateThread(udThreadStart *threadStarter, void *threadData); // Returns thread handle
void udDestroyThread(udThreadHandle threadHandle);

udSemaphore *udCreateSemaphore(int maxValue, int initialValue);
void udDestroySemaphore(udSemaphore **ppSemaphore);
void udIncrementSemaphore(udSemaphore *pSemaphore);
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs); // Returns zero on success

udMutex *udCreateMutex();
void udDestroyMutex(udMutex **ppMutex);
void udLockMutex(udMutex *pMutex);
void udReleaseMutex(udMutex *pMutex);

// A convenience class to lock and unlock based on scope of the variable
class udScopeLock
{
public:
  udScopeLock(udMutex *mutex) { m_mutex = mutex; udLockMutex(m_mutex); }
  ~udScopeLock() { udReleaseMutex(m_mutex); }
protected:
  udMutex *m_mutex;
};

#define UDALIGN_POWEROF2(x,b) (((x)+(b)-1) & -(b))

#define __MEMORY_DEBUG__ (0)

#if __MEMORY_DEBUG__
# define IF_MEMORY_DEBUG(x,y) x,y
#else 
# define IF_MEMORY_DEBUG(x,y) 
#endif //  __MEMORY_DEBUG__

void *_udAlloc(size_t size IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));
void *_udAllocAligned(size_t size, size_t alignment IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));

void *_udRealloc(void *pMemory, size_t size IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));
void *_udReallocAligned(void *pMemory, size_t size, size_t alignment IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));

void _udFree(void **pMemory IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));


enum udMemoryOverload
{
  UDMO_Memory
};

void *operator new (size_t size, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));
void *operator new[] (size_t size, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));

void operator delete (void *p, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));
void operator delete [](void *p, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line));

template <typename T> void _udDelete(T *&pMemory, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line)) { if (pMemory) { pMemory->~T(); operator delete (pMemory, memoryOverload IF_MEMORY_DEBUG(, pFile) IF_MEMORY_DEBUG(, line)); pMemory = NULL; } }
template <typename T> void _udDeleteArray(T *&pMemory, udMemoryOverload memoryOverload IF_MEMORY_DEBUG(,const char * pFile ) IF_MEMORY_DEBUG(,int  line)) { if (pMemory) { /*pMemory->~T(); */operator delete [] (pMemory, memoryOverload IF_MEMORY_DEBUG(,pFile) IF_MEMORY_DEBUG(, line)); pMemory = NULL; }}


#if __MEMORY_DEBUG__
#  define udAlloc(size) _udAlloc(size, __FILE__, __LINE__)
#  define udAllocType(type,count) (type*)_udAlloc(sizeof(type) * (count), __FILE__, __LINE__)
#  define udAllocAligned(size, alignment) _udAllocAligned(size, alignment, __FILE__, __LINE__)

#  define udRealloc(pMemory, size) _udRealloc(pMemory, size, __FILE__, __LINE__)
#  define udReallocAligned(pMemory, size, alignment) _udReallocAligned(pMemory, size, alignment, __FILE__, __LINE__)

#  define udFree(pMemory) _udFree((void**)&pMemory, __FILE__, __LINE__)

#  define udNew new (UDMO_Memory, __FILE__, __LINE__)
#  define udNewArray new [] (UDMO_Memory, __FILE__, __LINE__)

#  define udDelete(pMemory) _udDelete(pMemory, UDMO_Memory, __FILE__, __LINE__)
#  define udDeleteArray(pMemory) _udDeleteArray(pMemory, UDMO_Memory, __FILE__, __LINE__)

#else //  __MEMORY_DEBUG__
#  define udAlloc(size) _udAlloc(size)
#  define udAllocType(type, count) (type*)_udAlloc(sizeof(type) * (count))
#  define udAllocAligned(size, alignment) _udAllocAligned(size, alignment)

#  define udRealloc(pMemory, size) _udRealloc(pMemory, size)
#  define udReallocAligned(pMemory, size, alignment) _udReallocAligned(pMemory, size, alignment)

#  define udFree(pMemory) _udFree((void**)&pMemory)

#  define udNew new (UDMO_Memory)
#  define udNewArray new [] (UDMO_Memory)

#  define udDelete(pMemory) _udDelete(pMemory, UDMO_Memory)
#  define udDeleteArray(pMemory) _udDeleteArray(pMemory, UDMO_Memory)

#endif  //  __MEMORY_DEBUG__

#if __MEMORY_DEBUG__
void udMemoryDebugTrackingInit();
void udMemoryOutputLeaks();
void udMemoryOutputAllocInfo(void *pAlloc);
void udMemoryDebugTrackingDeinit();
#else
# define udMemoryDebugTrackingInit()
# define udMemoryOutputLeaks()
#define udMemoryOutputAllocInfo(pAlloc)
#define udMemoryDebugTrackingDeinit()
#endif // __MEMORY_DEBUG__

#if UDPLATFORM_WINDOWS
# define udMemoryBarrier() MemoryBarrier()
#else
# define udMemoryBarrier() __sync_synchronize()
#endif

#if defined(__GNUC__)
# define udUnusedVar(x) __attribute__((__unused__))x
#elif defined(_WIN32)
# define udUnusedVar(x) (void)x
#else 
# define udUnusedVar(x) x
#endif

#if defined(__GNUC__)
# define udUnusedParam(x) __attribute__((__unused__))x
#elif defined(_WIN32)
# define udUnusedParam(x) 
#else 
# define udUnusedParam(x) 
#endif


#if defined(_MSC_VER)
# define __FUNC_NAME__ __FUNCTION__
#elif defined(__GNUC__)
# define __FUNC_NAME__ __PRETTY_FUNCTION__
#else
#pragma message ("This platform hasn't setup up __FUNC_NAME__")
# define __FUNC_NAME__ "unknown"
#endif


// Disabled Warnings
#if defined(_MSC_VER)
#pragma warning(disable:4127) // conditional expression is constant
#endif //defined(_MSC_VER)



#if UDPLATFORM_WINDOWS
# define udU64L(x) x##ULL
# define udI64L(x) x##LL
# define UDFORCE_INLINE __forceinline
#else
# define udU64L(x) x##UL
# define udI64L(x) x##L
# define UDFORCE_INLINE inline
#endif


#include "udDebug.h"

#endif // UDPLATFORM_H
