#ifndef UDGEOMETRYTOOLS_H
#define UDGEOMETRYTOOLS_H

#include "udMath.h"

//------------------------------------------------------------------------
// Constants and Types
//------------------------------------------------------------------------

// If UD_USE_EXACT_MATH is defined, values are compared for exactness,
// otherwise a tolerance is used

/*
Notes:
 * Use the geomtery type Set() functions to avoid degeneracy.
   A dengenerate class can be thought of as a class of objects that are qualitativily different (and simpler than) the rest of the class.
   For example a point is a degenerate circle with radius 0, a line is degenerate triangle with all colinier points.
 * A query with a degenerate type will have undefined behaviour
*/

enum udGeometryCode
{
  udGC_Success,
  udGC_Overlapping,
  udGC_Parallel,
  udGC_Coincident,
  udGC_Intersecting,
  udGC_NotIntersecting,
  udGC_CompletelyInside,
  udGC_CompletelyOutside,
};

#define VECTOR_T typename udSpace<T, R>::vector_t

template<typename T, int R> struct udSpace;
template<typename T> struct udSpace<T, 2> { typedef udVector2<T> vector_t; };
template<typename T> struct udSpace<T, 3> { typedef udVector3<T> vector_t; };

//--------------------------------------------------------------------------------
// Geometry Types
//--------------------------------------------------------------------------------

template<typename T>
class udPlane
{
public:

  udResult Set(const udVector3<T> &p0, const udVector3<T> &p1, const udVector3<T> &p2);
  udResult Set(const udVector3<T> &point, const udVector3<T> &_normal);

  udVector3<T> normal;
  T offset;
};

template<typename T, int R>
class udAABB
{
public:

  udResult Set(const VECTOR_T &minPt, const VECTOR_T &maxPt);
  void Merge(udAABB const &other);
  VECTOR_T GetCentre() const {return (minPoint + maxPoint) * T(0.5);}

  VECTOR_T minPoint;
  VECTOR_T maxPoint;
};

template<typename T, int R>
class udLine
{
public:

  udResult SetFromEndPoints(const VECTOR_T &p0, const VECTOR_T &p1);
  udResult SetFromDirection(const VECTOR_T &p0, const VECTOR_T &dir);

  VECTOR_T origin;
  VECTOR_T direction;
};

template<typename T, int R>
class udRay
{
public:

  udResult SetFromEndPoints(const VECTOR_T &p0, const VECTOR_T &p1);
  udResult SetFromDirection(const VECTOR_T &p0, const VECTOR_T &dir);

  VECTOR_T origin;
  VECTOR_T direction;
};

template<typename T, int R>
class udSegment
{
public:

  udResult Set(const VECTOR_T &_p0, const VECTOR_T &_p1);
  udResult GetCenteredForm(VECTOR_T *pCentre, VECTOR_T *pDirection, VECTOR_T *pExtent) const;
  VECTOR_T Direction() const { return p1 - p0; }
  T Length() const { return udMag(p1 - p0); }
  T LengthSq() const { return udMagSq(p1 - p0); }

  VECTOR_T p0;
  VECTOR_T p1;
};

template<typename T, int R>
class udHyperSphere
{
public:

  udResult Set(const VECTOR_T & _centre, T _radius);

  VECTOR_T centre;
  T radius;
};

template<typename T, int R>
class udTriangle
{
public:

  udResult Set(const VECTOR_T & _p0, const VECTOR_T & _p1, const VECTOR_T & _p2);

  T GetArea() const;
  udVector3<T> GetSideLengths() const;

  VECTOR_T p0;
  VECTOR_T p1;
  VECTOR_T p2;
};

template <typename T> using udAABB2 = udAABB<T, 2>;
template <typename T> using udAABB3 = udAABB<T, 3>;
template <typename T> using udLine2 = udLine<T, 2>;
template <typename T> using udLine3 = udLine<T, 3>;
template <typename T> using udRay2 = udRay<T, 2>;
template <typename T> using udRay3 = udRay<T, 3>;
template <typename T> using udSegment2 = udSegment<T, 2>;
template <typename T> using udSegment3 = udSegment<T, 3>;
template <typename T> using udTriangle2 = udTriangle<T, 2>;
template <typename T> using udTriangle3 = udTriangle<T, 3>;
template <typename T> using udCircle2 = udHyperSphere<T, 2>;
template <typename T> using udSphere = udHyperSphere<T, 3>;

//--------------------------------------------------------------------------------
// Utility
//--------------------------------------------------------------------------------

template<uint32_t P> constexpr float udQuickPowf(float x) {return x * udQuickPowf<P - 1>(x);}
template<> constexpr float udQuickPowf<0>(float x) {udUnused(x); return 1.f;}

template<uint32_t P> constexpr double udQuickPowd(double x) {return x * udQuickPowd<P - 1>(x);}
template<> constexpr double udQuickPowd<0>(double x) {udUnused(x); return 1.0;}

template<typename T> bool udAreEqual(T a, T b);
template<typename T, int R> bool udAreEqual(const VECTOR_T &v0, const VECTOR_T &v1);

// If UD_USE_EXACT_MATH is not defined, this function tests if value is within an epsilon of zero, as defined in udGetEpsilon().
// Otherwise it will test if value == T(0)
template<typename T> bool udIsZero(T value);

// Utility function to sort two values
template<typename T> void udGeometry_SortLowToHigh(T &a, T &b);

// Utility function to sort vector elements
template<typename T> udVector3<T> udGeometry_SortLowToHigh(const udVector3<T> &a);

