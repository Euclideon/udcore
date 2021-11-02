
#ifdef UD_USE_EXACT_MATH
template<typename T> bool udIsZero(T value) { return value == T(0); }
#else
template<typename T> bool udIsZero(T value) { return udAbs(value) < udGetEpsilon<T>(); }
#endif

//------------------------------------------------------------------------------------
// udPlane
//------------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
udResult udPlane<T>::Set(const udVector3<T> &p0, const udVector3<T> &p1, const udVector3<T> &p2)
{
  udResult result;

  //Get plane vector
  udVector3<T> u = p1 - p0;
  udVector3<T> v = p2 - p0;
  udVector3<T> w = udCross3(u, v);

  //normalise for cheap distance checks
  T lensq = udMagSq(w);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  normal = w / udSqrt(lensq);
  offset = udDot(-p0, normal);

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
udResult udPlane<T>::Set(const udVector3<T> &point, const udVector3<T> &_normal)
{
  udResult result;

  //normalise for cheap distance checks
  T lensq = udMagSq(_normal);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  normal = _normal / udSqrt(lensq);
  offset = -udDot(point, normal);

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udAABB
//------------------------------------------------------------------------------------

template<typename T, int R>
udResult udAABB<T, R>::Set(const VECTOR_T &minPt, const VECTOR_T &maxPt)
{
  udResult result;

  UD_ERROR_IF(minPt.x > maxPt.x, udR_Failure);
  UD_ERROR_IF(minPt.y > maxPt.y, udR_Failure);
  UD_ERROR_IF(minPt.z > maxPt.z, udR_Failure);

  minPoint = minPt;
  maxPoint = maxPt;

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udLine
//------------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
udResult udLine<T, R>::SetFromEndPoints(const VECTOR_T &p0, const VECTOR_T &p1)
{
  udResult result;
  VECTOR_T v = p1 - p0;

  //normalise for cheap distance checks
  T lensq = udMagSq(v);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  direction = v / udSqrt(lensq);
  origin = p0;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
udResult udLine<T, R>::SetFromDirection(const VECTOR_T &p0, const VECTOR_T &dir)
{
  udResult result;

  //normalise for cheap distance checks
  T lensq = udMagSq(dir);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  direction = dir / udSqrt(lensq);
  origin = p0;

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udRay
//------------------------------------------------------------------------------------


// ****************************************************************************
// Author: Frank Hart, November 2021
template<typename T, int R>
udResult udRay<T, R>::SetFromEndPoints(const VECTOR_T &p0, const VECTOR_T &p1)
{
  udResult result;
  VECTOR_T v = p1 - p0;

  //normalise for cheap distance checks
  T lensq = udMagSq(v);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  direction = v / udSqrt(lensq);
  origin = p0;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, November 2021
template<typename T, int R>
udResult udRay<T, R>::SetFromDirection(const VECTOR_T &p0, const VECTOR_T &dir)
{
  udResult result;

  //normalise for cheap distance checks
  T lensq = udMagSq(dir);

  UD_ERROR_IF(udIsZero(lensq), udR_Failure);

  direction = dir / udSqrt(lensq);
  origin = p0;

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udSegment
//------------------------------------------------------------------------------------

template<typename T, int R>
udResult udSegment<T, R>::Set(const VECTOR_T &_p0, const VECTOR_T &_p1)
{
  udResult result;

  UD_ERROR_IF((udAreEqual<T, R>(_p0, _p1)), udR_Failure);

  p0 = _p0;
  p1 = _p1;

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udTriangle
//------------------------------------------------------------------------------------

template<typename T, int R>
udResult udTriangle<T, R>::Set(const VECTOR_T &_p0, const VECTOR_T &_p1, const VECTOR_T &_p2)
{
  udResult result;

  UD_ERROR_IF((udAreEqual<T, R>(_p0, _p1)), udR_Failure);
  UD_ERROR_IF((udAreEqual<T, R>(_p0, _p2)), udR_Failure);
  UD_ERROR_IF((udAreEqual<T, R>(_p1, _p2)), udR_Failure);

  p0 = _p0;
  p1 = _p1;
  p2 = _p2;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
T udTriangle<T, R>::GetArea() const
{
  udVector3<T> sideLengths = GetSideLengths();
  T p = (sideLengths[0] + sideLengths[1] + sideLengths[2]) / T(2);

  // Theoritically these values should not be below zero, but due to floting point
  // error, they can be. So we need to check.
  T a = (p - sideLengths[0]);
  if (a <= T(0))
    return T(0);

  T b = (p - sideLengths[1]);
  if (b <= T(0))
    return T(0);

  T c = (p - sideLengths[2]);
  if (c <= T(0))
    return T(0);

  return udSqrt(p * a * b * c);
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
udVector3<T> udTriangle<T, R>::GetSideLengths() const
{
  return udVector3<T>::create(udMag(p0 - p1), udMag(p0 - p2), udMag(p1 - p2));
}

//------------------------------------------------------------------------------------
// Utility
//------------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, July 2020
template<typename T>
bool udAreEqual(T a, T b)
{
  return udIsZero(a - b);
}

// ****************************************************************************
// Author: Frank Hart, July 2020
template<typename T, int R>
bool udAreEqual(const VECTOR_T &v0, const VECTOR_T &v1)
{
  for (int i = 0; i < R; i++)
  {
    if (!udAreEqual(v0[i], v1[i]))
      return false;
  }
  return true;
}

// ****************************************************************************
// Author: Frank Hart, July 2020
template<typename T>
T udGeometry_ScalarTripleProduct(const udVector3<T> &u, const udVector3<T> &v, const udVector3<T> &w)
{
  return udDot(udCross3(u, v), w);
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
void udGeometry_SortLowToHigh(T &a, T &b)
{
  if (b < a)
  {
    T temp = a;
    a = b;
    b = temp;
  }
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
udVector3<T> udGeometry_SortLowToHigh(const udVector3<T> &a)
{
  udVector3<T> result = a;

  udGeometry_SortLowToHigh(result[0], result[1]);
  udGeometry_SortLowToHigh(result[0], result[2]);
  udGeometry_SortLowToHigh(result[1], result[2]);

  return result;
}

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
T udGeometry_Sum(const VECTOR_T &v)
{
  T total = v[0];
  for (int i = 1; i < R; i++)
    total += v[i];
  return total;
}

// ****************************************************************************
// Author: Frank Hart, July 2020
// Based on Real Time Collision Detection, Christer Ericson p184
template<typename T, int R>
udResult udGeometry_Barycentric(const udTriangle<T, R> &tri, const VECTOR_T &p, udVector3<T> *pUVW)
{
  udResult result;

  VECTOR_T v0 = tri.p1 - tri.p0;
  VECTOR_T v1 = tri.p2 - tri.p0;
  VECTOR_T v2 = p - tri.p0;

  T d00 = udDot(v0, v0);
  T d01 = udDot(v0, v1);
  T d11 = udDot(v1, v1);
  T d20 = udDot(v2, v0);
  T d21 = udDot(v2, v1);

  T denom = d00 * d11 - d01 * d01;

  UD_ERROR_NULL(pUVW, udR_InvalidParameter);

  // TODO we need a udR_DivideByZero
  UD_ERROR_IF(udIsZero(denom), udR_Failure);

  pUVW->y = (d11 * d20 - d01 * d21) / denom;
  pUVW->z = (d00 * d21 - d01 * d20) / denom;
  pUVW->x = T(1) - pUVW->y - pUVW->z;

  result = udR_Success;
epilogue:
  return result;
}

//--------------------------------------------------------------------------------
// Distance Queries
//--------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
T udGeometry_SignedDistance(const udPlane<T> &plane, const udVector3<T> &point)
{
  return udDot(point, plane.normal) + plane.offset;
}

//--------------------------------------------------------------------------------
// Intersection Test Queries
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Closest Points Queries
//--------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T>
udResult udGeometry_CP3PointPlane(const udVector3<T> &point, const udPlane<T> &plane, udVector3<T> *pOut)
{
  udResult result;
  T signedDistance = {};

  UD_ERROR_NULL(pOut, udR_InvalidParameter);

  signedDistance = udGeometry_SignedDistance(plane, point);
  *pOut = point - signedDistance * plane.normal;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, June 2020
template<typename T, int R>
udResult udGeometry_CPPointLine(const VECTOR_T &point, const udLine<T, R> &line, CPPointLineResult<T, R> *pData)
{
  udResult result;
  VECTOR_T w = {};

  UD_ERROR_NULL(pData, udR_InvalidParameter);

  w = point - line.origin;
  pData->u = udDot(w, line.direction);
  pData->point = line.origin + pData->u * line.direction;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, June 2020
template<typename T, int R>
udResult udGeometry_CPPointSegment(const VECTOR_T &point, const udSegment<T, R> &seg, CPPointSegmentResult<T, R> *pResult)
{
  udResult result;
  VECTOR_T w = {};
  VECTOR_T axis = {};
  T proj;
  T vsq;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  w = point - seg.p0;
  axis = seg.p1 - seg.p0;

  proj = udDot(w, seg.p1 - seg.p0);

  if (proj <= T(0))
  {
    pResult->u = T(0);
  }
  else
  {
    vsq = udMagSq(axis);

    if (proj >= vsq)
      pResult->u = T(1);
    else
      pResult->u = (proj / vsq);
  }

  pResult->point = seg.p0 + pResult->u * axis;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, June 2020
// Based of the work of James M. Van Verth and Lars M. Biship, taken from 'Essential Mathematics for Games and Interactive Applications: A Programmer's Guide, Second Edition
template<typename T, int R>
udResult udGeometry_CPSegmentSegment(const udSegment<T, R> &seg_a, const udSegment<T, R> &seg_b, CPSegmentSegmentResult<T, R> *pResult)
{
  udResult result;
  VECTOR_T da = {};
  VECTOR_T db = {};
  VECTOR_T w0 = {};
  T a, b, c, d, e, denom, sn, sd, tn, td;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  pResult->code = udGC_Success;

  //directions
  da = seg_a.p1 - seg_a.p0;
  db = seg_b.p1 - seg_b.p0;

  //compute intermediate parameters
  w0 = seg_a.p0 - seg_b.p0;
  a = udDot(da, da);
  b = udDot(da, db);
  c = udDot(db, db);
  d = udDot(da, w0);
  e = udDot(db, w0);
  denom = a*c - b*b;

  // if denom is zero, try finding closest point on segment1 to origin0
  if (udIsZero(denom))
  {
    if (udIsZero(a)) //length of a is 0
    {
      if (udIsZero(c)) //length of b is also 0
      {
        pResult->u_a = T(0);
        pResult->u_b = T(0);
        pResult->cp_a = seg_a.p0;
        pResult->cp_b = seg_b.p0;
      }
      else
      {
        CPPointSegmentResult<T, R> tempResult = {};
        UD_ERROR_CHECK(udGeometry_CPPointSegment(seg_a.p0, seg_b, &tempResult));
        pResult->cp_a = seg_a.p0;
        pResult->cp_b = tempResult.point;
        pResult->u_a = T(0);
        pResult->u_b = tempResult.u;
      }
      UD_ERROR_SET_NO_BREAK(udR_Success);
    }
    else if (udIsZero(c)) //length of b is 0
    {
      CPPointSegmentResult<T, R> tempResult = {};
      UD_ERROR_CHECK(udGeometry_CPPointSegment(seg_b.p0, seg_a, &tempResult));
      pResult->cp_a = tempResult.point;
      pResult->cp_b = seg_b.p0;
      pResult->u_a = tempResult.u;
      pResult->u_b = T(0);
      UD_ERROR_SET_NO_BREAK(udR_Success);
    }

    // clamp pResult->u_a to 0
    sd = td = c;
    sn = T(0);
    tn = e;

    //Do the line segments overlap?
    udVector3<T> w1((seg_a.p0 + da) - seg_b.p0);
    udVector3<T> w2(seg_a.p0 - (seg_b.p0 + db));
    udVector3<T> w3((seg_a.p0 + da) - (seg_b.p0 + db));
    bool bse = (e < T(0));
    if (!(bse == (udDot(w1, db) < T(0)) && bse == (udDot(w2, db) < T(0)) && bse == (udDot(w3, db) < T(0))))
      pResult->code = udGC_Overlapping;
  }
  else
  {
    // clamp pResult->u_a within [0,1]
    sd = td = denom;
    sn = b * e - c * d;
    tn = a * e - b * d;
    
    // clamp pResult->u_a to 0
    if (sn < T(0))
    {
      sn = T(0);
      tn = e;
      td = c;
    }
    // clamp pResult->u_a to 1
    else if (sn > sd)
    {
      sn = sd;
      tn = e + b;
      td = c;
    }
  }

  // clamp pResult->u_b within [0,1]
  // clamp pResult->u_b to 0
  if (tn < T(0))
  {
    pResult->u_b = T(0);
    // clamp pResult->u_a to 0
    if (-d < T(0))
      pResult->u_a = T(0);
    // clamp pResult->u_a to 1
    else if (-d > a)
      pResult->u_a = T(1);
    else
      pResult->u_a = -d / a;
  }
  // clamp pResult->u_b to 1
  else if (tn > td)
  {
    pResult->u_b = T(1);
    // clamp pResult->u_a to 0
    if ((-d + b) < T(0))
      pResult->u_a = T(0);
    // clamp pResult->u_a to 1
    else if ((-d + b) > a)
      pResult->u_a = T(1);
    else
      pResult->u_a = (-d + b) / a;
  }
  else
  {
    pResult->u_b = tn / td;
    pResult->u_a = sn / sd;
  }

  pResult->cp_a = seg_a.p0 + pResult->u_a * da;
  pResult->cp_b = seg_b.p0 + pResult->u_b * db;

  result = udR_Success;
epilogue:
  return result;
}


// ****************************************************************************
// Author: Frank Hart, October 2021
template<typename T, int R>
udResult udGeometry_TIPointAABB(const VECTOR_T &point, const udAABB<T, R> &box, udGeometryCode *pCode)
{
  udResult result;

  UD_ERROR_NULL(pCode, udR_InvalidParameter);

  *pCode = udGC_Intersecting;
  for (int i = 0; i < 3; i++)
  {
    if (point[i] < box.minPoint[i] || point[i] > box.maxPoint[i])
    {
      *pCode = udGC_NotIntersecting;
      break;
    }
  }

  result = udR_Success;
epilogue:
  return result;
}


// ****************************************************************************
// Author: Frank Hart, October 2021
template<typename T, int R>
udResult udGeometry_TIAABBAABB(const udAABB<T, R> &box0, const udAABB<T, R> &box1, udGeometryCode *pCode)
{
  udResult result;

  UD_ERROR_NULL(pCode, udR_InvalidParameter);

  *pCode = udGC_Intersecting;
  for (int i = 0; i < 3; i++)
  {
    if (box0.minPoint[i] > box1.maxPoint[i] || box1.minPoint[i] > box0.maxPoint[i])
    {
      *pCode = udGC_NotIntersecting;
      break;
    }
  }

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, October 2021
template<typename T>
udResult udGeometry_TI2PointPolygon(const udVector2<T> &point, const udVector2<T> *pPolygon, size_t count, udGeometryCode *pCode)
{
  udResult result;
  /*size_t total = 0;
  udLine<T, 2> line = {};
  line.CreateFromDirection(point, udVector3::create(T(1), T(0), T(0)));
  for (size_t i = 0; i < count; i++)
  {
    j = (i + 1) % count;
    udSegment<T, 2> seg;
    seg.p0 = pPolygon[i];
    seg.p1 = pPolygon[j];

    CPLineSegmentResult data = {};
    UD_ERROR_CHECK(CPLineSegmentResult(line, seg, &data), result);

    if ((result.u_s > T(0) && result.u_s < T(1)) && u_l < T(0))
      total++;
  }

  result = (total % 2 == 0) ? udR_CompletelyOutside : udR_CompletelyInside;
epilogue:*/
  return result;
}

// ****************************************************************************
// Author: Frank Hart, July 2020
// Based on Real Time Collision Detection, Christer Ericson p184
template<typename T>
udResult udGeometry_FI3SegmentTriangle(const udSegment3<T> &seg, const udTriangle3<T> &tri, FI3SegmentTriangleResult<T> *pResult)
{
  udResult result;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  udVector3<T> s0s1 = seg.p1 - seg.p0;
  udVector3<T> s0t0 = tri.p0 - seg.p0;
  udVector3<T> s0t1 = tri.p1 - seg.p0;
  udVector3<T> s0t2 = tri.p2 - seg.p0;

  T u = udGeometry_ScalarTripleProduct(s0s1, s0t2, s0t1);
  T v = udGeometry_ScalarTripleProduct(s0s1, s0t0, s0t2);
  T w = udGeometry_ScalarTripleProduct(s0s1, s0t1, s0t0);

  // TODO Line is on triangle plane
  if (udIsZero(u) && udIsZero(v) && udIsZero(w))
  {
    // Flag as fail for now...
    UD_ERROR_SET(udR_Failure);
  }

  int sign = 0;
  sign |= (u < T(0) ? 1 : 0);
  sign |= (v < T(0) ? 2 : 0);
  sign |= (w < T(0) ? 4 : 0);

  if (sign > 0 && sign < 7)
  {
    pResult->code = udGC_NotIntersecting;
    UD_ERROR_SET(udR_Success);
  }

  T denom = T(1) / (u + v + w);
  u *= denom;
  v *= denom;
  w *= denom;

  pResult->point = u * tri.p0 + v * tri.p1 + w * tri.p2;
  pResult->code = udGC_Intersecting;

  result = udR_Success;
epilogue:
  return result;
}

template<typename T, int R>
udResult udGeometry_CPPointTriangle(const VECTOR_T &point, const udTriangle<T, R> &tri, VECTOR_T *pOut)
{
  udResult result;
  VECTOR_T v01 = {};
  VECTOR_T v02 = {};
  VECTOR_T v0p = {};
  T d1, d2;

  UD_ERROR_NULL(pOut, udR_InvalidParameter);

  v01 = tri.p1 - tri.p0;
  v02 = tri.p2 - tri.p0;
  v0p = point - tri.p0;

  d1 = udDot(v01, v0p);
  d2 = udDot(v02, v0p);

  do
  {
    if (d1 <= T(0) && d2 <= T(0))
    {
      *pOut = tri.p0;
      break;
    }

    udVector3<T> v1p = point - tri.p1;
    T d3 = udDot(v01, v1p);
    T d4 = udDot(v02, v1p);
    if (d3 >= T(0) && d4 <= d3)
    {
      *pOut = tri.p1;
      break;
    }

    T v2 = d1 * d4 - d3 * d2;
    if (v2 <= T(0) && d1 >= T(0) && d3 <= T(0))
    {
      T v = d1 / (d1 - d3);
      *pOut = tri.p0 + v * v01;
      break;
    }

    udVector3<T> v2p = point - tri.p2;
    T d5 = udDot(v01, v2p);
    T d6 = udDot(v02, v2p);
    if (d6 >= T(0) && d5 <= d6)
    {
      *pOut = tri.p2;
      break;
    }

    T v1 = d5 * d2 - d1 * d6;
    if (v1 <= T(0) && d2 >= T(0) && d6 <= T(0))
    {
      T w = d2 / (d2 - d6);
      *pOut = tri.p0 + w * v02;
      break;
    }

    T v0 = d3 * d6 - d5 * d4;
    if (v0 <= T(0) && (d4 - d3) >= T(0) && (d5 - d6) >= T(0))
    {
      T w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
      *pOut = tri.p1 + w * (tri.p2 - tri.p1);
      break;
    }

    T denom = T(1) / (v0 + v1 + v2);
    T v = v1 * denom;
    T w = v2 * denom;
    *pOut = tri.p0 + v01 * v + v02 * w;

  } while (false);

  result = udR_Success;
epilogue:
  return result;
}
