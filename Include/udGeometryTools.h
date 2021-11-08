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

// These should be separate form udResult as they are not really error codes, but more describe the interaction between objects
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

// Many geometry queries can use the same algotirhm for both 2D and 3D, so we abstract away the dimension...
#define udVector_t typename udSpace<T, R>::vector_t

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

  udResult Set(const udVector_t &minPt, const udVector_t &maxPt);
  void Merge(udAABB const &other);
  udVector_t GetCentre() const {return (minPoint + maxPoint) * T(0.5);}

  udVector_t minPoint;
  udVector_t maxPoint;
};

template<typename T, int R>
class udLine
{
public:

  udResult SetFromEndPoints(const udVector_t &p0, const udVector_t &p1);
  udResult SetFromDirection(const udVector_t &p0, const udVector_t &dir);

  udVector_t origin;
  udVector_t direction;
};

template<typename T, int R>
class udRay
{
public:

  udResult SetFromEndPoints(const udVector_t &p0, const udVector_t &p1);
  udResult SetFromDirection(const udVector_t &p0, const udVector_t &dir);

  udVector_t origin;
  udVector_t direction;
};

template<typename T, int R>
class udSegment
{
public:

  udResult Set(const udVector_t &_p0, const udVector_t &_p1);
  udResult GetCenteredForm(udVector_t *pCentre, udVector_t *pDirection, udVector_t *pExtent) const;
  udVector_t Direction() const { return p1 - p0; }
  T Length() const { return udMag(p1 - p0); }
  T LengthSq() const { return udMagSq(p1 - p0); }

  udVector_t p0;
  udVector_t p1;
};

template<typename T, int R>
class udHyperSphere
{
public:

  udResult Set(const udVector_t & _centre, T _radius);

  udVector_t centre;
  T radius;
};

template<typename T, int R>
class udTriangle
{
public:

  udResult Set(const udVector_t & _p0, const udVector_t & _p1, const udVector_t & _p2);

  T GetArea() const;
  udVector3<T> GetSideLengths() const;

  udVector_t p0;
  udVector_t p1;
  udVector_t p2;
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
template<typename T, int R> bool udAreEqual(const udVector_t &v0, const udVector_t &v1);

// If UD_USE_EXACT_MATH is not defined, this function tests if value is within an epsilon of zero, as defined in udGetEpsilon().
// Otherwise it will test if value == T(0)
template<typename T> bool udIsZero(T value);

// Utility function to sort two values
template<typename T> void udGeometry_SortLowToHigh(T &a, T &b);

// Utility function to sort vector elements
template<typename T> udVector3<T> udGeometry_SortLowToHigh(const udVector3<T> &a);

// Utility function to sum vector elements
template<typename T, int R> T udGeometry_Sum(const udVector_t &v);

// The scalar triple product is defined as ((v x u) . w), which is equivilent to the signed
// volume of the parallelepiped formed by the three vectors u, v and w.
template<typename T> T udGeometry_ScalarTripleProduct(const udVector3<T> &u, const udVector3<T> &v, const udVector3<T> &w);

// Compute the barycentric coordinates of a point wrt a triangle.
template<typename T, int R> udResult udGeometry_Barycentric(const udTriangle<T, R> &tri, const udVector_t &p, udVector3<T> *pUVW);

template<typename T> udResult SetRotationAround(const udRay3<T> & ray, const udVector3<T> & center, const udVector3<T> & axis, const T & angle, udRay3<T> *pOut);

//--------------------------------------------------------------------------------
// Distance Queries
//--------------------------------------------------------------------------------

// Compute the signed distance between a plane and point
template<typename T> T udGeometry_SignedDistance(const udPlane<T> &plane, const udVector3<T> &point);

//--------------------------------------------------------------------------------
// Intersection Test Queries
//--------------------------------------------------------------------------------

template<typename T, int R> udResult udGeometry_TIPointAABB(const udVector_t &point, const udAABB<T, R> &box, udGeometryCode *pCode);
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
  udVector_t point;
  T u;
};
template<typename T, int R> udResult udGeometry_CPPointLine(const udVector_t &point, const udLine<T, R> &line, udCPPointLineResult<T, R> *pData);

// Closest point between a point and a line segment in 3D.
// Returns: udGC_Success
template<typename T, int R>
struct udCPPointSegmentResult
{
  udVector_t point;
  T u;
};
template<typename T, int R> udResult udGeometry_CPPointSegment(const udVector_t &point, const udSegment<T, R> &seg, udCPPointSegmentResult<T, R> *pData);

// Closest point between two line segments in 3D.
// Returns: udGC_Success
//          udGC_Overlapping if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
template<typename T, int R>
struct udCPSegmentSegmentResult
{
  udGeometryCode code;
  udVector_t cp_a;
  udVector_t cp_b;
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
  udVector_t cp_a;
  udVector_t cp_b;
  T u_a;
  T u_b;
};
template<typename T, int R> udResult udGeometry_CPLineLine(const udLine<T, R> & line_a, const udLine<T, R> & line_b, udCPLineLineResult<T, R> *pResult);

template<typename T, int R>
struct udCPLineSegmentResult
{
  udGeometryCode code;
  udVector_t cp_l;
  udVector_t cp_s;
  T u_l;
  T u_s;
};
template<typename T, int R> udResult udGeometry_CPLineSegment(const udLine<T, R> &line, const udSegment<T, R> &seg, udCPLineSegmentResult<T, R> *pResult);

// Find the closest point on a triangle to a point in 3D space.
template<typename T, int R> udResult udGeometry_CPPointTriangle(const udVector_t &point, const udTriangle<T, R> &tri, udVector_t *pOut);

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

template<typename T>
struct udFI3RayPlaneResult
{
  udGeometryCode code;
  udVector3<T> point;
  T u;
};
template<typename T> udResult udGeometry_FI3RayPlane(const udRay3<T> & ray, const udPlane<T> & plane, udFI3RayPlaneResult<T> * pResult);

#include "udGeometryTools_Inl.h"

#endif
