#ifndef UDTHREAD_H
#define UDTHREAD_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, November 2016
//
// Very simple thread/synchronisation API wrapping the various platform implementations
//

#include "udPlatform.h"
#include "udCallback.h"

// Minimalist MOST BASIC cross-platform thread support
struct udSemaphore;
struct udConditionVariable;
struct udRWLock;
struct udMutex;
struct udThread;
enum udThreadPriority { udTP_Lowest, udTP_Low, udTP_Normal, udTP_High, udTP_Highest };
enum udThreadCreateFlags { udTCF_None }; // For future expansion
#define UDTHREAD_WAIT_INFINITE -1

using udThreadStart = udCallback<uint32_t(void *)>;

// Create a thread object
udResult udThread_Create(udThread **ppThread, udThreadStart threadStarter, void *pThreadData, udThreadCreateFlags flags = udTCF_None, const char *pThreadName = nullptr);

// Set the thread priority
void udThread_SetPriority(udThread *pThread, udThreadPriority priority);

// Destroy a thread, this should be called after the thread has exited (udThread_Join can be used to assist)
void udThread_Destroy(udThread **ppThreadHandle);

// Destroy cached threads
void udThread_DestroyCached();

// Wait for a thread to complete
udResult udThread_Join(udThread *pThread, int waitMs = UDTHREAD_WAIT_INFINITE);

udSemaphore *udCreateSemaphore();
void udDestroySemaphore(udSemaphore **ppSemaphore);
void udIncrementSemaphore(udSemaphore *pSemaphore, int count = 1);
int udWaitSemaphore(udSemaphore *pSemaphore, int waitMs = UDTHREAD_WAIT_INFINITE); // Returns zero on success

udConditionVariable *udCreateConditionVariable();
void udDestroyConditionVariable(udConditionVariable **ppConditionVariable);
void udSignalConditionVariable(udConditionVariable *pConditionVariable, int count = 1);
int udWaitConditionVariable(udConditionVariable *pConditionVariable, udMutex *pMutex, int waitMs = UDTHREAD_WAIT_INFINITE); // Returns zero on success

udRWLock *udCreateRWLock();
void udDestroyRWLock(udRWLock **ppRWLock);
int udReadLockRWLock(udRWLock *pRWLock); // Returns zero on success
int udWriteLockRWLock(udRWLock *pRWLock); // Returns zero on success
void udReadUnlockRWLock(udRWLock *pRWLock);
void udWriteUnlockRWLock(udRWLock *pRWLock);

udMutex *udCreateMutex();
void udDestroyMutex(udMutex **ppMutex);
udMutex *udLockMutex(udMutex *pMutex); // Returns pMutex on success
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

#endif // UDTHREAD_H
