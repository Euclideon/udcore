#ifndef UDASYNCJOB_H
#define UDASYNCJOB_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2018
//
// This module provides a simple and convenient way to have a function-call execute on another thread,
// with the caller able to either poll or wait for completion at a later time
//

#include "udPlatformUtil.h"
#include "udResult.h"
#include "udThread.h"

// A simple interface to allow function calls to be easily made optionally background calls with one additional parameter

struct udAsyncJob;

// A helper for pausing ASync jobs, ensure all fields are zero to be initialised into non-paused state
struct udAsyncPause
{
  udSemaphore *volatile pSema; // Created locked by initiator of pause, released and destroyed by handler of pause when incremented to release pause
  udResult errorCausingPause;  // If an error condition (eg disk full) initiated a pause, the error code is here
  enum Context
  {
    EC_None,
    EC_WritingTemporaryFile,
    EC_WritingOutputFile,
  } errorContext;
  bool isPaused;
  bool errorsCanInitiatePause; // Set to true if the caller will handle user interaction when paused due to recoverable error
};

// Create an async job handle
udResult udAsyncJob_Create(udAsyncJob **ppJobHandle);

// Get the result of an async job (wait on semaphore)
udResult udAsyncJob_GetResult(udAsyncJob *pJobHandle);

// Get the result of an async job (wait on semaphore for specified time)
// Returns true if job is complete (*pResult is set), false if timed out
bool udAsyncJob_GetResultTimeout(udAsyncJob *pJobHandle, udResult *pResult, int timeoutMs);

// Set the result (increment semaphore)
void udAsyncJob_SetResult(udAsyncJob *pJobHandle, udResult returnResult);

// Set the pending flag (called internally by UDASYNC_CALLx macros)
void udAsyncJob_SetPending(udAsyncJob *pJobHandle);

// Get the pending flag (used to determine if an async call is in flight)
bool udAsyncJob_IsPending(udAsyncJob *pJobHandle);

// Destroy the async job (destroy semaphore)
void udAsyncJob_Destroy(udAsyncJob **ppJobHandle);

// Initiate a pause
void udAsyncPause_RequestPause(udAsyncPause *pPause);

// Resume a process currently paused
void udAsyncPause_Resume(udAsyncPause *pPause);

// (Called on worker thread). If paused, blocks until Resume is called, and destroys
void udAsyncPause_HandlePause(udAsyncPause *pPause);

// Return a human-readable (English) string for a given error context
const char *udAsyncPause_GetErrorContextString(udAsyncPause::Context errorContext);

// Some helper macros for boiler-plate code generation, each macro corresponds to number of parameters before pAsyncJob
// For these macros to work, udAsyncJob *pAsyncJob must be the LAST PARAMETER of the function

#define UDASYNC_CALL(funcCall)                                                          \
  if (pAsyncJob)                                                                        \
  {                                                                                     \
    udThreadStart udajStartFunc = [=](void *) -> unsigned int                           \
    {                                                                                   \
      udAsyncJob_SetResult(pAsyncJob, funcCall);                                        \
      return 0;                                                                         \
    };                                                                                  \
    udAsyncJob_SetPending(pAsyncJob);                                                   \
    return udThread_Create(nullptr, udajStartFunc, nullptr, udTCF_None, __FUNC_NAME__); \
  }

#endif // UDASYNCJOB_H
