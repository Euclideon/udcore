#ifndef UDPLATFORM_H
#define UDPLATFORM_H


// An abstraction layer for common functions that differ on various platforms
#include <stdint.h>

#if _WIN64 || __amd64__
  //64-bit code
# define UD_64BIT
# define UD_WORD_SHIFT  6   // 6 bits for 64 bit pointer
# define UD_WORD_BITS   64
# define UD_WORD_BYTES  8
# define UD_WORD_MAX    0x7fffffffffffffffLL
  typedef signed long long udIWord;
  typedef unsigned long long udUWord;
#elif  defined(_WIN32)
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
#elif defined(__GNUC__) // TODO: Work out best tag to detect linux here
# define UDPLATFORM_LINUX
#else
# error "Unknown platform"
#endif

#if defined(_DEBUG)
# define UD_DEBUG
#else
# define UD_RELEASE
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
    bool compare_exchange_weak(const T &expected, T desired) { return InterlockedCompareExchangePointer((void*volatile*)&member, desired, expected) == expected; }
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


#include "udDebug.h"

#endif // UDPLATFORM_H
