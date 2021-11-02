#include "udGeometryTools.h"
#include "gtest/gtest.h"
#include "udPlatform.h"

#define EXPECT_VEC3_NEAR(v0, v1, e) EXPECT_NEAR(v0.x, v1.x, e);\
EXPECT_NEAR(v0.y, v1.y, e);\
EXPECT_NEAR(v0.z, v1.z, e)

TEST(GeometryTests, GeometryUtility)
{
  double a = 2.0, b = 1.0;
  udGeometry_SortLowToHigh(a, b);
  EXPECT_EQ(a, 1.0);
  EXPECT_EQ(b, 2.0);
  
  udInt3 arr = {3, 2, 1};
  EXPECT_EQ(udGeometry_SortLowToHigh(arr), udInt3::create(1, 2, 3));
  EXPECT_EQ((udGeometry_Sum<int32_t, 3>(arr)), 6);
}

TEST(GeometryTests, GeometryConstruction)
{
  udPlane<double> plane = {};
  EXPECT_EQ(plane.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(plane.Set({1.0, 2.0, 3.0}, {0.0, 0.0, 0.0}), udR_Failure);

  EXPECT_EQ(plane.Set({1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}), udR_Success);
  EXPECT_EQ(plane.Set({1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}), udR_Failure);

  udAABB3<double> aabb = {};
  EXPECT_EQ(aabb.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(aabb.Set({1.0, 2.1, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);

  udSegment3<double> seg = {};
  EXPECT_EQ(seg.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(seg.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);

  udLine3<double> line = {};
  EXPECT_EQ(line.SetFromEndPoints({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(line.SetFromEndPoints({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);

  EXPECT_EQ(line.SetFromDirection({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(line.SetFromDirection({1.0, 2.0, 3.0}, {0.0, 0.0, 0.0}), udR_Failure);

  udRay3<double> ray = {};
  EXPECT_EQ(ray.SetFromEndPoints({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(ray.SetFromEndPoints({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);

  EXPECT_EQ(ray.SetFromDirection({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Success);
  EXPECT_EQ(ray.SetFromDirection({1.0, 2.0, 3.0}, {0.0, 0.0, 0.0}), udR_Failure);

  udTriangle3<double> tri = {};
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}, {1.0, 2.0, 3.2}), udR_Success);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}, {1.0, 2.0, 3.0}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.1}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);
}
TEST(GeometryTests, GeometryLines)
{
  //point vs line
  {
    udLine3<double> line;
    EXPECT_EQ(line.SetFromDirection({1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}), udR_Success);
    CPPointLineResult<double, 3> cpResult = {};
    udDouble3 point = {};

    //Point lies 'before' line origin
    point = {-3.0, 1.0, 2.0};
    EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.u, -4.0);
    EXPECT_EQ(cpResult.point, udDouble3::create(-3.0, 1.0, 1.0));

    //Point lies perpendicular to line origin
    point = {1.0, 1.0, 2.0};
    EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.u, 0.0);
    EXPECT_EQ(cpResult.point, udDouble3::create(1.0, 1.0, 1.0));

    //Point lies 'after' line origin
    point = {7.0, 1.0, 2.0};
    EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.u, 6.0);
    EXPECT_EQ(cpResult.point, udDouble3::create(7.0, 1.0, 1.0));
  }

  //point v segment
  {
    udSegment<double, 3> seg = {};
    seg.p0 = {1.0, 1.0, 1.0};
    seg.p1 = {3.0, 1.0, 1.0};
    CPPointSegmentResult<double, 3> cpPointSegmentResult = {};
    udDouble3 point = {};

    //Point lies 'before' segment 0
    point = {-1.0, 1.0, 1.0};
    EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
    EXPECT_EQ(cpPointSegmentResult.u, 0.0);
    EXPECT_EQ(cpPointSegmentResult.point, seg.p0);

    //Point lies 'after' segment 1
    point = {5.0, 1.0, 1.0};
    EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
    EXPECT_EQ(cpPointSegmentResult.u, 1.0);
    EXPECT_EQ(cpPointSegmentResult.point, seg.p1);

    //Point lies along the segment line
    point = {2.0, 10.0, 42.0};
    EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
    EXPECT_EQ(cpPointSegmentResult.u, 0.5);
    EXPECT_EQ(cpPointSegmentResult.point, udDouble3::create(2.0, 1.0, 1.0));
  }

  //segment v segment
  {
    udSegment<double, 3> seg_a = {};
    udSegment<double, 3> seg_b = {};
    CPSegmentSegmentResult<double, 3> cpResult = {};

    //Segments have zero length
    seg_a.p0 = seg_b.p0 = {1.0, 4.0, 12.0};
    seg_a.p1 = seg_b.p1 = {1.0, 4.0, 12.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    seg_a.p0 = {2.0, 0.0, 0.0};
    seg_a.p1 = {6.0, 0.0, 0.0};

    //LineSegs parallel, no overlap, closest points a0, seg_b.p0
    seg_b.p0 = {-1.0, -4.0, 12.0};
    seg_b.p1 = {-5.0, -4.0, 12.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs parallel, no overlap, closest points a0, seg_b.p1
    seg_b.p0 = {-5.0, -4.0, 12.0};
    seg_b.p1 = {-1.0, -4.0, 12.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 1.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, seg_b.p1);

    //LineSegs parallel, no overlap, closest points a1, seg_b.p0
    seg_b.p0 = {9.0, -4.0, 12.0};
    seg_b.p1 = {18.0, -4.0, 12.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 1.0);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p1);
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs parallel, no overlap, closest points a1, seg_b.p1
    seg_b.p0 = {10.0, -4.0, 12.0};
    seg_b.p1 = {9.0, -4.0, 12.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 1.0);
    EXPECT_EQ(cpResult.u_b, 1.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p1);
    EXPECT_EQ(cpResult.cp_b, seg_b.p1);

    //LineSegs parallel, overlap, a0---seg_b.p0---a1---seg_b.p1
    seg_b.p0 = {4.0, -3.0, 4.0};
    seg_b.p1 = {10.0, -3.0, 4.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    //Why udQCOverlapping and not udQC_Parallel? Because overlapping segments are already parallel.
    EXPECT_EQ(cpResult.code, udGC_Overlapping);
    EXPECT_EQ(cpResult.u_a, 0.5);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs parallel, overlap, a1---seg_b.p0---a0---seg_b.p1
    seg_b.p0 = {4.0, -3.0, 4.0};
    seg_b.p1 = {0.0, -3.0, 4.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Overlapping);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 0.5);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, udDouble3::create(2.0, -3.0, 4.0));

    //LineSegs parallel, overlap, a0---seg_b.p0---seg_b.p1---a1
    seg_b.p0 = {4.0, -3.0, 4.0};
    seg_b.p1 = {5.0, -3.0, 4.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Overlapping);
    EXPECT_EQ(cpResult.u_a, 0.5);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs parallel, overlap, a1---seg_b.p0---seg_b.p1---a0
    seg_b.p0 = {4.0, -3.0, 4.0};
    seg_b.p1 = {8.0, -3.0, 4.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Overlapping);
    EXPECT_EQ(cpResult.u_a, 0.5);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs not parallel, closest points: a0, seg_b.p0
    seg_b.p0 = {-2.0, -5.0, 20.0};
    seg_b.p1 = {-2.0, -5.0, 23.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs not parallel, closest points: a0, seg_b.p1
    seg_b.p0 = {-2.0, -5.0, 23.0};
    seg_b.p1 = {-2.0, -5.0, 20.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 1.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, seg_b.p1);

    //LineSegs not parallel, closest points: a1, seg_b.p0
    seg_b.p0 = {10.0, -5.0, 20.0};
    seg_b.p1 = {10.0, -5.0, 23.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 1.0);
    EXPECT_EQ(cpResult.u_b, 0.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p1);
    EXPECT_EQ(cpResult.cp_b, seg_b.p0);

    //LineSegs not parallel, closest points: a1, seg_b.p1
    seg_b.p0 = {10.0, -5.0, 23.0};
    seg_b.p1 = {10.0, -5.0, 20.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 1.0);
    EXPECT_EQ(cpResult.u_b, 1.0);
    EXPECT_EQ(cpResult.cp_a, seg_a.p1);
    EXPECT_EQ(cpResult.cp_b, seg_b.p1);

    //LineSegs not parallel, closest points: a0, ls1-along ls
    seg_b.p0 = {-1.0, 4.0, -3.0};
    seg_b.p1 = {-1.0, 4.0, 3.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.0);
    EXPECT_EQ(cpResult.u_b, 0.5);
    EXPECT_EQ(cpResult.cp_a, seg_a.p0);
    EXPECT_EQ(cpResult.cp_b, udDouble3::create(-1.0, 4.0, 0.0));

    //LineSegs not parallel, closest points: a1, ls1-along ls
    seg_b.p0 = {9.0, 4.0, -3.0};
    seg_b.p1 = {9.0, 4.0, 3.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 1.0);
    EXPECT_EQ(cpResult.u_b, 0.5);
    EXPECT_EQ(cpResult.cp_a, seg_a.p1);
    EXPECT_EQ(cpResult.cp_b, udDouble3::create(9.0, 4.0, 0.0));

    //LineSegs not parallel, closest points: ls0-along ls, ls1-along ls
    seg_b.p0 = {4.0, 4.0, -3.0};
    seg_b.p1 = {4.0, -4.0, -3.0};
    EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
    EXPECT_EQ(cpResult.code, udGC_Success);
    EXPECT_EQ(cpResult.u_a, 0.5);
    EXPECT_EQ(cpResult.u_b, 0.5);
    EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpResult.cp_b, udDouble3::create(4.0, 0.0, -3.0));
  }
}

TEST(GeometryTests, GeometryEquivalence)
{
  EXPECT_TRUE(udAreEqual<double>(1.0, 1.0));
  EXPECT_TRUE(udAreEqual<double>(1.0, 1.0 + udGetEpsilon<double>() * 0.5));

  EXPECT_FALSE(udAreEqual<double>(2.0, 3.0));
  EXPECT_FALSE(udAreEqual<double>(2.0, 2.0 + udGetEpsilon<double>() * 1.5));

  udDouble3 a = {1.0, 2.0, 3.0};
  udDouble3 b = {1.01, 2.01, 3.01};

  EXPECT_TRUE((udAreEqual<double, 3>(a, a)));
  EXPECT_FALSE((udAreEqual<double, 3>(a, b)));
}

bool CheckBarycentricResult(udTriangle<double, 3> tri, udDouble3 p, udDouble3 uvw)
{
  double epsilon = 0.01;
  udDouble3 out = {};
  udResult result = udGeometry_Barycentric(tri, p, &out);
  if (result != udR_Success)
    return false;

  return udMag(uvw - out) < epsilon;
}

//Compared with results from https: //www.geogebra.org/m/ZuvmPjmy
TEST(GeometryTests, GeometryBaryCentric)
{
  double a = 1.0;
  double b = udSqrt(3.0) / 2.0;
  double c = 0.5;

  udDouble3 t0 = {0.0, a, 0.0};
  udDouble3 t1 = {b, -c, 0.0};
  udDouble3 t2 = {-b, -c, 0.0};

  udTriangle<double, 3> tri = {t0, t1, t2};

  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 0.0, 0.0}, {0.33, 0.33, 0.33}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 0.0, 345.0}, {0.33, 0.33, 0.33}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 0.0, -327}, {0.33, 0.33, 0.33}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {1.0, 1.0, 0.0}, {1.0, 0.58, -0.58}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {1.0, 1.0, 345.0}, {1.0, 0.58, -0.58}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {1.0, 1.0, -327}, {1.0, 0.58, -0.58}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 1.0, 345.0}, {1.0, 0.0, 0.0}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 1.0, -327}, {1.0, 0.0, 0.0}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 2.0, 0.0}, {1.67, -0.33, -0.33}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 2.0, 345.0}, {1.67, -0.33, -0.33}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, 2.0, -327}, {1.67, -0.33, -0.33}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {-1.0, 1.0, 0.0}, {1.0, -0.58, 0.58}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {-1.0, 1.0, 345.0}, {1.0, -0.58, 0.58}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {-1.0, 1.0, -327}, {1.0, -0.58, 0.58}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, -1.0, 0.0}, {-0.33, 0.67, 0.67}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, -1.0, 345.0}, {-0.33, 0.67, 0.67}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.0, -1.0, -327}, {-0.33, 0.67, 0.67}));

  EXPECT_TRUE(CheckBarycentricResult(tri, {0.2, 0.2, 0.0}, {0.47, 0.38, 0.15}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.2, 0.2, 345.0}, {0.47, 0.38, 0.15}));
  EXPECT_TRUE(CheckBarycentricResult(tri, {0.2, 0.2, -327}, {0.47, 0.38, 0.15}));
}

