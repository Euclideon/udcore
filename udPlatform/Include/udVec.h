#ifndef UDVEC_H
#define UDVEC_H


#include <xmmintrin.h>
#include <immintrin.h> // For the switch to doubles

#if defined(_WIN32)
#define UD_ALIGN_16_VS __declspec(align(16))
#define UD_ALIGN_16_GCC
#else
#define UD_ALIGN_16_VS 
#define UD_ALIGN_16_GCC __attribute__ ((aligned (16)))
#endif

typedef __m128 udVec4f;
typedef __m128i udVec4i;

// A declaration making it easier to statically declare a udVec4f as 4 integers
UD_ALIGN_16_VS struct udVec4f_I32
{
  unsigned i32[4];
  inline operator udVec4f() const { return *(udVec4f*)i32; }
} UD_ALIGN_16_GCC;

extern const udVec4f udVec4fConst_One;
extern const udVec4f udVec4fConst_NegOne;
extern const udVec4f udVec4fConst_One0W;
extern const udVec4f udVec4fConst_Half;
extern const udVec4f udVec4fConst_Zero1X;
extern const udVec4f udVec4fConst_Zero1Y;
extern const udVec4f udVec4fConst_Zero1Z;
extern const udVec4f udVec4fConst_Zero1W;
extern const udVec4f_I32 udVec4fConst_Infinity;
extern const udVec4f_I32 udVec4fConst_FloatMin;
extern const udVec4f_I32 udVec4fConst_FloatMax; // TODO: udVec4fConst_FloatNegMax
extern const udVec4f_I32 udVec4fConst_MaskXYZ;
extern const udVec4f_I32 udVec4fConst_MaskX;
extern const udVec4f_I32 udVec4fConst_MaskY;
extern const udVec4f_I32 udVec4fConst_MaskZ;
extern const udVec4f_I32 udVec4fConst_MaskW;
extern const udVec4f_I32 udVec4fConst_indicesAsMask[16];


inline udVec4f udVec4fSet(float x, float y, float z, float w) { return _mm_set_ps(w, z, y, x); }

#define udVecZero() _mm_setzero_ps()
#define udVecOne() udVec4fConst_One
inline udVec4f udVecSplatX(udVec4f v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)); }
inline udVec4f udVecSplatY(udVec4f v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)); }
inline udVec4f udVecSplatZ(udVec4f v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)); }
inline udVec4f udVecSplatW(udVec4f v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)); }
inline udVec4f udVecOr(udVec4f a, udVec4f b)  { return _mm_or_ps(a, b); }
inline udVec4f udVecAnd(udVec4f a, udVec4f b) { return _mm_and_ps(a, b); }
inline udVec4f udVecAndNot(udVec4f a, udVec4f b) { return _mm_andnot_ps(a, b); } // ~a & b
inline udVec4f udVecNeg(udVec4f v) { return _mm_sub_ps( _mm_setzero_ps(), v); }
inline udVec4f udVecAbs(udVec4f v) { udVec4f r = _mm_setzero_ps(); r = _mm_sub_ps(r, v); r = _mm_max_ps(r, v); return r; }
inline udVec4f udVecMax(udVec4f a, udVec4f b) { return _mm_max_ps(a, b); }
inline udVec4f udVecMin(udVec4f a, udVec4f b) { return _mm_min_ps(a, b); }
inline udVec4f udVecAdd(udVec4f a, udVec4f b) { return _mm_add_ps(a, b); }
inline udVec4f udVecSub(udVec4f a, udVec4f b) { return _mm_sub_ps(a, b); }
inline udVec4f udVecMul(udVec4f a, udVec4f b) { return _mm_mul_ps(a, b); }
inline udVec4f udVecDiv(udVec4f a, udVec4f b) { return _mm_div_ps(a, b); }
inline udVec4f udVecMulAdd(udVec4f a, udVec4f b, udVec4f c) { return _mm_add_ps(_mm_mul_ps(a, b), c); }
inline udVec4f udVecReciprocal(udVec4f v) { return _mm_div_ps(udVec4fConst_One, v); }
inline udVec4f udVecReciprocalEst(udVec4f v) { return _mm_rcp_ps(v); }

// Comparisons
inline int udVecGetSignsMask(udVec4f v)              { return _mm_movemask_ps(v); }
inline int udVecLess(udVec4f a, udVec4f b)           { return ((_mm_movemask_ps(_mm_cmplt_ps(a,b))==0x0f) != 0); }
inline int udVecLessOrEqual(udVec4f a, udVec4f b)    { return ((_mm_movemask_ps(_mm_cmple_ps(a,b))==0x0f) != 0); }
inline int udVecGreater(udVec4f a, udVec4f b)        { return ((_mm_movemask_ps(_mm_cmpgt_ps(a,b))==0x0f) != 0); }
inline int udVecGreaterOrEqual(udVec4f a, udVec4f b) { return ((_mm_movemask_ps(_mm_cmpge_ps(a,b))==0x0f) != 0); }
inline int udVecEqual(udVec4f a, udVec4f b)          { return ((_mm_movemask_ps(_mm_cmpeq_ps(a,b))==0x0f) != 0); }
inline int udVecIsNaN(udVec4f v)                     { return ((_mm_movemask_ps(_mm_cmpneq_ps(v,v))     ) != 0); }
inline int udVecIsInfinite(udVec4f v)                { return ((_mm_movemask_ps(_mm_cmpeq_ps(v,udVec4fConst_Infinity))) != 0); }

