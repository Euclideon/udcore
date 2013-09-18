#include "udMatrix.h"
#include <math.h>

void udMatrix4f::SetIdentity()
{
  r[0] = udVec4fConst_Zero1X;
  r[1] = udVec4fConst_Zero1Y;
  r[2] = udVec4fConst_Zero1Z;
  r[3] = udVec4fConst_Zero1W;
}

void udMatrix4f::SetFromArray(const float values[16])
{
  r[0] = udVec4fSet(values[0], values[1], values[2], values[3]);
  r[1] = udVec4fSet(values[4], values[5], values[6], values[7]);
  r[2] = udVec4fSet(values[8], values[9], values[10], values[11]);
  r[3] = udVec4fSet(values[12], values[13], values[14], values[15]);
}

void udMatrix4f::SetFromArray(const double values[16])
{
  r[0] = udVec4fSet((float)values[0], (float)values[1], (float)values[2], (float)values[3]);
  r[1] = udVec4fSet((float)values[4], (float)values[5], (float)values[6], (float)values[7]);
  r[2] = udVec4fSet((float)values[8], (float)values[9], (float)values[10], (float)values[11]);
  r[3] = udVec4fSet((float)values[12], (float)values[13], (float)values[14], (float)values[15]);
}

void udMatrix4f::Multiply(const udMatrix4f &m1, const udMatrix4f &m2)
{
  udMatrix4f result;
  // Use vW to hold the original row
  udVec4f vW = m1.r[0];
  // Splat the component X,Y,Z then W
  udVec4f vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
  udVec4f vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
  udVec4f vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
  vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));

  // Row 0
  vX = _mm_mul_ps(vX,m2.r[0]);
  vY = _mm_mul_ps(vY,m2.r[1]);
  vZ = _mm_mul_ps(vZ,m2.r[2]);
  vW = _mm_mul_ps(vW,m2.r[3]);
  // Perform a binary add to reduce cumulative errors
  vX = _mm_add_ps(vX,vZ);
  vY = _mm_add_ps(vY,vW);
  vX = _mm_add_ps(vX,vY);
  result.r[0] = vX;

  // Row 1
  vW = m1.r[1];
  vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
  vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
  vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
  vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
  vX = _mm_mul_ps(vX,m2.r[0]);
  vY = _mm_mul_ps(vY,m2.r[1]);
  vZ = _mm_mul_ps(vZ,m2.r[2]);
  vW = _mm_mul_ps(vW,m2.r[3]);
  vX = _mm_add_ps(vX,vZ);
  vY = _mm_add_ps(vY,vW);
  vX = _mm_add_ps(vX,vY);
  result.r[1] = vX;
  vW = m1.r[2];
  vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
  vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
  vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
  vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
  vX = _mm_mul_ps(vX,m2.r[0]);
  vY = _mm_mul_ps(vY,m2.r[1]);
  vZ = _mm_mul_ps(vZ,m2.r[2]);
  vW = _mm_mul_ps(vW,m2.r[3]);
  vX = _mm_add_ps(vX,vZ);
  vY = _mm_add_ps(vY,vW);
  vX = _mm_add_ps(vX,vY);
  result.r[2] = vX;
  vW = m1.r[3];
  vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
  vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
  vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
  vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
  vX = _mm_mul_ps(vX,m2.r[0]);
  vY = _mm_mul_ps(vY,m2.r[1]);
  vZ = _mm_mul_ps(vZ,m2.r[2]);
  vW = _mm_mul_ps(vW,m2.r[3]);
  vX = _mm_add_ps(vX,vZ);
  vY = _mm_add_ps(vY,vW);
  vX = _mm_add_ps(vX,vY);
  result.r[3] = vX;

  // Assign result to *this
  r[0] = result.r[0];
  r[1] = result.r[1];
  r[2] = result.r[2];
  r[3] = result.r[3];
}

