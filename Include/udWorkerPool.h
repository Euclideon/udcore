#ifndef UDWORKERPOOL_H
#define UDWORKERPOOL_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Paul Fox, May 2015
//
// Handles fire and forget work by pooling and processing async
//

#include "udResult.h"

// Function definition for async and marshalled work
typedef void (udWorkerPoolCallback)(void *pUserData);
struct udWorkerPool;

udResult udWorkerPool_Create(udWorkerPool **ppPool, uint8_t totalThreads, const char *pThreadNamePrefix = "udWorkerPool");
void udWorkerPool_Destroy(udWorkerPool **ppPool);

// Adds a function to run on a background thread, optionally with userdata. If clearMemory is true, it will call udFree on pUserData after running
udResult udWorkerPool_AddTask(udWorkerPool *pPool, udWorkerPoolCallback *pFunc, void *pUserData = nullptr, bool clearMemory = true, udWorkerPoolCallback *pPostFunction = nullptr);

// This must be run on the main thread, handles marshalling work back from worker threads if required
// The parameter can be used to limit how much work is done each time this is called
// Returns udR_NothingToDo if no work was done- otherwise udR_Success
udResult udWorkerPool_DoPostWork(udWorkerPool *pPool, int processLimit = 0);

// Returns true if there are workers currently processing tasks or if workers should be processing tasks
bool udWorkerPool_HasActiveWorkers(udWorkerPool *pPool);

#endif // udWorkerPool_h__
