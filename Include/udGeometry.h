#ifndef UDGEOMETRY_H
#define UDGEOMETRY_H

#include "udMath.h"

//------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------

//#define UD_USE_EXACT_MATH

//Check individual geometry queries to determine possible return values.
enum udGeometryCode
{
  udGC_Success,
  udGC_Overlapping,
  udGC_Intersecting,
  udGC_NotIntersecting,
  udGC_CompletelyInside,
  udGC_CompletelyOutside
};

//If UD_USE_EXACT_MATH is not defined, this function tests if value is within an epsilon of zero, as defined in udGetEpsilon().
//Otherwise it will test if value == T(0)
template<typename T> bool udIsZero(T value);

//Closest point between a point and a line in 3D.
//Returns: udGC_Success
template<typename T> udGeometryCode udGeometry_CPPointLine3(const udVector3<T> &lineOrigin, const udVector3<T> &lineDirection, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);

//Closest point between a point and a line segment in 3D.
//Returns: udGC_Success
template<typename T> udGeometryCode udGeometry_CPPointSegment3(const udVector3<T> &ls0, const udVector3<T> &ls1, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);

//Closest point between two line segments in 3D.
//Returns: udGC_Success
//         udGC_Overlapping if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
template<typename T> udGeometryCode udGeometry_CPSegmentSegment3(const udVector3<T> &a0, const udVector3<T> &a1, const udVector3<T> &b0, const udVector3<T> &b1, udVector3<T> &aOut, udVector3<T> &bOut, udVector2<T> *pU = nullptr);

#include "udGeometry_Inl.h"

#endif