bool AreEqual(udPlane<double> p0, udPlane<double> p1)
{
  return udAreEqual<double, 3>(p0.normal, p1.normal) && udAreEqual(p0.offset, p1.offset);
}

TEST(GeometryTests, Planes)
{
  typedef udPlane<double> udDoublePlane;
  udDoublePlane plane0 = {};
  udDoublePlane plane1 = {};

  // Creation
  {
    EXPECT_EQ(plane0.Set({1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}), udR_Success);
    plane1.normal = {1.0, 0.0, 0.0};
    plane1.offset = -1.0;
    EXPECT_TRUE(AreEqual(plane0, plane1));

    EXPECT_EQ(plane0.Set({1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}), udR_Success);
    EXPECT_TRUE(AreEqual(plane0, plane1));

    EXPECT_EQ(plane0.Set({1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {3.0, 0.0, 0.0}), udR_Failure);
  }

  // Distance fom point to plane
  {
    plane0.Set({1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0});

    EXPECT_EQ(udGeometry_SignedDistance(plane0, {42.0, 1.0, 1.0}), 41.0);
    EXPECT_EQ(udGeometry_SignedDistance(plane0, {-42.0, 1.0, 1.0}), -43.0);
    EXPECT_EQ(udGeometry_SignedDistance(plane0, {1.0, 1.0, 1.0}), .0);
  }
}

// TODO this needs many more tests!
TEST(GeometryTests, CPPointTriangle)
{
  udDouble3 cp = {};
  udTriangle<double, 3> tri = {};
  udDouble3 point = {2.0, 0.0, 0.5};

  tri.p0 = {1.0, -1.0, -1.0};
  tri.p1 = {1.0, 1.0, -1.0};
  tri.p2 = {1.0, 0.0, 10.0};

  EXPECT_EQ(udGeometry_CPPointTriangle(point, tri, &cp), udR_Success);
  EXPECT_EQ(cp, udDouble3::create(1.0, 0.0, 0.5));
}

TEST(GeometryTests, GeometrySegmentTriangle)
{
  udTriangle3<double> tri = {};
  udSegment3<double> seg = {};
  FI3SegmentTriangleResult<double> data = {};

  //------------------------------------------------------------------------
  // Intersecting
  //------------------------------------------------------------------------
  tri.p0 = {0.0, -1.0, 0.0};
  tri.p1 = {-1.0, 1.0, 0.0};
  tri.p2 = {1.0, 1.0, 0.0};
  seg.p0 = {0.5, 0.5, 1.0};
  seg.p1 = {0.5, 0.5, -1.0};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.5, 0.5, 0.0))));

  tri.p0 = {0.0, -1.0, 0.0};
  tri.p1 = {1.0, 1.0, 0.0};
  tri.p2 = {-1.0, 1.0, 0.0};
  seg.p0 = {0.5, 0.5, 1.0};
  seg.p1 = {0.5, 0.5, -1.0};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.5, 0.5, 0.0))));

  tri.p0 = {0.0, -1.0, 0.0};
  tri.p1 = {-1.0, 1.0, 0.0};
  tri.p2 = {1.0, 1.0, 0.0};
  seg.p0 = {0.5, 0.5, -1.0};
  seg.p1 = {0.5, 0.5, 1.0};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.5, 0.5, 0.0))));

  tri.p0 = {0.0, -1.0, 0.0};
  tri.p1 = {1.0, 1.0, 0.0};
  tri.p2 = {-1.0, 1.0, 0.0};
  seg.p0 = {0.5, 0.5, -1.0};
  seg.p1 = {0.5, 0.5, 1.0};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.5, 0.5, 0.0))));

  tri.p0 = {0.0, 0.0, -1.0};
  tri.p1 = {0.0, -1.0, 1.0};
  tri.p2 = {0.0, 1.0, 1.0};
  seg.p0 = {-2.0, -0.25, 0.25};
  seg.p1 = {2.0, -0.25, 0.25};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.0, -0.25, 0.25))));

  tri.p0 = {0.0, 0.0, -1.0};
  tri.p1 = {0.0, 1.0, 1.0};
  tri.p2 = {0.0, -1.0, 1.0};
  seg.p0 = {-2.0, -0.25, 0.25};
  seg.p1 = {2.0, -0.25, 0.25};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.0, -0.25, 0.25))));

  tri.p0 = {0.0, 0.0, -1.0};
  tri.p1 = {0.0, -1.0, 1.0};
  tri.p2 = {0.0, 1.0, 1.0};
  seg.p0 = {2.0, -0.25, 0.25};
  seg.p1 = {-2.0, -0.25, 0.25};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.0, -0.25, 0.25))));

  tri.p0 = {0.0, 0.0, -1.0};
  tri.p1 = {0.0, 1.0, 1.0};
  tri.p2 = {0.0, -1.0, 1.0};
  seg.p0 = {2.0, -0.25, 0.25};
  seg.p1 = {-2.0, -0.25, 0.25};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_Intersecting);
  EXPECT_TRUE((udAreEqual<double, 3>(data.point, udDouble3::create(0.0, -0.25, 0.25))));

  //------------------------------------------------------------------------
  // Non intersecting
  //------------------------------------------------------------------------
  tri.p0 = {0.0, -1.0, 0.0};
  tri.p1 = {-1.0, 1.0, 0.0};
  tri.p2 = {1.0, 1.0, 0.0};
  seg.p0 = {10.0, 0.5, 1.0};
  seg.p1 = {10.0, 0.5, -1.0};
  EXPECT_EQ(udGeometry_FI3SegmentTriangle(seg, tri, &data), udR_Success);
  EXPECT_EQ(data.code, udGC_NotIntersecting);

}