void udMatrix4f::Inverse(const udMatrix4f &a, udVec4f *determinant)
{
  udMatrix4f trans;
  trans.Transpose(a);
  udVec4f V00 = _mm_shuffle_ps(trans.r[2], trans.r[2],_MM_SHUFFLE(1,1,0,0));
  udVec4f V10 = _mm_shuffle_ps(trans.r[3], trans.r[3],_MM_SHUFFLE(3,2,3,2));
  udVec4f V01 = _mm_shuffle_ps(trans.r[0], trans.r[0],_MM_SHUFFLE(1,1,0,0));
  udVec4f V11 = _mm_shuffle_ps(trans.r[1], trans.r[1],_MM_SHUFFLE(3,2,3,2));
  udVec4f V02 = _mm_shuffle_ps(trans.r[2], trans.r[0],_MM_SHUFFLE(2,0,2,0));
  udVec4f V12 = _mm_shuffle_ps(trans.r[3], trans.r[1],_MM_SHUFFLE(3,1,3,1));

  udVec4f D0 = _mm_mul_ps(V00,V10);
  udVec4f D1 = _mm_mul_ps(V01,V11);
  udVec4f D2 = _mm_mul_ps(V02,V12);

  V00 = _mm_shuffle_ps(trans.r[2],trans.r[2],_MM_SHUFFLE(3,2,3,2));
  V10 = _mm_shuffle_ps(trans.r[3],trans.r[3],_MM_SHUFFLE(1,1,0,0));
  V01 = _mm_shuffle_ps(trans.r[0],trans.r[0],_MM_SHUFFLE(3,2,3,2));
  V11 = _mm_shuffle_ps(trans.r[1],trans.r[1],_MM_SHUFFLE(1,1,0,0));
  V02 = _mm_shuffle_ps(trans.r[2],trans.r[0],_MM_SHUFFLE(3,1,3,1));
  V12 = _mm_shuffle_ps(trans.r[3],trans.r[1],_MM_SHUFFLE(2,0,2,0));

  V00 = _mm_mul_ps(V00,V10);
  V01 = _mm_mul_ps(V01,V11);
  V02 = _mm_mul_ps(V02,V12);
  D0 = _mm_sub_ps(D0,V00);
  D1 = _mm_sub_ps(D1,V01);
  D2 = _mm_sub_ps(D2,V02);
  // V11 = D0Y,D0W,D2Y,D2Y
  V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,1,3,1));
  V00 = _mm_shuffle_ps(trans.r[1], trans.r[1],_MM_SHUFFLE(1,0,2,1));
  V10 = _mm_shuffle_ps(V11,D0,_MM_SHUFFLE(0,3,0,2));
  V01 = _mm_shuffle_ps(trans.r[0], trans.r[0],_MM_SHUFFLE(0,1,0,2));
  V11 = _mm_shuffle_ps(V11,D0,_MM_SHUFFLE(2,1,2,1));
  // V13 = D1Y,D1W,D2W,D2W
  udVec4f V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,3,3,1));
  V02 = _mm_shuffle_ps(trans.r[3], trans.r[3],_MM_SHUFFLE(1,0,2,1));
  V12 = _mm_shuffle_ps(V13,D1,_MM_SHUFFLE(0,3,0,2));
  udVec4f V03 = _mm_shuffle_ps(trans.r[2], trans.r[2],_MM_SHUFFLE(0,1,0,2));
  V13 = _mm_shuffle_ps(V13,D1,_MM_SHUFFLE(2,1,2,1));

  udVec4f C0 = _mm_mul_ps(V00,V10);
  udVec4f C2 = _mm_mul_ps(V01,V11);
  udVec4f C4 = _mm_mul_ps(V02,V12);
  udVec4f C6 = _mm_mul_ps(V03,V13);

  // V11 = D0X,D0Y,D2X,D2X
  V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(0,0,1,0));
  V00 = _mm_shuffle_ps(trans.r[1], trans.r[1],_MM_SHUFFLE(2,1,3,2));
  V10 = _mm_shuffle_ps(D0,V11,_MM_SHUFFLE(2,1,0,3));
  V01 = _mm_shuffle_ps(trans.r[0], trans.r[0],_MM_SHUFFLE(1,3,2,3));
  V11 = _mm_shuffle_ps(D0,V11,_MM_SHUFFLE(0,2,1,2));
  // V13 = D1X,D1Y,D2Z,D2Z
  V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(2,2,1,0));
  V02 = _mm_shuffle_ps(trans.r[3], trans.r[3],_MM_SHUFFLE(2,1,3,2));
  V12 = _mm_shuffle_ps(D1,V13,_MM_SHUFFLE(2,1,0,3));
  V03 = _mm_shuffle_ps(trans.r[2], trans.r[2],_MM_SHUFFLE(1,3,2,3));
  V13 = _mm_shuffle_ps(D1,V13,_MM_SHUFFLE(0,2,1,2));

  V00 = _mm_mul_ps(V00,V10);
  V01 = _mm_mul_ps(V01,V11);
  V02 = _mm_mul_ps(V02,V12);
  V03 = _mm_mul_ps(V03,V13);
  C0 = _mm_sub_ps(C0,V00);
  C2 = _mm_sub_ps(C2,V01);
  C4 = _mm_sub_ps(C4,V02);
  C6 = _mm_sub_ps(C6,V03);

  V00 = _mm_shuffle_ps(trans.r[1],trans.r[1],_MM_SHUFFLE(0,3,0,3));
  // V10 = D0Z,D0Z,D2X,D2Y
  V10 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,0,2,2));
  V10 = _mm_shuffle_ps(V10,V10,_MM_SHUFFLE(0,2,3,0));
  V01 = _mm_shuffle_ps(trans.r[0],trans.r[0],_MM_SHUFFLE(2,0,3,1));
  // V11 = D0X,D0W,D2X,D2Y
  V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,0,3,0));
  V11 = _mm_shuffle_ps(V11,V11,_MM_SHUFFLE(2,1,0,3));
  V02 = _mm_shuffle_ps(trans.r[3],trans.r[3],_MM_SHUFFLE(0,3,0,3));
  // V12 = D1Z,D1Z,D2Z,D2W
  V12 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,2,2,2));
  V12 = _mm_shuffle_ps(V12,V12,_MM_SHUFFLE(0,2,3,0));
  V03 = _mm_shuffle_ps(trans.r[2],trans.r[2],_MM_SHUFFLE(2,0,3,1));
  // V13 = D1X,D1W,D2Z,D2W
  V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,2,3,0));
  V13 = _mm_shuffle_ps(V13,V13,_MM_SHUFFLE(2,1,0,3));

  V00 = _mm_mul_ps(V00,V10);
  V01 = _mm_mul_ps(V01,V11);
  V02 = _mm_mul_ps(V02,V12);
  V03 = _mm_mul_ps(V03,V13);
  udVec4f C1 = _mm_sub_ps(C0,V00);
  C0 = _mm_add_ps(C0,V00);
  udVec4f C3 = _mm_add_ps(C2,V01);
  C2 = _mm_sub_ps(C2,V01);
  udVec4f C5 = _mm_sub_ps(C4,V02);
  C4 = _mm_add_ps(C4,V02);
  udVec4f C7 = _mm_add_ps(C6,V03);
  C6 = _mm_sub_ps(C6,V03);

  C0 = _mm_shuffle_ps(C0,C1,_MM_SHUFFLE(3,1,2,0));
  C2 = _mm_shuffle_ps(C2,C3,_MM_SHUFFLE(3,1,2,0));
  C4 = _mm_shuffle_ps(C4,C5,_MM_SHUFFLE(3,1,2,0));
  C6 = _mm_shuffle_ps(C6,C7,_MM_SHUFFLE(3,1,2,0));
  C0 = _mm_shuffle_ps(C0,C0,_MM_SHUFFLE(3,1,2,0));
  C2 = _mm_shuffle_ps(C2,C2,_MM_SHUFFLE(3,1,2,0));
  C4 = _mm_shuffle_ps(C4,C4,_MM_SHUFFLE(3,1,2,0));
  C6 = _mm_shuffle_ps(C6,C6,_MM_SHUFFLE(3,1,2,0));
  
  // Get the determinate if required
  udVec4f vTemp = udVecDot4(C0,trans.r[0]);

  if (determinant)
    *determinant = vTemp;

  vTemp = _mm_div_ps(udVec4fConst_One, vTemp);
  
  // Assign result to *this
  r[0] = _mm_mul_ps(C0,vTemp);
  r[1] = _mm_mul_ps(C2,vTemp);
  r[2] = _mm_mul_ps(C4,vTemp);
  r[3] = _mm_mul_ps(C6,vTemp);
}

