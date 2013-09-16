#include "udVec.h"

const udVec4f udVec4fConst_One             = { 1.0f, 1.0f, 1.0f, 1.0f };
const udVec4f udVec4fConst_NegOne          = { -1.f, -1.f, -1.f, -1.f };
const udVec4f udVec4fConst_One0W           = { 1.0f, 1.0f, 1.0f, 0.0f };
const udVec4f udVec4fConst_Half            = { 0.5f, 0.5f, 0.5f, 0.5f };
const udVec4f udVec4fConst_Zero1X          = { 1.0f, 0.0f, 0.0f, 0.0f };
const udVec4f udVec4fConst_Zero1Y          = { 0.0f, 1.0f, 0.0f, 0.0f };
const udVec4f udVec4fConst_Zero1Z          = { 0.0f, 0.0f, 1.0f, 0.0f };
const udVec4f udVec4fConst_Zero1W          = { 0.0f, 0.0f, 0.0f, 1.0f };

const udVec4f_I32 udVec4fConst_Infinity    = { { 0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000 } };
const udVec4f_I32 udVec4fConst_FloatMin    = { { 0x00800000, 0x00800000, 0x00800000, 0x00800000 } };
const udVec4f_I32 udVec4fConst_FloatMax    = { { 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF } };
const udVec4f_I32 udVec4fConst_MaskXYZ     = { { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 } };
const udVec4f_I32 udVec4fConst_MaskX       = { { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 } };
const udVec4f_I32 udVec4fConst_MaskY       = { { 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 } };
const udVec4f_I32 udVec4fConst_MaskZ       = { { 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 } };
const udVec4f_I32 udVec4fConst_MaskW       = { { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF } };

const udVec4f_I32 udVec4fConst_indicesAsMask[16] =
{
  { { 0x00000000, 0x00000000, 0x00000000, 0x00000000 } }, // 0000
  { { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 } }, // 0001
  { { 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 } }, // 0010
  { { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 } }, // 0011
  { { 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 } }, // 0100
  { { 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000 } }, // 0101
  { { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 } }, // 0110
  { { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 } }, // 0111
  { { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF } }, // 1000
  { { 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF } }, // 1001
  { { 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF } }, // 1010
  { { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF } }, // 1011
  { { 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF } }, // 1100
  { { 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF } }, // 1101
  { { 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF } }, // 1110
  { { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF } }, // 1111
};