TEST(GeometryTests, GeometryTrianglesGeneral)
{
  {
    udDouble3 t0 = {1.0, 0.0, 0.0};
    udDouble3 t1 = {udCos(120.0 / 180.0 * UD_PI), udSin(120.0 / 180.0 * UD_PI), 0.0};
    udDouble3 t2 = {udCos(240.0 / 180.0 * UD_PI), udSin(240.0 / 180.0 * UD_PI), 0.0};
    double side = 2.0 * udCos(30.0 / 180.0 * UD_PI);
    double area = udSqrt(3.0) * side * side / 4.0;

    udTriangle3<double> tri = {t0, t1, t2};

    udDouble3 sideLengths = tri.GetSideLengths();
    EXPECT_TRUE((udAreEqual<double, 3>(sideLengths, udDouble3::create(side, side, side))));

    EXPECT_TRUE((udAreEqual<double>(tri.GetArea(), area)));
    EXPECT_TRUE((udAreEqual<double>(tri.GetArea(), area)));
  }

  {
    udDouble2 t0 = {1.0, 0.0};
    udDouble2 t1 = {udCos(120.0 / 180.0 * UD_PI), udSin(120.0 / 180.0 * UD_PI)};
    udDouble2 t2 = {udCos(240.0 / 180.0 * UD_PI), udSin(240.0 / 180.0 * UD_PI)};
    double side = 2.0 * udCos(30.0 / 180.0 * UD_PI);
    double area = udSqrt(3.0) * side * side / 4.0;

    udTriangle2<double> tri = {t0, t1, t2};

    udDouble3 sideLengths = tri.GetSideLengths();
    EXPECT_TRUE((udAreEqual<double, 3>(sideLengths, udDouble3::create(side, side, side))));

    EXPECT_TRUE((udAreEqual<double>(tri.GetArea(), area)));
    EXPECT_TRUE((udAreEqual<double>(tri.GetArea(), area)));
  }
}

