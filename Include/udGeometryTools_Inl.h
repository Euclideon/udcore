
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
udResult udAABB<T, R>::Set(const udVector_t &minPt, const udVector_t &maxPt)
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

template<typename T, int R>
void udAABB<T, R>::Merge(udAABB<T, R> const &other)
{
  for (int i = 0; i < R; i++)
  {
    minPoint[i] = udMin(minPoint[i], other.minPoint[i]);
    maxPoint[i] = udMax(maxPoint[i], other.maxPoint[i]);
  }
}

//------------------------------------------------------------------------------------
// udLine
//------------------------------------------------------------------------------------

// ****************************************************************************
// Author: Frank Hart, August 2020
template<typename T, int R>
udResult udLine<T, R>::SetFromEndPoints(const udVector_t &p0, const udVector_t &p1)
{
  udResult result;
  udVector_t v = p1 - p0;

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
udResult udLine<T, R>::SetFromDirection(const udVector_t &p0, const udVector_t &dir)
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
udResult udRay<T, R>::SetFromEndPoints(const udVector_t &p0, const udVector_t &p1)
{
  udResult result;
  udVector_t v = p1 - p0;

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
udResult udRay<T, R>::SetFromDirection(const udVector_t &p0, const udVector_t &dir)
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
udResult udSegment<T, R>::Set(const udVector_t &_p0, const udVector_t &_p1)
{
  udResult result;

  UD_ERROR_IF((udAreEqual<T, R>(_p0, _p1)), udR_Failure);

  p0 = _p0;
  p1 = _p1;

  result = udR_Success;
epilogue:
  return result;
}

template<typename T, int R>
udResult udSegment<T, R>::GetCenteredForm(udVector_t *pCentre, udVector_t *pDirection, udVector_t *pExtent) const
{
  udResult result;

  UD_ERROR_NULL(pCentre, udR_InvalidParamter);
  UD_ERROR_NULL(pDirection, udR_InvalidParamter);
  UD_ERROR_NULL(pExtent, udR_InvalidParamter);

  *pCenter = T(0.5) * (p0 + p1);
  *pDirection = p1 - p0;

  T lenSq = udMagSq(direction);
  UD_ERROR_IF(udIsZero(lenSq), udR_Failure);

  *pExtent = T(0.5) * (direction / udSqrt(lenSq));

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udHyperSphere
//------------------------------------------------------------------------------------

template<typename T, int R>
udResult udHyperSphere<T, R>::Set(const udVector_t & _centre, T _radius)
{
  udResult result;

  UD_ERROR_IF(_radius < udGetEpsilon<T>(), udR_Failure);

  centre = _centre;
  radius = _radius;

  result = udR_Success;
epilogue:
  return result;
}

//------------------------------------------------------------------------------------
// udTriangle
//------------------------------------------------------------------------------------

template<typename T, int R>
udResult udTriangle<T, R>::Set(const udVector_t &_p0, const udVector_t &_p1, const udVector_t &_p2)
{
  udResult result;

  udTriangle<T, R> temp;

  temp.p0 = _p0;
  temp.p1 = _p1;
  temp.p2 = _p2;

  udVector3<T> sideLengths = udGeometry_SortLowToHigh(temp.GetSideLengths());

  UD_ERROR_IF(udIsZero(sideLengths[2] - (sideLengths[0] + sideLengths[1])), udR_Failure);

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
bool udAreEqual(const udVector_t &v0, const udVector_t &v1)
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
T udGeometry_Sum(const udVector_t &v)
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
udResult udGeometry_Barycentric(const udTriangle<T, R> &tri, const udVector_t &p, udVector3<T> *pUVW)
{
  udResult result;

  udVector_t v0 = tri.p1 - tri.p0;
  udVector_t v1 = tri.p2 - tri.p0;
  udVector_t v2 = p - tri.p0;

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

template <typename T>
udResult SetRotationAround(const udRay3<T> & ray, const udVector3<T> & center, const udVector3<T> & axis, const T & angle, udRay3<T> * pOut)
{
  udResult result;
  udVector3<T> origin = {};
  udVector3<T> direction = {};

  UD_ERROR_NULL(pOut, udR_InvalidParameter);

  udQuaternion<T> rotation = udQuaternion<T>::create(axis, angle);

  udVector3<T> direction = ray.origin - center; // find current direction relative to center
  origin = center + rotation.apply(direction); // define new position
  direction = udDirectionFromYPR((rotation * udQuaternion<T>::create(udDirectionToYPR(ray.direction))).eulerAngles()); // rotate object to keep looking at the center

  UD_ERROR_CHECK(pOut->SetFromDirection(origin, direction));

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
// Find Intersection Queries
//--------------------------------------------------------------------------------

template<typename T> udResult udGeometry_FI3SegmentPlane(const udSegment3<T> &seg, const udPlane<T> &plane, udFI3SegmentPlaneResult<T> *pData)
{
  udResult result;

  UD_ERROR_NULL(pData, udR_InvalidParameter);

  T denom = udDot(plane.normal, seg.Direction());

  //check if line is parallel to plane
  if (udIsZero(denom))
  {
    pData->u = T(0);

    //check if line is on the plane
    T dist = udAbs(udGeometry_SignedDistance(plane, seg.p0));

    if (udIsZero(dist))
      pData->code = udGC_Overlapping;
    else
      pData->code = udGC_NotIntersecting;
  }
  else
  {
    pData->u = (-(udDot(seg.p0, plane.normal) + plane.offset) / denom);
    if (pData->u < T(0))
    {
      pData->u = T(0);
      pData->code = udGC_NotIntersecting;
    }
    else if (pData->u > T(1))
    {
      pData->u = T(1);
      pData->code = udGC_NotIntersecting;
    }
    else
    {
      pData->code = udGC_Intersecting;
    }
  }

  pData->point = seg.p0 + pData->u * seg.Direction();

  result = udR_Success;
epilogue:
  return result;
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
udResult udGeometry_CPPointLine(const udVector_t &point, const udLine<T, R> &line, udCPPointLineResult<T, R> *pData)
{
  udResult result;
  udVector_t w = {};

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
udResult udGeometry_CPPointSegment(const udVector_t &point, const udSegment<T, R> &seg, udCPPointSegmentResult<T, R> *pResult)
{
  udResult result;
  udVector_t w = {};
  udVector_t axis = {};
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
udResult udGeometry_CPLineLine(const udLine<T, R> & line_a, const udLine<T, R> & line_b, udCPLineLineResult<T, R> *pResult)
{
  udResult result;
  udVector_t w0 = {};
  T a, b, c, d;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  //compute intermediate parameters
  w0 = line_a.origin - line_b.origin;
  a = udDot(line_a.direction, line_b.direction);
  b = udDot(line_a.direction, w0);
  c = udDot(line_b.direction, w0);
  d = static_cast<T>(1) - a*a;

  if (udIsZero(d))
  {
    pResult->u_a = static_cast<T>(0);
    pResult->u_b = c;
    pResult->code = udGC_Parallel;
  }
  else
  {
    pResult->u_a = ((a*c - b) / d);
    pResult->u_b = ((c - a*b) / d);
    pResult->code = udGC_Success;
  }

  pResult->cp_a = line_a.origin + pResult->u_a * line_a.direction;
  pResult->cp_b = line_b.origin + pResult->u_b * line_b.direction;

  if (pResult->code == udGC_Parallel && udAreEqual<T, R>(pResult->cp_a, pResult->cp_b))
    pResult->code = udGC_Coincident;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, November 2021
// Based of the work of James M. Van Verth and Lars M. Biship, taken from 'Essential Mathematics for Games and Interactive Applications: A Programmer's Guide, Second Edition
template<typename T, int R>
udResult udGeometry_CPLineSegment(const udLine<T, R> & line, const udSegment<T, R> & seg, udCPLineSegmentResult<T, R> *pResult)
{
  udResult result;
  udVector_t segDir = seg.Direction();
  udVector_t w0 = {};
  T a, b, c, d, denom;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  //compute intermediate parameters
  w0 = (seg.p0 - line.origin);
  a = udDot(segDir, segDir);
  b = udDot(segDir, line.direction);
  c = udDot(segDir, w0);
  d = udDot(line.direction, w0);
  denom = a - b*b;

  // if denom is zero, try finding closest point on line to segment origin
  if (udIsZero(denom))
  {
    pResult->u_s = static_cast<T>(0);
    pResult->u_l = d;
    pResult->code = udGC_Parallel;
  }
  else
  {
    pResult->code = udGC_Success;

    // clamp pResult.uls within [0,1]
    T sn = b*d - c;

    // clamp pResult.uls to 0
    if (sn < static_cast<T>(0))
    {
      pResult->u_s = static_cast<T>(0);
      pResult->u_l = d;
    }
    // clamp pResult.uls to 1
    else if (sn > denom)
    {
      pResult->u_s = static_cast<T>(1);
      pResult->u_l = (d + b);
    }
    else
    {
      pResult->u_s = sn / denom;
      pResult->u_l = (a*d - b*c) / denom;
    }
  }

  pResult->cp_s = seg.p0 + pResult->u_s*segDir;
  pResult->cp_l = line.origin + pResult->u_l*line.direction;

  if (pResult->code == udGC_Parallel && udAreEqual<T, R>(pResult->cp_l, pResult->cp_s))
    pResult->code = udGC_Coincident;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, June 2020
// Based of the work of James M. Van Verth and Lars M. Biship, taken from 'Essential Mathematics for Games and Interactive Applications: A Programmer's Guide, Second Edition
template<typename T, int R>
udResult udGeometry_CPSegmentSegment(const udSegment<T, R> &seg_a, const udSegment<T, R> &seg_b, udCPSegmentSegmentResult<T, R> *pResult)
{
  udResult result;
  udVector_t da = {};
  udVector_t db = {};
  udVector_t w0 = {};
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
udResult udGeometry_TIPointAABB(const udVector_t &point, const udAABB<T, R> &box, udGeometryCode *pCode)
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
//template<typename T>
//udResult udGeometry_TI2PointPolygon(const udVector2<T> &point, const udVector2<T> *pPoints, size_t count, udGeometryCode *pCode)
//{
//  udResult result;
//  size_t total = 0;
//  size_t _count = count;
//  udLine<T, 2> line_a = {};
//
//  UD_ERROR_NULL(pPoints, udR_InvalidParameter);
//  UD_ERROR_NULL(pCode, udR_InvalidParameter);
//
//  *pCode = udGC_Success;
//
//  line_a.SetFromDirection(point, {T(1), T(0)});
//  for (size_t i = 0; i < _count; i++)
//  {
//    size_t j = (i + 1) % count;
//    udLine<T, 2> line_b = {};
//    udSegment<T, 2> seg_b = {};
//    udCPLineLineResult<double, 2> lineLineRes = {};
//    udCPPointSegmentResult<double, 2> segLineRes = {};
//    bool intersects;
//
//    // Check if point is on the boundary 
//    UD_ERROR_CHECK(seg_b.Set(pPoints[i], pPoints[j]));
//    UD_ERROR_CHECK(udGeometry_CPPointSegment(point, seg_b, &segLineRes));
//
//    if (udIsZero(udMagSq(segLineRes.point - point)))
//    {
//      *pCode = udGC_OnBoundary;
//      break;
//    }
//
//    // Check is segment crosses line
//    intersects = pPoints[i].y <= point.y && pPoints[j].y > point.y;
//    intersects = intersects || (pPoints[i].y >= point.y && pPoints[j].y < point.y);
//
//    if (!intersects)
//      continue;
//
//    // In-out test
//    UD_ERROR_CHECK(line_b.SetFromEndPoints(pPoints[i], pPoints[j]));
//    UD_ERROR_CHECK(udGeometry_CPLineLine(line_a, line_b, &lineLineRes)); // TODO should use a cheaper TI test
//
//    if (lineLineRes.u_a < T(0))
//        total++;
//  }
//
//  if (*pCode != udGC_OnBoundary)
//    *pCode = (total % 2 == 0) ? udGC_CompletelyOutside : udGC_CompletelyInside;
//
//  result = udR_Success;
//epilogue:
//  return result;
//}

// ****************************************************************************
// Adapted from "Optimal Reliable Point-in-Polygon Test and Differential Coding Boolean Operations on Polygons"
// Authors: Jianqiang Hao, Jianzhi Sun, Yi Chen, Qiang Cai and Li Tan
template<typename T>
udResult udGeometry_TI2PointPolygon(const udVector2<T> & point, const udVector2<T> * pPoints, size_t count, udGeometryCode * pCode)
{
  udResult result;

  size_t k = 0;
  T f = T(0);
  T u1 = T(0);
  T v1 = T(0);
  T u2 = T(0);
  T v2 = T(0);

  UD_ERROR_NULL(pPoints, udR_InvalidParameter);
  UD_ERROR_NULL(pCode, udR_InvalidParameter);

  for (size_t i = 0; i < count; i++)
  {
    size_t j = (i + 1) % count;
    T xi = pPoints[i].x;
    T yi = pPoints[i].y;
    T xj = pPoints[j].x;
    T yj = pPoints[j].y;

    v1 = yi - point.y;
    v2 = yj - point.y;

    if (((v1 < T(0)) && (v2 < T(0))) || ((v1 > T(0)) && (v2 > T(0))))
      continue;

    u1 = xi - point.x;
    u2 = xj - point.x;

    if ((v2 > T(0)) && (v1 <= T(0)))
    {
      f = u1 * v2 - u2 * v1;
      if (f > 0)
      {
        k++;
      }
      else if (f == 0)
      {
        *pCode = udGC_OnBoundary;
        break;
      }
    }
    else if ((v1 > T(0)) && (v2 <= T(0)))
    {
      f = u1 * v2 - u2 * v1;
      if (f < 0)
      {
        k++;
      }
      else if (f == 0)
      {
        *pCode = udGC_OnBoundary;
        break;
      }
    }
    else if ((v2 == T(0)) && (v1 < T(0)))
    {
      f = u1 * v2 - u2 * v1;
      if (f == 0)
      {
        *pCode = udGC_OnBoundary;
        break;
      }
    }
    else if ((v1 == T(0)) && (v2 < T(0)))
    {
      f = u1 * v2 - u2 * v1;
      if (f == 0)
      {
        *pCode = udGC_OnBoundary;
        break;
      }
    }
    else if ((v1 == T(0)) && (v2 == T(0)))
    {
      if ((u2 <= T(0)) && (u1 >= T(0)))
      {
        *pCode = udGC_OnBoundary;
        break;
      }
      else if ((u1 <= T(0)) && (u2 >= T(0)))
      {
        *pCode = udGC_OnBoundary;
        break;
      }
    }
  }

  if (*pCode != udGC_OnBoundary)
    *pCode = (k % 2 == 0) ? udGC_CompletelyOutside : udGC_CompletelyInside;

  result = udR_Success;
epilogue:
  return result;
}

// ****************************************************************************
// Author: Frank Hart, July 2020
// Based on Real Time Collision Detection, Christer Ericson p184
template<typename T>
udResult udGeometry_FI3SegmentTriangle(const udSegment3<T> &seg, const udTriangle3<T> &tri, udFI3SegmentTriangleResult<T> *pResult)
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
udResult udGeometry_CPPointTriangle(const udVector_t &point, const udTriangle<T, R> &tri, udVector_t *pOut)
{
  udResult result;
  udVector_t v01 = {};
  udVector_t v02 = {};
  udVector_t v0p = {};
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

template<typename T>
udResult udGeometry_FI3RayPlane(const udRay3<T> & ray, const udPlane<T> & plane, udFI3RayPlaneResult<T> * pResult)
{
  udResult result;
  T denom;

  UD_ERROR_NULL(pResult, udR_InvalidParameter);

  denom = udDot(plane.normal, ray.direction);

  //check if ray is parallel to plane
  if (udIsZero(denom))
  {
    pResult->u = T(0);

    //check if ray is on the plane
    if (udIsZero(udGeometry_SignedDistance(plane, ray.origin)))
      pResult->code = udGC_Coincident;
    else
      pResult->code = udGC_NotIntersecting;
  }
  else
  {
    pResult->u = (-(udDot(ray.origin, plane.normal) + plane.offset) / denom);

    //ray points away from plane
    if (pResult->u < T(0))
    {
      pResult->code = udGC_NotIntersecting;
      pResult->u = T(0);
    }
    else
    {
      pResult->code = udGC_Intersecting;
    }
  }

  pResult->point = ray.origin + pResult->u * ray.direction;

  result = udR_Success;
epilogue:
  return result;
}
