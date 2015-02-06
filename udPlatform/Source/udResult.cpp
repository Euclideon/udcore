#include "udResult.h"
#include "udDebug.h"

#define RESULTINFO(x) { #x+4, x }
#define RESULTINFO_CUSTOM(x,str) { str, x }

struct udResultInfo
{
  const char *pString;
  udResult result; // Used to make sure the array is correctly filled out
};

udResultInfo udResultInfoArray[] =
{
  RESULTINFO(udR_Success),
  RESULTINFO(udR_Failure_),
  RESULTINFO(udR_NothingToDo),
  RESULTINFO(udR_InternalError),
  RESULTINFO(udR_NotInitialized_),
  RESULTINFO(udR_InvalidConfiguration),
  RESULTINFO(udR_InvalidParameter_),
  RESULTINFO(udR_OutstandingReferences),
  RESULTINFO(udR_MemoryAllocationFailure),
  RESULTINFO(udR_CountExceeded),
  RESULTINFO(udR_ObjectNotFound),
  RESULTINFO(udR_RenderAlreadyInProgress),
  RESULTINFO(udR_BufferTooSmall),
  RESULTINFO(udR_VersionMismatch),
  RESULTINFO(udR_ObjectTypeMismatch),
  RESULTINFO(udR_NodeLimitExceeded),
  RESULTINFO(udR_BlockLimitExceeded),
  RESULTINFO(udR_CorruptData),
  RESULTINFO(udR_CompressionInputExhausted),
  RESULTINFO(udR_CompressionOutputExhausted),
  RESULTINFO(udR_CompressionError),

  RESULTINFO(udR_Writer_InitFailure),
  RESULTINFO(udR_Writer_WritePointCloudHeaderFailure),
  RESULTINFO(udR_Writer_WriteBlockHeaderFailure),
  RESULTINFO(udR_Writer_WriteBlockFailure),
  RESULTINFO(udR_Writer_ReadBlockFailure),
  RESULTINFO(udR_Writer_ReadNextBlockFailure),
  RESULTINFO(udR_Writer_WriteBlockInfoArrayFailure),
  RESULTINFO(udR_Writer_WriteStringTableFailure),
  RESULTINFO(udR_Writer_WriteResourceFailure),
  RESULTINFO(udR_Writer_DeinitFailure),

  RESULTINFO(udR_BlockInfoArrayExhausted),

  RESULTINFO(udR_File_OpenFailure),
  RESULTINFO(udR_File_CloseFailure),
  RESULTINFO(udR_File_NoFileJobsAvailable),
  RESULTINFO(udR_File_SeekFailure),
  RESULTINFO(udR_File_ReadFailure),
  RESULTINFO(udR_File_WriteFailure),
  RESULTINFO(udR_File_FailedToAddFileJob),
  RESULTINFO(udR_File_SocketError),
};

UDCOMPILEASSERT(sizeof(udResultInfoArray) == (udR_Count*sizeof(udResultInfoArray[0])), _ResultCodeNotEnteredInStringsTable_);


// ****************************************************************************
// Author: Dave Pevreal, March 2014
const char *udResultAsString(udResult result)
{
  if (result < 0  || result > udR_Count)
    return "Unknown error";
  if (result == udR_Count)
  {
    // Count is a special case to test the result string array
    for (int i = 0; i < udR_Count; ++i)
    {
      if (udResultInfoArray[i].result != i)
        return 0; // Signal that the array is out of sync
    }
    return ""; // Empty string signals array looks ok
  }
  return udResultInfoArray[result].pString;
}