TEST(GeometryTests, GeometryAABB)
{
  // Point vs AABB
  {
    udAABB3<double> aabb = {};
    udGeometryCode code = udGC_Success;
    EXPECT_EQ(aabb.Set({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}), udR_Success);

    EXPECT_EQ(udGeometry_TIPointAABB({0.0, 0.0, 0.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_Intersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({-2.0, 0.0, 0.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({2.0, 0.0, 0.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({0.0, -2.0, 0.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({0.0, 2.0, 0.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({0.0, 0.0, -2.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(udGeometry_TIPointAABB({0.0, 0.0, 2.0}, aabb, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);
  }

  // AABB vs AABB
  {
    udAABB3<double> aabb0 = {};
    udAABB3<double> aabb1 = {};
    udGeometryCode code = udGC_Success;

    EXPECT_EQ(aabb0.Set({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}), udR_Success);
    EXPECT_EQ(aabb1.Set({2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}), udR_Success);

    EXPECT_EQ(udGeometry_TIAABBAABB(aabb0, aabb1, &code), udR_Success);
    EXPECT_EQ(code, udGC_NotIntersecting);

    EXPECT_EQ(aabb1.Set({1.0, 0.0, 0.0}, {3.0, 3.0, 3.0}), udR_Success);
    EXPECT_EQ(udGeometry_TIAABBAABB(aabb0, aabb1, &code), udR_Success);
    EXPECT_EQ(code, udGC_Intersecting);

    EXPECT_EQ(aabb1.Set({0.0, 0.0, 0.0}, {3.0, 3.0, 3.0}), udR_Success);
    EXPECT_EQ(udGeometry_TIAABBAABB(aabb0, aabb1, &code), udR_Success);
    EXPECT_EQ(code, udGC_Intersecting);
  }
}
