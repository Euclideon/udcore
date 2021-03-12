#ifndef UDRESULT_H
#define UDRESULT_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, February 2014
//
// udResult enum and associated helpers
//

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

extern bool g_udBreakOnError;      // Set to true normally, unset and reset around sections of code (eg tests) that aren't unexpected
extern const char *g_udLastErrorFilename;
extern int g_udLastErrorLine;
extern uint32_t g_udLastErrorHelpCode; // Help code for output to user to track back to line that generated error

#if !defined(UD_ERROR_BREAK_ON_ERROR)
# define UD_ERROR_BREAK_ON_ERROR 0  // Set to 1 to have the debugger break on error
#endif
#if defined(_DEBUG)
# define UD_ERROR_SETLAST(f,l) g_udLastErrorFilename = f; g_udLastErrorLine = l
#else
# define UD_ERROR_SETLAST(f,l) g_udLastErrorLine = l
#endif
constexpr unsigned udResultCalcHelpCode(const char filename[], const int line, uint32_t code = 1, int seed = 4)
{
  return !filename[0] ? (code << 11) | (line & ((1 << 11) - 1))
    : (filename[0] == '/' || filename[0] == '\\')
    ? udResultCalcHelpCode(filename + 1, line, 1, seed)
    : udResultCalcHelpCode(filename + 1, line, (uint32_t)(uint64_t(code) + (filename[0] + seed)), seed);
}

// Some helper macros that assume an exit path label "epilogue" and a local variable "result"

#define UD_ERROR_IF(cond, code)     do { if (cond)           { result = code; if (result) { UD_ERROR_SETLAST(__FILE__,__LINE__); constexpr unsigned _c = udResultCalcHelpCode(__FILE__,__LINE__); g_udLastErrorHelpCode = _c; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_NULL(ptr, code)    do { if (ptr == nullptr) { result = code; if (result) { UD_ERROR_SETLAST(__FILE__,__LINE__); constexpr unsigned _c = udResultCalcHelpCode(__FILE__,__LINE__); g_udLastErrorHelpCode = _c; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue; } } while(0)
#define UD_ERROR_SET(code)          do { result = code;                       if (result) { UD_ERROR_SETLAST(__FILE__,__LINE__); constexpr unsigned _c = udResultCalcHelpCode(__FILE__,__LINE__); g_udLastErrorHelpCode = _c; if (g_udBreakOnError && UD_ERROR_BREAK_ON_ERROR) { __debugbreak(); } } goto epilogue;   } while(0)
#define UD_ERROR_HANDLE()           do {                    if (result) goto epilogue; } while(0)
#define UD_ERROR_CHECK(funcCall)    do { result = funcCall; if (result) goto epilogue; } while(0)
#define UD_ERROR_SET_NO_BREAK(code) do { result = code;                 goto epilogue; } while(0)

#define UD_RESULT_VERSION 2103121440U

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
  udR_BufferTooSmall,
  udR_FormatVariationNotSupported,
  udR_ObjectTypeMismatch,
  udR_CorruptData,
  udR_InputExhausted,
  udR_OutputExhausted,
  udR_CompressionError,
  udR_Unsupported,
  udR_Timeout,
  udR_AlignmentRequirement,
  udR_DecryptionKeyRequired,
  udR_DecryptionKeyMismatch,
  udR_SignatureMismatch,
  udR_ObjectExpired,
  udR_ParseError,
  udR_InternalCryptoError,
  udR_OutOfOrder,
  udR_OutOfRange,
  udR_CalledMoreThanOnce,
  udR_ImageLoadFailure,
  udR_StreamerNotInitialised,

  udR_OpenFailure,
  udR_CloseFailure,
  udR_ReadFailure,
  udR_WriteFailure,
  udR_SocketError,

  udR_DatabaseError,
  udR_ServerError,
  udR_AuthError,
  udR_NotAllowed,
  udR_InvalidLicense,
  udR_Pending,
  udR_Cancelled,
  udR_OutOfSync,
  udR_SessionExpired,

  udR_ProxyError,
  udR_ProxyAuthRequired,
  udR_ExceededAllowedLimit,

  udR_RateLimited,
  udR_PremiumOnly,

  udR_Count
};

// Return a human-friendly string for a given result code
const char *udResultAsString(udResult result);

#endif // UDRESULT_H
