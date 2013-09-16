#ifndef UDMATRIX_H
#define UDMATRIX_H

#include "udVec.h"
UD_ALIGN_16_VS struct udMatrix4f
{
  udVec4f r[4];

  void SetIdentity();
  void SetFromArray(const float values[16]);
  void SetFromArray(const double values[16]);
  void Multiply(const udMatrix4f &a, const udMatrix4f &b);
  void Inverse(const udMatrix4f &a, udVec4f *determinant = nullptr);
  void Transpose(const udMatrix4f &a);
  void SetRotationX(float angle);
  void SetRotationY(float angle);
  void SetRotationZ(float angle);
  void SetScalingFromVector(udVec4f v);
  void SetPerspectiveFovRHDX(float fovAngleY, float aspect, float nearZ, float farZ);
  
  // Helpers
  void Transpose() { Transpose(*this); }
  void Inverse() { Inverse(*this, nullptr); }
  udVec4f Transform3(udVec4f a) const;
  udVec4f Transform4(udVec4f a) const;
} UD_ALIGN_16_GCC;


#endif // UDMATRIX_H

