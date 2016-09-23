#ifndef UDRESULT_H
#define UDRESULT_H

enum udResult
{
  udR_Success,
  udR_Failure_,
  udR_NothingToDo,
  udR_InternalError,
  udR_NotInitialized_,
  udR_InvalidConfiguration,
  udR_InvalidParameter_,
  udR_OutstandingReferences,
  udR_MemoryAllocationFailure,
  udR_CountExceeded,
  udR_ObjectNotFound,
  udR_ParentNotFound,
  udR_RenderAlreadyInProgress,
  udR_BufferTooSmall,
  udR_VersionMismatch,
  udR_ObjectTypeMismatch,
  udR_NodeLimitExceeded,
  udR_BlockLimitExceeded,
  udR_CorruptData,
  udR_CompressionInputExhausted,
  udR_CompressionOutputExhausted,
  udR_CompressionError,
  udR_Unsupported,
  udR_Timeout,
  udR_AlignmentRequirement,
  udR_DecryptionKeyRequired,
  udR_DecryptionKeyMismatch,

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
  udR_File_SeekFailure,
  udR_File_ReadFailure,
  udR_File_WriteFailure,
  udR_File_FailedToAddFileJob,
  udR_File_SocketError,

  udR_UDI_MaxOctreeCountExceeded,
  udR_UDI_MaxGridTreeDepthExceeded,
  udR_EventNotHandled,

  udR_Count,


  udR_ForceInt = 0x7FFFFFFF
};

// Return a human-friendly string fro a given result code
const char *udResultAsString(udResult result);

// Some helper macros that assume an exit path label "epilogue" and a local variable "result"

#define UD_ERROR_BREAK_ON_ERROR 0  // Set to 1 to have the debugger break on error

#define UD_ERROR_IF(x, code)      do { if (x) { result = code; if (UD_ERROR_BREAK_ON_ERROR && code) { __debugbreak(); } goto epilogue; }                  } while(0)
#define UD_ERROR_NULL(ptr, code)  do { if (ptr == nullptr) { result = code; if (UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } goto epilogue; }             } while(0)
#define UD_ERROR_CHECK(funcCall)  do { result = funcCall; if (result != udR_Success) { if (UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } goto epilogue; }  } while(0)
#define UD_ERROR_HANDLE()         do { if (result != udR_Success) { if (UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } goto epilogue; }                     } while(0)
#define UD_ERROR_SET(code)        do { result = code; if (UD_ERROR_BREAK_ON_ERROR) __debugbreak(); goto epilogue;                                         } while(0)


#endif // UDRESULT_H
