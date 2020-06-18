#ifndef UDGEOMETRYTOOLS_H
#define UDGEOMETRYTOOLS_H

#include "udMath.h"

/*
//------------------------------------------------------------------------
   THE GOAL
//------------------------------------------------------------------------

 The purpose of this library is to collect all common and reusable geometry code in a single place
 for convenience and testing. 

 udGeometry values: Correctness, speed and robustness.

//------------------------------------------------------------------------
   GEOMETRY TYPES
//------------------------------------------------------------------------

 * Examples include lines, circles, spheres, planes and boxes.
 * A geometry type will have public access to data members, to follow the convention of other udMath types.
 * To construct a geomtery type, the Set() functions should be used to ensure legitimate construction, and avoid degeneracy.
   A dengenerate class can be thought of as a class of objects that are qualitativily different (and simpler than) the rest of the class.
   For example a point is a degenerate circle with radius 0, a line is degenerate triangle with all colinier points.
 * Any method taking a geometry type can assume it is legitimate

//------------------------------------------------------------------------
   GEOMETRY QUERIES
//------------------------------------------------------------------------

 * There will be three types of geometry queries:
      Closest Points        (udGeometry_CP*): Closest points between two geometries.
      Test for Intersection (udGeometry_TI*): Determine if two geometries are intersecting. No other output may be given other than a
                                              boolean. TI queries are usually faster to perform than FI as we can stop once we
                                              determine intersection state.
      Find Intersection     (udGeometry_FI*): Find the intersection space between two geometries. Can be more intensive then TI querieas as we not
                                              only want to determine if the two geometries are intersecting, but we need to construct the geometry
                                              that defines the intersection space. For example, a line vs triangle FI query in 3D might return nothing,
                                              a point or a segment (line is on the plane of the triangle).
 * A query with a degenerate type will have undefined behaviour
*/

//------------------------------------------------------------------------
// Constants and Types
//------------------------------------------------------------------------

// If UD_USE_EXACT_MATH is defined, values are compared for exactness,
// otherwise a tolerance is used

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
  udGC_OnBoundary,
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
  udResult GetSide(uint32_t i, udSegment<T, R> *pSeg) const;

  union
  {
    struct
    {
      udVector_t p0;
      udVector_t p1;
      udVector_t p2;
    } pt;
    udVector_t ary[3];
  };
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
template <typename T> using udDisk2 = udHyperSphere<T, 2>; // The region bounded by a circle
template <typename T> using udSphere = udHyperSphere<T, 3>;
template <typename T> using udBall = udHyperSphere<T, 3>;  // The region bounded by a sphere

//********************************************************************************
// Utility
//********************************************************************************

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

//********************************************************************************
// Distance Queries
//********************************************************************************

// Compute the signed distance between a plane and point
template<typename T> T udGeometry_SignedDistance(const udPlane<T> &plane, const udVector3<T> &point);

//********************************************************************************
// Intersection Test Queries
//********************************************************************************

//-----------------------------------------
// udGeometry_TIPointAABB()
// Geometry codes: udGC_Intersecting, udGC_NotIntersecting
template<typename T, int R> udResult udGeometry_TIPointAABB(const udVector_t &point, const udAABB<T, R> &box, udGeometryCode *pCode);

//-----------------------------------------
// udGeometry_TIAABBAABB()
// Geometry codes: udGC_Intersecting, udGC_NotIntersecting
template<typename T, int R> udResult udGeometry_TIAABBAABB(const udAABB<T, R> &box0, const udAABB<T, R> &box1, udGeometryCode *pCode);

//-----------------------------------------
// udGeometry_TI2PointPolygon()
// Geometry codes: udGC_CompletelyInside, udGC_CompletelyOutside, udGC_OnBoundary
template<typename T> udResult udGeometry_TI2PointPolygon(const udVector2<T> &point, const udVector2<T> *pPoints, size_t pointCount, udGeometryCode *pCode);

//********************************************************************************
// Closest Points Queries
//********************************************************************************

//-----------------------------------------
// udGeometry_CP3PointPlane()
template<typename T> udResult udGeometry_CP3PointPlane(const udVector3<T> &point, const udPlane<T> &plane,  udVector3<T> *pOut);

//-----------------------------------------
// udGeometry_CPPointLine()
template<typename T, int R>
struct udCPPointLineResult
{
  udVector_t point;
  T u; // Distance along the line to closest point
};
template<typename T, int R> udResult udGeometry_CPPointLine(const udVector_t &point, const udLine<T, R> &line, udCPPointLineResult<T, R> *pData);

//-----------------------------------------
// udGeometry_CPPointSegment()
template<typename T, int R>
struct udCPPointSegmentResult
{
  udVector_t point;
  T u; // Distance along the line to closest point
};
template<typename T, int R> udResult udGeometry_CPPointSegment(const udVector_t &point, const udSegment<T, R> &seg, udCPPointSegmentResult<T, R> *pData);

//-----------------------------------------
// udGeometry_CPSegmentSegment()
// Geometry codes: udGC_Success
//                 udGC_Overlapping if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
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

//-----------------------------------------
// udGeometry_CPLineLine()
// Geometry codes: udGC_Success
//                 udGC_Parallel if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
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

//-----------------------------------------
// udGeometry_CPLineSegment()
// Geometry codes: udGC_Success
//                 udGC_Parallel if the segments are overlapping in a way that produces an infinite number of closest points. In this case, a point is chosen along this region to be the closest points set.
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

//-----------------------------------------
// udGeometry_CPPointTriangle()
template<typename T, int R> udResult udGeometry_CPPointTriangle(const udVector_t &point, const udTriangle<T, R> &tri, udVector_t *pOut);

//********************************************************************************
// Find Intersection Queries
//********************************************************************************

//-----------------------------------------
// udGeometry_FI3SegmentPlane()
// Geometry codes: udGC_Intersecting
//                 udGC_NotIntersecting
//                 udGC_Coincident
template<typename T>
struct udFI3SegmentPlaneResult
{
  udGeometryCode code;
  udVector3<T> point;
  T u;
};
template<typename T> udResult udGeometry_FI3SegmentPlane(const udSegment3<T> & seg, const udPlane<T> & plane, udFI3SegmentPlaneResult<T> * pData);

//-----------------------------------------
// udGeometry_FI3SegmentTriangle()
// Geometry codes: udGC_Intersecting
//                 udGC_NotIntersecting
//                 udGC_Overlapping (produces segment)
template<typename T>
struct udFI3SegmentTriangleResult
{
  udGeometryCode code;
  union
  {
    struct
    {
      udVector3<T> point;
      T u;
    } intersecting;

    struct
    {
      udSegment3<T> segment;
      T u_0;
      T u_1;
    } overlapping;
  };
};
template<typename T> udResult udGeometry_FI3SegmentTriangle(const udSegment3<T> &seg, const udTriangle3<T> &tri, udFI3SegmentTriangleResult<T> *pResult);

//-----------------------------------------
// udGeometry_FI3RayPlane()
// Geometry codes: udGC_Intersecting
//                 udGC_NotIntersecting
//                 udGC_Coincident
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
