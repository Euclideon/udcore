#include "udAsyncJob.h"
#include "udThread.h"

struct udAsyncJob
{
  udSemaphore *pSemaphore;
  udResult returnResult;
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
    udIncrementSemaphore(pJobHandle->pSemaphore);
    pJobHandle->returnResult = returnResult;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, March 2018
udResult udAsyncJob_GetResult(udAsyncJob *pJobHandle)
{
  if (pJobHandle)
  {
    udWaitSemaphore(pJobHandle->pSemaphore);
    return pJobHandle->returnResult;
  }
  return udR_InvalidParameter_;
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