// Utility function to sum vector elements
template<typename T, int R> T udGeometry_Sum(const VECTOR_T &v);

// The scalar triple product is defined as ((v x u) . w), which is equivilent to the signed
// volume of the parallelepiped formed by the three vectors u, v and w.
template<typename T> T udGeometry_ScalarTripleProduct(const udVector3<T> &u, const udVector3<T> &v, const udVector3<T> &w);

// Compute the barycentric coordinates of a point wrt a triangle.
template<typename T, int R> udResult udGeometry_Barycentric(const udTriangle<T, R> &tri, const VECTOR_T &p, udVector3<T> *pUVW);

//--------------------------------------------------------------------------------
// Distance Queries
//--------------------------------------------------------------------------------

// Compute the signed distance between a plane and point
template<typename T> T udGeometry_SignedDistance(const udPlane<T> &plane, const udVector3<T> &point);

//--------------------------------------------------------------------------------
// Intersection Test Queries
//--------------------------------------------------------------------------------

template<typename T, int R> udResult udGeometry_TIPointAABB(const VECTOR_T &point, const udAABB<T, R> &box, udGeometryCode *pCode);
template<typename T, int R> udResult udGeometry_TIAABBAABB(const udAABB<T, R> &box0, const udAABB<T, R> &box1, udGeometryCode *pCode);
template<typename T> udResult udGeometry_TI2PointPolygon(const udVector2<T> &point, const udVector2<T> *pPoints, size_t pointCount, udGeometryCode *pCode);

//--------------------------------------------------------------------------------
// Find Intersection Queries
//--------------------------------------------------------------------------------

template<typename T>
struct udFI3SegmentPlaneResult
{
  udGeometryCode code;
  udVector3<T> point;
  T u;
};
template<typename T> udResult udGeometry_FI3SegmentPlane(const udSegment3<T> &seg, const udPlane<T> &plane, udFI3SegmentPlaneResult<T> *pData);

//--------------------------------------------------------------------------------
// Closest Points Queries
//--------------------------------------------------------------------------------

// Find the closest point between a point and a plane
template<typename T> udResult udGeometry_CP3PointPlane(const udVector3<T> &point, const udPlane<T> &plane,  udVector3<T> *pOut);

// Closest point between a point and a line in 3D.
// Returns: udGC_Success
template<typename T, int R>
struct udCPPointLineResult
{
  VECTOR_T point;
  T u;
};
template<typename T, int R> udResult udGeometry_CPPointLine(const VECTOR_T &point, const udLine<T, R> &line, udCPPointLineResult<T, R> *pData);

// Closest point between a point and a line segment in 3D.
// Returns: udGC_Success
template<typename T, int R>
struct udCPPointSegmentResult
{
  VECTOR_T point;
  T u;
};
template<typename T, int R> udResult udGeometry_CPPointSegment(const VECTOR_T &point, const udSegment<T, R> &seg, udCPPointSegmentResult<T, R> *pData);

// Closest point between two line segments in 3D.
// Returns: udGC_Success
//          udGC_Overlapping if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
template<typename T, int R>
struct udCPSegmentSegmentResult
{
  udGeometryCode code;
  VECTOR_T cp_a;
  VECTOR_T cp_b;
  T u_a;
  T u_b;
};
template<typename T, int R> udResult udGeometry_CPSegmentSegment(const udSegment<T, R> & seg_a, const udSegment<T, R> & seg_b, udCPSegmentSegmentResult<T, R> * pResult);

// Closest point between two line segments in 3D.
// Returns: udGC_Success
//          udGC_Overlapping if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
template<typename T, int R>
struct udCPLineLineResult
{
  udGeometryCode code;
  VECTOR_T cp_a;
  VECTOR_T cp_b;
  T u_a;
  T u_b;
};
template<typename T, int R> udResult udGeometry_CPLineLine(const udLine<T, R> & line_a, const udLine<T, R> & line_b, udCPLineLineResult<T, R> *pResult);

template<typename T, int R>
struct udCPLineSegmentResult
{
  udGeometryCode code;
  VECTOR_T cp_l;
  VECTOR_T cp_s;
  T u_l;
  T u_s;
};
template<typename T, int R> udResult udGeometry_CPLineSegment(const udLine<T, R> &line, const udSegment<T, R> &seg, udCPLineSegmentResult<T, R> *pResult);

// Find the closest point on a triangle to a point in 3D space.
template<typename T, int R> udResult udGeometry_CPPointTriangle(const VECTOR_T &point, const udTriangle<T, R> &tri, VECTOR_T *pOut);

//--------------------------------------------------------------------------------
// Find Intersection Queries
//--------------------------------------------------------------------------------

// Intersection test between line segment and triangle.
//
// If the segment does not lie on the plane of the triangle
//   Returns: udGC_Intersecting, intersect0 set
//            udGC_NotIntersecting
//
// If the segment lies on the plane of the triangle
//   Returns: udGC_Intersecting, intersect0 and intersect1 set (intersection will be a segment)
//            udGC_NotIntersectingtemplate<typename T, int R>
template<typename T>
struct udFI3SegmentTriangleResult
{
  udGeometryCode code;
  udVector3<T> point;
};
template<typename T> udResult udGeometry_FI3SegmentTriangle(const udSegment3<T> &seg, const udTriangle3<T> &tri, udFI3SegmentTriangleResult<T> *pResult);

#include "udGeometryTools_Inl.h"

#endif
