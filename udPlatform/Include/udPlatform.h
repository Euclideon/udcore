#ifndef UDPLATFORM_H
#define UDPLATFORM_H


// An abstraction layer for common functions that differ on various platforms
#include <stdint.h>

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
# define UDPLATFORM_WINDOWS
#elif defined(__linux__) // TODO: Work out best tag to detect linux here
# define UDPLATFORM_LINUX
#else
# error "Unknown platform"
#endif

#if defined(_DEBUG)
# define UD_DEBUG   (1)
# define UD_RELEASE (0)
#else
# define UD_DEBUG   (0)
# define UD_RELEASE (1)
#endif


#ifdef UDPLATFORM_WINDOWS
# define udInterlockedIncrement(x) InterlockedIncrement(x)
# define udInterlockedDecrement(x) InterlockedDecrement(x)
# define udSleep(x) Sleep(x)

#elif defined(UDPLATFORM_LINUX)
# define udInterlockedIncrement(x) ++(*x) // TODO
# define udInterlockedDecrement(x) --(*x) // TODO
# define udSleep(x) sleep(x)

#else
# define udInterlockedIncrement(x) ++(*x)
# define udInterlockedDecrement(x) --(*x)
# define udSleep(x)

#endif

// TODO: Consider wrapping instead of implementing psuedo-c++11 interfaces
// Using c++11 ATOMIC library, so for MSVC versions not supporting this provide a minimal implementation
#if (defined(_MSC_VER) && (_MSC_VER <= 1600)) || defined(__MINGW32__)// Visual studio 2010 (VC110) and below
// Define a subset of std::atomic specifically to meet exactly the needs of udRender's use
#include <windows.h>

namespace std
{
  class atomic_long
  {
  public:
    operator long() { return member; }
    long operator--() { return InterlockedDecrement(&member); }
    long operator--(int) { return InterlockedDecrement(&member) + 1; }
    long operator++() { return InterlockedIncrement(&member); }
    long operator++(int) { return InterlockedIncrement(&member) - 1; }
    void operator=(long v) { member = v; }
  protected:
    volatile long member;
  };

  template <typename T>
  class atomic
  {
  public:
    operator T() { return member; }
    void operator =(const T &value) { member = value; } // TODO: Check there's no better Interlocked way to do this
    bool compare_exchange_weak(T &expected, T desired) { T actual = InterlockedCompareExchangePointer((void*volatile*)&member, desired, expected); if (actual == expected) return true; expected = actual; return false; }
  protected:
    T member;
  };


  class mutex
  {
  public:
    mutex() { handle = CreateMutex(NULL, FALSE, NULL); }
    ~mutex() { CloseHandle(handle); }
    void lock() { WaitForSingleObject(handle, INFINITE); }
    bool try_lock() { return WaitForSingleObject(handle, 0) == WAIT_OBJECT_0; }
    void unlock() { ReleaseMutex(handle); }
  protected:
    HANDLE handle;
  };
};

#else

#include <thread>
#include <atomic>
#include <mutex>

#endif

// Minimalist MOST BASIC cross-platform thread support
typedef uint32_t (udThreadStart)(void *data);
uint64_t udCreateThread(udThreadStart *threadStarter, void *threadData); // Returns thread handle
void udDestroyThread(uint64_t threadHandle);

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
#  define udAllocAligned(size, alignment) _udAllocAligned(size, alignment, __FILE__, __LINE__)

#  define udRealloc(pMemory, size) _udRealloc(pMemory, size, __FILE__, __LINE__)
#  define udReallocAligned(size, alignment) _udReallocAligned(size, alignment, __FILE__, __LINE__)

#  define udFree(pMemory) _udFree((void**)&pMemory, __FILE__, __LINE__)

#  define udNew new (UDMO_Memory, __FILE__, __LINE__)
#  define udNewArray new [] (UDMO_Memory, __FILE__, __LINE__)

#  define udDelete(pMemory) _udDelete(pMemory, UDMO_Memory, __FILE__, __LINE__)
#  define udDeleteArray(pMemory) _udDeleteArray(pMemory, UDMO_Memory, __FILE__, __LINE__)

#else //  __MEMORY_DEBUG__
#  define udAlloc(size) _udAlloc(size)
#  define udAllocAligned(size, alignment) _udAllocAligned(size, alignment)

#  define udRealloc(pMemory, size) _udRealloc(pMemory, size)
#  define udReallocAligned(size, alignment) _udReallocAligned(size, alignment)

#  define udFree(pMemory) _udFree((void**)&pMemory)

#  define udNew new (UDMO_Memory)
#  define udNewArray new [] (UDMO_Memory)

#  define udDelete(pMemory) _udDelete(pMemory, UDMO_Memory)
#  define udDeleteArray(pMemory) _udDeleteArray(pMemory, UDMO_Memory)

#endif  //  __MEMORY_DEBUG__

#if __MEMORY_DEBUG__
void udMemoryOutputLeaks();
#else
# define udMemoryOutputLeaks()
#endif // __MEMORY_DEBUG__

#ifdef UDPLATFORM_WINDOWS
# define udMemoryBarrier() MemoryBarrier()
#else
# define udMemoryBarrier() __sync_synchronize()
#endif

#if defined(__GNUC__)
# define udUnused(x) x __attribute__((unused))
#elif defined(_WIN32)
# define udUnused(x) (void)x
#else 
# define udUnused(x) x
#endif

#if defined(_WIN32)
# define __FUNC_NAME__ __FUNCTION__
#elif defined(__GNUC__)
# define __FUNC_NAME__ __PRETTY_FUNCTION__
#else
#pragma message ("This platform hasn't setup up __FUNC_NAME__")
# define __FUNC_NAME__ "unknown"
#endif



#include "udDebug.h"

#endif // UDPLATFORM_H
