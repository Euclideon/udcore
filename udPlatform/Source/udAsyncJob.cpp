#include "udAsyncJob.h"
#include "udThread.h"

#define RESULT_SENTINAL ((udResult)-1) // A sentinal value used to determine when valid result has been written

struct udAsyncJob
{
  udSemaphore *pSemaphore;
  volatile udResult returnResult;
};

// ****************************************************************************
// Author: Dave Pevreal, March 2018
udResult udAsyncJob_Create(udAsyncJob **ppJobHandle)
{
  udResult result;
  udAsyncJob *pJob = nullptr;

  pJob = udAllocType(udAsyncJob, 1, udAF_Zero);
  UD_ERROR_NULL(pJob, udR_MemoryAllocationFailure);
  pJob->pSemaphore = udCreateSemaphore();
  UD_ERROR_NULL(pJob->pSemaphore, udR_MemoryAllocationFailure);
  pJob->returnResult = RESULT_SENTINAL;
  *ppJobHandle = pJob;
  pJob = nullptr;
  result = udR_Success;

epilogue:
  if (pJob)
    udAsyncJob_Destroy(&pJob);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
void udAsyncJob_SetResult(udAsyncJob *pJobHandle, udResult returnResult)
{
  if (pJobHandle)
  {
    pJobHandle->returnResult = returnResult;
    udIncrementSemaphore(pJobHandle->pSemaphore);
  }
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
udResult udAsyncJob_GetResult(udAsyncJob *pJobHandle)
{
  if (pJobHandle)
  {
    udWaitSemaphore(pJobHandle->pSemaphore);
    udResult result = pJobHandle->returnResult;
    pJobHandle->returnResult = RESULT_SENTINAL;
    return result;
  }
  return udR_InvalidParameter_;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
bool udAsyncJob_GetResultTimeout(udAsyncJob *pJobHandle, udResult *pResult, int timeoutMs)
{
  if (pJobHandle)
  {
    udWaitSemaphore(pJobHandle->pSemaphore, timeoutMs);
    if (pJobHandle->returnResult != RESULT_SENTINAL)
    {
      *pResult = pJobHandle->returnResult;
      pJobHandle->returnResult = RESULT_SENTINAL;
      return true;
    }
  }
  return false;
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
void udAsyncJob_Destroy(udAsyncJob **ppJobHandle)
{
  if (ppJobHandle && *ppJobHandle)
  {
    udDestroySemaphore(&(*ppJobHandle)->pSemaphore);
    udFree(*ppJobHandle);
  }
}