void udMatrix4f::Transpose(const udMatrix4f &M)
{
  // x.x,x.y,y.x,y.y
  udVec4f vTemp1 = _mm_shuffle_ps(M.r[0],M.r[1],_MM_SHUFFLE(1,0,1,0));
  // x.z,x.w,y.z,y.w
  udVec4f vTemp3 = _mm_shuffle_ps(M.r[0],M.r[1],_MM_SHUFFLE(3,2,3,2));
  // z.x,z.y,w.x,w.y
  udVec4f vTemp2 = _mm_shuffle_ps(M.r[2],M.r[3],_MM_SHUFFLE(1,0,1,0));
  // z.z,z.w,w.z,w.w
  udVec4f vTemp4 = _mm_shuffle_ps(M.r[2],M.r[3],_MM_SHUFFLE(3,2,3,2));

  // Assign result to *this
  // x.x,y.x,z.x,w.x
  r[0] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(2,0,2,0));
  // x.y,y.y,z.y,w.y
  r[1] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(3,1,3,1));
  // x.z,y.z,z.z,w.z
  r[2] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(2,0,2,0));
  // x.w,y.w,z.w,w.w
  r[3] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(3,1,3,1));
}

udVec4f udMatrix4f::Transform3(udVec4f v) const
{
  // Splat x,y,z and w
  udVec4f vTempX = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0));
  udVec4f vTempY = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  udVec4f vTempZ = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2));
  // Mul by the matrix
  vTempX = _mm_mul_ps(vTempX, r[0]);
  vTempY = _mm_mul_ps(vTempY, r[1]);
  vTempZ = _mm_mul_ps(vTempZ, r[2]);
  // Add them all together
  vTempX = _mm_add_ps(vTempX, vTempY);
  vTempZ = _mm_add_ps(vTempZ, r[3]);
  vTempX = _mm_add_ps(vTempX, vTempZ);
  return vTempX;
}

