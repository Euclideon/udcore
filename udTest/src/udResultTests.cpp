#include "gtest/gtest.h"

#include "udResult.h"
#include "udStringUtil.h"

#define CHECK_STR(x)                                        \
  case x:                                                   \
    EXPECT_TRUE(udStrEqual(udResultAsString(x), &(#x)[4])); \
    break;

TEST(udResult, StringNamesMatch)
{
  EXPECT_EQ((int)udR_Success, 0);

  for (int i = udR_Success; i < udR_Count; ++i)
  {
    udResult code = (udResult)i;

    switch (code)
    {
      CHECK_STR(udR_Success);
      CHECK_STR(udR_Failure);
      CHECK_STR(udR_NothingToDo);
      CHECK_STR(udR_InternalError);
      CHECK_STR(udR_NotInitialized);
      CHECK_STR(udR_InvalidConfiguration);
      CHECK_STR(udR_InvalidParameter);
      CHECK_STR(udR_OutstandingReferences);
      CHECK_STR(udR_MemoryAllocationFailure);
      CHECK_STR(udR_CountExceeded);
      CHECK_STR(udR_NotFound);
      CHECK_STR(udR_BufferTooSmall);
      CHECK_STR(udR_FormatVariationNotSupported);
      CHECK_STR(udR_ObjectTypeMismatch);
      CHECK_STR(udR_CorruptData);
      CHECK_STR(udR_InputExhausted);
      CHECK_STR(udR_OutputExhausted);
      CHECK_STR(udR_CompressionError);
      CHECK_STR(udR_Unsupported);
      CHECK_STR(udR_Timeout);
      CHECK_STR(udR_AlignmentRequired);
      CHECK_STR(udR_DecryptionKeyRequired);
      CHECK_STR(udR_DecryptionKeyMismatch);
      CHECK_STR(udR_SignatureMismatch);
      CHECK_STR(udR_ObjectExpired);
      CHECK_STR(udR_ParseError);
      CHECK_STR(udR_InternalCryptoError);
      CHECK_STR(udR_OutOfOrder);
      CHECK_STR(udR_OutOfRange);
      CHECK_STR(udR_CalledMoreThanOnce);
      CHECK_STR(udR_ImageLoadFailure);
      CHECK_STR(udR_StreamerNotInitialised);
      CHECK_STR(udR_OpenFailure);
      CHECK_STR(udR_CloseFailure);
      CHECK_STR(udR_ReadFailure);
      CHECK_STR(udR_WriteFailure);
      CHECK_STR(udR_SocketError);
      CHECK_STR(udR_DatabaseError);
      CHECK_STR(udR_ServerError);
      CHECK_STR(udR_AuthError);
      CHECK_STR(udR_NotAllowed);
      CHECK_STR(udR_InvalidLicense);
      CHECK_STR(udR_Pending);
      CHECK_STR(udR_Cancelled);
      CHECK_STR(udR_OutOfSync);
      CHECK_STR(udR_SessionExpired);
      CHECK_STR(udR_ProxyError);
      CHECK_STR(udR_ProxyAuthRequired);
      CHECK_STR(udR_ExceededAllowedLimit);
      CHECK_STR(udR_RateLimited);
      CHECK_STR(udR_PremiumOnly);
      CHECK_STR(udR_InProgress);
      CHECK_STR(udR_Count);
    }
  }
}
