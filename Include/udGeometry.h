//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Frank Hart, June 2020
//

#ifndef UDGEOMETRY_H
#define UDGEOMETRY_H

#include "udMath.h"

//------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------

#//define UD_USE_EXACT_MATH

template<typename T>
struct udEpsilon {};

template<> struct udEpsilon<float> { static const float value; };
template<> struct udEpsilon<double> { static const double value; };

enum udGeometryCode
{
  udGC_Fail = 0,
  udGC_Success,
  udGC_Parallel,
  udGC_Overlapping,
  udGC_Intersecting,
  udGC_NotIntersecting,
  udGC_CompletelyInside,
  udGC_CompletelyOutside
};

//------------------------------------------------------------------------
// Index
//------------------------------------------------------------------------

template<typename T>bool udIsZero(T);

template<typename T> udGeometryCode udCP_PointLine3(const udVector3<T> &lineOrigin, const udVector3<T> &lineDirection, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);
template<typename T> udGeometryCode udCP_PointSegment3(const udVector3<T> &ls0, const udVector3<T> &ls1, const udVector3<T> &point, udVector3<T> &out, T *pU = nullptr);
template<typename T> udGeometryCode udCP_SegmentSegment3(const udVector3<T> &a0, const udVector3<T> &a1, const udVector3<T> &b0, const udVector3<T> &b1, udVector3<T> &aOut, udVector3<T> &bOut, udVector2<T> *pU = nullptr);

//------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------

#ifdef UD_USE_EXACT_MATH
template<typename T>
bool udIsZero(T value)
{
  return value == T(0);
}
#else
template<typename T>
bool udIsZero(T value)
{
  return udAbs(value) < udEpsilon<T>::value;
}
#endif

template<typename T>
udGeometryCode udCP_PointLine3(const udVector3<T> &lineOrigin, const udVector3<T> &lineDirection, const udVector3<T> &point, udVector3<T> &out, T * pU)
{
  udVector3<T> w;
  w = point - lineOrigin;
  T proj = udDot(w, lineDirection);
  out = lineOrigin + proj * lineDirection;
  if (pU != nullptr)
    *pU = proj;
  return udGC_Success;
}

template<typename T>
udGeometryCode udCP_PointSegment3(const udVector3<T> &ls0, const udVector3<T> &ls1, const udVector3<T> &point, udVector3<T> &out, T *pU)
{
  udVector3<T> w = point - ls0;
  udVector3<T> axis = ls1 - ls0;
  double u;

  double proj = udDot(w, ls1 - ls0);

  if (proj <= T(0))
  {
    u = T(0);
  }
  else
  {
    double vsq = udMagSq3(axis);

    if (proj >= vsq)
      u = T(1);
    else
      u = (proj / vsq);
  }

  out = ls0 + u * axis;

  if (pU != nullptr)
    *pU = u;

  return udGC_Success;
}

template<typename T>
udGeometryCode udCP_SegmentSegment3(const udVector3<T> &a0, const udVector3<T> &a1, const udVector3<T> &b0, const udVector3<T> &b1, udVector3<T> &aOut, udVector3<T> &bOut, udVector2<T> * pU)
{
  T ua, ub; //could be outputs?
  udGeometryCode result = udGC_Success;

  //directions
  udVector3<T> da = a1 - a0;
  udVector3<T> db = b1 - b0;

  //compute intermediate parameters
  udDouble3 w0(a0 - b0);
  double a = udDot(da, da);
  double b = udDot(da, db);
  double c = udDot(db, db);
  double d = udDot(da, w0);
  double e = udDot(db, w0);
  double denom = a*c - b*b;

  double sn, sd, tn, td;

  // if denom is zero, try finding closest point on segment1 to origin0
  if (udIsZero(denom))
  {
    if (udIsZero(a)) //length of a is 0
    {
      if (udIsZero(c)) //length of b is also 0
      {
        ua = T(0);
        ub = T(0);
        aOut = a0;
        bOut = b0;
      }
      else
      {
        udCP_PointSegment3(b0, b1, a0, bOut, &ub);
        aOut = a0;
        ua = T(0);
      }
      goto epilogue;
    }
    else if (udIsZero(c)) //length of b is 0
    {
      udCP_PointSegment3(a0, a1, b0, aOut, &ua);
      bOut = b0;
      ub = T(0);
      goto epilogue;
    }

    // clamp ua to 0
    sd = td = c;
    sn = T(0);
    tn = e;

    //Do the line segments overlap?
    udVector3<T> w1((a0 + da) - b0);
    udVector3<T> w2(a0 - (b0 + db));
    udVector3<T> w3((a0 + da) - (b0 + db));
    bool bse = (e < T(0));
    if (!(bse == (udDot(w1, db) < T(0)) && bse == (udDot(w2, db) < T(0)) && bse == (udDot(w3, db) < T(0))))
      result = udGC_Overlapping;
  }
  else
  {
    // clamp ua within [0,1]
    sd = td = denom;
    sn = b*e - c*d;
    tn = a*e - b*d;

    // clamp ua to 0
    if (sn < T(0))
    {
      sn = T(0);
      tn = e;
      td = c;
    }
    // clamp ua to 1
    else if (sn > sd)
    {
      sn = sd;
      tn = e + b;
      td = c;
    }
  }

  // clamp ub within [0,1]
  // clamp ub to 0
  if (tn < T(0))
  {
    ub = T(0);
    // clamp ua to 0
    if (-d < T(0))
      ua = T(0);
    // clamp ua to 1
    else if (-d > a)
      ua = T(1);
    else
      ua = -d / a;
  }
  // clamp ub to 1
  else if (tn > td)
  {
    ub = T(1);
    // clamp ua to 0
    if ((-d + b) < T(0))
      ua = T(0);
    // clamp ua to 1
    else if ((-d + b) > a)
      ua = T(1);
    else
      ua = (-d + b) / a;
  }
  else
  {
    ub = tn / td;
    ua = sn / sd;
  }

epilogue:
  aOut = a0 + ua*da;
  bOut = b0 + ub*db;

  if (pU != nullptr)
  {
    pU->x = ua;
    pU->y = ub;
  }

  return result;
}

#endif
