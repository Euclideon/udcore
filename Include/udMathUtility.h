#ifndef UDMATHUTILITY_H
#define UDMATHUTILITY_H

#include "udMath.h"

// Find a (non-normalised) perpendicular vector to axis, in no particular direction.
// A zero vector input will return a zero vector
template <typename T>
udVector3<T> udPerpendicular3(const udVector3<T> &axis);

//Determine if a quaternion transformation produces an axis-aligned basis, given an axis-aligned basis.
//If so, the transformation will be applied to the basis extentsIn [x, y, z], to produce extentsOut.
template <typename T>
bool udIsRotatedAxisStillAxisAligned(const udQuaternion<T> &q, const udVector3<T> &extentsIn, udVector3<T> &extentsOut, const T epsilon = T(0));

#include "udMathUtility_Inl.h"

#endif
