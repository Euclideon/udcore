#ifndef UDGEOMETRY_H
#define UDGEOMETRY_H

#include "udMath.h"

//------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------

//#define UD_USE_EXACT_MATH

template<typename T>
constexpr T udGetEpsilon();

template<>
constexpr double udGetEpsilon() { return 1e-12; };

template<>
constexpr float udGetEpsilon() { return 1e-6f; };

enum udGeometryCode
{
  udGC_Success,
  udGC_Parallel,
  udGC_Overlapping,
  udGC_Intersecting,
  udGC_NotIntersecting,
  udGC_CompletelyInside,
  udGC_CompletelyOutside
};

template<typename T>bool udIsZero(T);

template<typename T> udGeometryCode udCP_PointLine3(const udVector3<T> &lineOrigin, const udVector3<T> &lineDirection, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);
template<typename T> udGeometryCode udCP_PointSegment3(const udVector3<T> &ls0, const udVector3<T> &ls1, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);
template<typename T> udGeometryCode udCP_SegmentSegment3(const udVector3<T> &a0, const udVector3<T> &a1, const udVector3<T> &b0, const udVector3<T> &b1, udVector3<T> &aOut, udVector3<T> &bOut, udVector2<T> *pU = nullptr);

#include "udGeometry_Inl.h"

#endif