inline float udVecGetX(udVec4f v) { return _mm_cvtss_f32(v); }
inline float udVecGetY(udVec4f v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1))); }
inline float udVecGetZ(udVec4f v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2))); }
inline float udVecGetW(udVec4f v) { return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3))); }

inline udVec4f udVecSetX(udVec4f v, float x) 
{ 
  return _mm_move_ss(v,_mm_set_ss(x));
}

inline udVec4f udVecSetY(udVec4f v, float y)
{
  // Swap y and x
  udVec4f vResult = _mm_shuffle_ps(v, v,_MM_SHUFFLE(3,2,0,1));
  // Convert input to vector
  udVec4f vTemp = _mm_set_ss(y);
  // Replace the x component
  vResult = _mm_move_ss(vResult,vTemp);
  // Swap y and x again
  vResult = _mm_shuffle_ps(vResult, vResult, _MM_SHUFFLE(3,2,0,1));
  return vResult;
}

inline udVec4f udVecSetZ(udVec4f v, float z)
{
  // Swap z and x
  udVec4f vResult = _mm_shuffle_ps(v, v,_MM_SHUFFLE(3,0,1,2));
  // Convert input to vector
  udVec4f vTemp = _mm_set_ss(z);
  // Replace the x component
  vResult = _mm_move_ss(vResult, vTemp);
  // Swap z and x again
  vResult = _mm_shuffle_ps(vResult, vResult, _MM_SHUFFLE(3,0,1,2));
  return vResult;
}

inline udVec4f udVecSetW(udVec4f v, float w)
{ 
  // Swap w and x
  udVec4f vResult = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,2,1,3));
  // Convert input to vector
  udVec4f vTemp = _mm_set_ss(w);
  // Replace the x component
  vResult = _mm_move_ss(vResult, vTemp);
  // Swap w and x again
  vResult = _mm_shuffle_ps(vResult, vResult, _MM_SHUFFLE(0,2,1,3));
  return vResult;
}

// TODO: These must be done properly/kept in simd if possible
inline udVec4f udVecCopyX(udVec4f a, udVec4f b) { return udVecSetX(a, udVecGetX(b)); }
inline udVec4f udVecCopyY(udVec4f a, udVec4f b) { return udVecSetY(a, udVecGetY(b)); }
inline udVec4f udVecCopyZ(udVec4f a, udVec4f b) { return udVecSetZ(a, udVecGetZ(b)); }
inline udVec4f udVecCopyW(udVec4f a, udVec4f b) { return udVecSetW(a, udVecGetW(b)); }



inline udVec4f udVecDot3(udVec4f V1, udVec4f V2)
{ 
  udVec4f vDot = _mm_mul_ps(V1,V2);
  // x=Dot.vector4_f32[1], y=Dot.vector4_f32[2]
  udVec4f vTemp = _mm_shuffle_ps(vDot,vDot,_MM_SHUFFLE(2,1,2,1));
  // Result.vector4_f32[0] = x+y
  vDot = _mm_add_ss(vDot,vTemp);
  // x=Dot.vector4_f32[2]
  vTemp = _mm_shuffle_ps(vTemp,vTemp,_MM_SHUFFLE(1,1,1,1));
  // Result.vector4_f32[0] = (x+y)+z
  vDot = _mm_add_ss(vDot,vTemp);
  // Splat x
  return _mm_shuffle_ps(vDot,vDot,_MM_SHUFFLE(0,0,0,0));
}

inline udVec4f udVecDot4(udVec4f V1, udVec4f V2)
{ 
  udVec4f vTemp2 = V2;
  udVec4f vTemp = _mm_mul_ps(V1,vTemp2);
  vTemp2 = _mm_shuffle_ps(vTemp2,vTemp,_MM_SHUFFLE(1,0,0,0)); // Copy X to the Z position and Y to the W position
  vTemp2 = _mm_add_ps(vTemp2,vTemp);          // Add Z = X+Z; W = Y+W;
  vTemp = _mm_shuffle_ps(vTemp,vTemp2,_MM_SHUFFLE(0,3,0,0));  // Copy W to the Z position
  vTemp = _mm_add_ps(vTemp,vTemp2);           // Add Z and W together
  return _mm_shuffle_ps(vTemp,vTemp,_MM_SHUFFLE(2,2,2,2));    // Splat Z and return
}

inline udVec4f udVecPlaneNormalize(udVec4f p)
{
  // Perform the dot product on x,y and z only
  udVec4f vLengthSq = _mm_mul_ps(p, p);
  udVec4f vTemp = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(2,1,2,1));
  vLengthSq = _mm_add_ss(vLengthSq,vTemp);
  vTemp = _mm_shuffle_ps(vTemp,vTemp, _MM_SHUFFLE(1,1,1,1));
  vLengthSq = _mm_add_ss(vLengthSq, vTemp);
  vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(0,0,0,0));
  // Prepare for the division
  udVec4f vResult = _mm_sqrt_ps(vLengthSq);
  // Failsafe on zero (Or epsilon) length planes
  // If the length is infinity, set the elements to zero
  vLengthSq = _mm_cmpneq_ps(vLengthSq, udVec4fConst_Infinity);
  // Reciprocal mul to perform the normalization
  vResult = _mm_div_ps(p,vResult);
  // Any that are infinity, set to zero
  vResult = _mm_and_ps(vResult, vLengthSq);
  return vResult;
}

#endif // UDVEC_H

