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
  udR_SignatureMismatch,
  udR_Expired,
  udR_ParseError,
  udR_InternalCryptoError,

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

// Return a human-friendly string for a given result code
const char *udResultAsString(udResult result);

// Some helper macros that assume an exit path label "epilogue" and a local variable "result"

#define UD_ERROR_BREAK_ON_ERROR 0  // Set to 1 to have the debugger break on error
extern bool g_udBreakOnError;      // Set to true normally, unset and reset around sections of code (eg tests) that aren't unexpected

#define UD_ERROR_IF(cond, code)   do { if (cond)                      { result = code; if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (UD_ERROR_BREAK_ON_ERROR && g_udBreakOnError) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_NULL(ptr, code)  do { if (ptr == nullptr)            { result = code; if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (UD_ERROR_BREAK_ON_ERROR && g_udBreakOnError) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_CHECK(funcCall)  do { result = funcCall;                              if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (UD_ERROR_BREAK_ON_ERROR && g_udBreakOnError) { __debugbreak(); }   goto epilogue; } } while(0)
#define UD_ERROR_HANDLE()         do {                                                 if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (UD_ERROR_BREAK_ON_ERROR && g_udBreakOnError) { __debugbreak(); }   goto epilogue; } } while(0)
#define UD_ERROR_SET(code)        do { result = code;                                  if (result) { g_udLastErrorFilename = __FILE__; g_udLastErrorLine = __LINE__; if (UD_ERROR_BREAK_ON_ERROR && g_udBreakOnError) { __debugbreak(); } } goto epilogue;   } while(0)

extern const char *g_udLastErrorFilename;
extern int g_udLastErrorLine;

#endif // UDRESULT_H