udVec4f udMatrix4f::Transform4(udVec4f v) const
{
  // Splat x,y,z and w
  udVec4f vTempX = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0));
  udVec4f vTempY = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1));
  udVec4f vTempZ = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2));
  udVec4f vTempW = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3));
  // Mul by the matrix
  vTempX = _mm_mul_ps(vTempX, r[0]);
  vTempY = _mm_mul_ps(vTempY, r[1]);
  vTempZ = _mm_mul_ps(vTempZ, r[2]);
  vTempW = _mm_mul_ps(vTempW, r[3]);
  // Add them all together
  vTempX = _mm_add_ps(vTempX, vTempY);
  vTempZ = _mm_add_ps(vTempZ, vTempW);
  vTempX = _mm_add_ps(vTempX, vTempZ);
  return vTempX;
}

void udMatrix4f::SetRotationX(float angle)
{
  float sinAngle = sinf(angle);
  float cosAngle = cosf(angle);

  udVec4f vSin = _mm_set_ss(sinAngle);
  udVec4f vCos = _mm_set_ss(cosAngle);
  // x = 0,y = cos,z = sin, w = 0
  vCos = _mm_shuffle_ps(vCos,vSin,_MM_SHUFFLE(3,0,0,3));

  r[0] = udVec4fConst_Zero1X;
  r[1] = vCos;
  // x = 0,y = sin,z = cos, w = 0
  vCos = _mm_shuffle_ps(vCos,vCos,_MM_SHUFFLE(3,1,2,0));
  // x = 0,y = -sin,z = cos, w = 0
  vCos = _mm_mul_ps(vCos, _mm_mul_ps(udVec4fConst_NegOne, udVec4fConst_Zero1Y));
  r[2] = vCos;
  r[3] = udVec4fConst_Zero1W;
}

