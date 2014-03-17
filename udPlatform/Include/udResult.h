#ifndef UDRESULT_H
#define UDRESULT_H

enum udResult
{
  udR_Success,
  udR_Failure_,
  udR_InternalError,
  udR_NotInitialized_,
  udR_InvalidConfiguration,
  udR_InvalidParameter_,
  udR_OutstandingReferences,
  udR_MemoryAllocationFailure,
  udR_CountExceeded,
  udR_ObjectNotFound,
  udR_RenderAlreadyInProgress,
  udR_BufferTooSmall,

  udR_Writer_InitFailure, 
  udR_Writer_WritePointCloudHeaderFailure, 
  udR_Writer_WriteBlockHeaderFailure, 
  udR_Writer_WriteBlockFailure,
  udR_Writer_ReadBlockFailure,
  udR_Writer_ReadNextBlockFailure,
  udR_Writer_WriteBlockInfoArrayFailure,
  udR_Writer_WriteStringTableFailure,
  udR_Writer_WriteResourceFailure,
  udR_Writer_DeinitFailure,

  udR_BlockInfoArrayExhausted, 

  udR_File_OpenFailure,
  udR_File_CloseFailure,
  udR_File_NoFileJobsAvailable,
  udR_File_ReadFailure,
  udR_File_WriteFailure,
  udR_File_FailedToAddFileJob,
  udR_File_SocketError,
  udR_Count,

  udR_ForceInt = 0x7FFFFFFF
};


#endif // UDRESULT_H