void udMatrix4f::SetRotationY(float angle)
{
  float sinAngle = sinf(angle);
  float cosAngle = cosf(angle);

  udVec4f vSin = _mm_set_ss(sinAngle);
  udVec4f vCos = _mm_set_ss(cosAngle);
  // x = sin,y = 0,z = cos, w = 0
  vSin = _mm_shuffle_ps(vSin,vCos,_MM_SHUFFLE(3,0,3,0));
  
  r[2] = vSin;
  r[1] = udVec4fConst_Zero1Y;
  // x = cos,y = 0,z = sin, w = 0
  vSin = _mm_shuffle_ps(vSin,vSin,_MM_SHUFFLE(3,0,1,2));
  // x = cos,y = 0,z = -sin, w = 0
  vSin = _mm_mul_ps(vSin, _mm_mul_ps(udVec4fConst_NegOne, udVec4fConst_Zero1Z));
  r[0] = vSin;
  r[3] = udVec4fConst_Zero1W;
}

void udMatrix4f::SetRotationZ(float angle)
{
  float sinAngle = sinf(angle);
  float cosAngle = cosf(angle);

  udVec4f vSin = _mm_set_ss(sinAngle);
  udVec4f vCos = _mm_set_ss(cosAngle);
  // x = cos,y = sin,z = 0, w = 0
  vCos = _mm_unpacklo_ps(vCos,vSin);
  
  r[0] = vCos;
  // x = sin,y = cos,z = 0, w = 0
  vCos = _mm_shuffle_ps(vCos,vCos,_MM_SHUFFLE(3,2,0,1));
  // x = cos,y = -sin,z = 0, w = 0
  vCos = _mm_mul_ps(vCos,  _mm_mul_ps(udVec4fConst_NegOne, udVec4fConst_Zero1X));
  r[1] = vCos;
  r[2] = udVec4fConst_Zero1Z;
  r[3] = udVec4fConst_Zero1W;
}

void udMatrix4f::SetScalingFromVector(udVec4f v)
{
  r[0] = _mm_and_ps(v, udVec4fConst_MaskX);
  r[1] = _mm_and_ps(v, udVec4fConst_MaskY);
  r[2] = _mm_and_ps(v, udVec4fConst_MaskZ);
  r[3] = udVec4fConst_Zero1W;
}

void udMatrix4f::SetPerspectiveFovRHDX(float fovAngleY, float aspect, float nearZ, float farZ)
{
  float sinFov = sinf(fovAngleY * 0.5f);
  float cosFov = cosf(fovAngleY * 0.5f);

  float height = cosFov / sinFov;
  float width = height / aspect;
  float q = farZ / (nearZ - farZ);

  r[0] = udVec4fSet(width, 0.0f, 0.0f, 0.0f);
  r[1] = udVec4fSet(0.0f, height, 0.0f, 0.0f);
  r[2] = udVec4fSet(0.0f, 0.0f, q, -1.0f);
  r[3] = udVec4fSet(0.0f, 0.0f, q * nearZ, 0.0f);
}
