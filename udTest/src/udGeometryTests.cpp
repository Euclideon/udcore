#include "udGeometryTools.h"
#include "gtest/gtest.h"
#include "udPlatform.h"

#define EXPECT_VEC3_NEAR(v0, v1, e) EXPECT_NEAR(v0.x, v1.x, e);\
EXPECT_NEAR(v0.y, v1.y, e);\
EXPECT_NEAR(v0.z, v1.z, e)

TEST(GeometryTests, GeometryUtility)
{
  EXPECT_EQ(udQuickPowd<4>(2.0), 16.0);
  EXPECT_EQ(udQuickPowf<16>(2.0), 65536.f);

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
  
  udSphere<double> sphere = {};
  EXPECT_EQ(sphere.Set({0.0, 0.0, 0.0}, 1.0), udR_Success);
  EXPECT_EQ(sphere.Set({0.0, 0.0, 0.0}, 0.0), udR_Failure);
  EXPECT_EQ(sphere.Set({0.0, 0.0, 0.0}, -1.0), udR_Failure);

  udTriangle3<double> tri = {};
  EXPECT_EQ(tri.Set({1.1, 2.0, 3.0}, {1.0, 2.1, 3.1}, {1.0, 2.0, 3.2}), udR_Success);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}, {1.0, 2.0, 3.2}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.0}, {1.0, 2.0, 3.1}, {1.0, 2.0, 3.0}), udR_Failure);
  EXPECT_EQ(tri.Set({1.0, 2.0, 3.1}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), udR_Failure);
}

TEST(GeometryTests, Query_Ray_Plane)
{
  udFI3RayPlaneResult<double> result = {};
  udPlane<double> plane = {};
  udRay3<double> ray = {};

  EXPECT_EQ(ray.SetFromDirection({1.0, 2.0, 3.0}, {1.0, 0.0, 0.0}), udR_Success);

  //Ray not intersecting plane, pointing away from plane
  EXPECT_EQ(plane.Set({-1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3RayPlane(ray, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);
  EXPECT_EQ(result.u, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(result.point, ray.origin)));

  //Ray not intersecting plane, parallel to plane
  EXPECT_EQ(plane.Set({0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3RayPlane(ray, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);
  EXPECT_EQ(result.u, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(result.point, ray.origin)));

  //Ray lies on the plane
  EXPECT_EQ(plane.Set({1.0, 2.0, 3.0}, {0.0, 1.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3RayPlane(ray, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Coincident);
  EXPECT_EQ(result.u, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(result.point, ray.origin)));

  //Ray intersects plane
  EXPECT_EQ(plane.Set({3.0, 0.0, 0.0}, {1.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3RayPlane(ray, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Intersecting);
  EXPECT_EQ(result.u, 2.0);
  EXPECT_TRUE((udAreEqual<double, 3>(result.point, udDouble3::create(3.0, 2.0, 3.0))));

  //Ray intersects plane
  EXPECT_EQ(plane.Set({6.0, 2.3, -5.6}, {-1.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3RayPlane(ray, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Intersecting);
  EXPECT_EQ(result.u, 5.0);
  EXPECT_TRUE((udAreEqual<double, 3>(result.point, udDouble3::create(6.0, 2.0, 3.0))));
}
TEST(GeometryTests, Query_Segment_Plane)
{
  udFI3SegmentPlaneResult<double> result = {};
  udPlane<double> plane = {};
  udSegment3<double> seg = {};

  EXPECT_EQ(plane.Set({0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}), udR_Success);

  EXPECT_EQ(seg.Set({0.0, 0.0, -2.0}, {0.0, 0.0, -1.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);

  EXPECT_EQ(seg.Set({0.0, 0.0, -1.0}, {0.0, 0.0, -2.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);

  EXPECT_EQ(seg.Set({0.0, 0.0, 2.0}, {0.0, 0.0, 3.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);

  EXPECT_EQ(seg.Set({0.0, 0.0, 3.0}, {0.0, 0.0, 2.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_NotIntersecting);

  EXPECT_EQ(seg.Set({0.0, 0.0, 0.0}, {0.0, 0.0, 2.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Intersecting);
  EXPECT_EQ(result.u, 0.5);

  EXPECT_EQ(seg.Set({0.0, 0.0, 2.0}, {0.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Intersecting);
  EXPECT_EQ(result.u, 0.5);

  EXPECT_EQ(seg.Set({1.0, 1.0, 1.0}, {2.0, 2.0, 1.0}), udR_Success);
  EXPECT_EQ(udGeometry_FI3SegmentPlane(seg, plane, &result), udR_Success);
  EXPECT_EQ(result.code, udGC_Overlapping);
  EXPECT_EQ(result.u, 0.0);
  EXPECT_EQ(result.point, seg.p0);
}

TEST(GeometryTests, Query_Point_Line)
{
  udLine3<double> line;
  EXPECT_EQ(line.SetFromDirection({1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}), udR_Success);
  udCPPointLineResult<double, 3> cpResult ={};
  udDouble3 point ={};

  //Point lies 'before' line origin
  point ={-3.0, 1.0, 2.0};
  EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.u, -4.0);
  EXPECT_EQ(cpResult.point, udDouble3::create(-3.0, 1.0, 1.0));

  //Point lies perpendicular to line origin
  point ={1.0, 1.0, 2.0};
  EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.u, 0.0);
  EXPECT_EQ(cpResult.point, udDouble3::create(1.0, 1.0, 1.0));

  //Point lies 'after' line origin
  point ={7.0, 1.0, 2.0};
  EXPECT_EQ(udGeometry_CPPointLine(point, line, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.u, 6.0);
  EXPECT_EQ(cpResult.point, udDouble3::create(7.0, 1.0, 1.0));
}

TEST(GeometryTests, Query_Line_Line)
{
  udLine3<double> line_a ={};
  udLine3<double> line_b ={};
  udCPLineLineResult<double, 3> cpResult ={};

  EXPECT_EQ(line_a.SetFromDirection({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}), udR_Success);

  //Lines coincident
  EXPECT_EQ(line_b.SetFromDirection({42.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_CPLineLine(line_a, line_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Coincident);
  EXPECT_EQ(cpResult.cp_a, line_a.origin);
  EXPECT_EQ(cpResult.cp_b, line_a.origin);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 42.0);

  //Lines parallel
  EXPECT_EQ(line_b.SetFromDirection({42.0, 1.0, 1.0}, {1.0, 0.0, 0.0}), udR_Success);
  EXPECT_EQ(udGeometry_CPLineLine(line_a, line_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Parallel);
  EXPECT_EQ(cpResult.cp_a, line_a.origin);
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(0.0, 1.0, 1.0));
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, -42.0);

  //Lines not parallel
  EXPECT_EQ(line_b.SetFromDirection({42.0, 0.0, 1.0}, {0.0, 0.0, 1.0}), udR_Success);
  EXPECT_EQ(udGeometry_CPLineLine(line_a, line_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.cp_a, udDouble3::create(42.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(42.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.u_a, 42.0);
  EXPECT_EQ(cpResult.u_b, -1.0);
}

TEST(GeometryTests, Query_Line_Segment)
{
  udCPLineSegmentResult<double, 3> cpLineSegmentResult;
  udSegment<double, 3> seg ={};
  udLine3<double> line;

  seg.Set({2.0, 0.0, 0.0}, {6.0, 0.0, 0.0});

  //Segment and line coincident
  line.SetFromDirection({13.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Coincident);
  EXPECT_EQ(cpLineSegmentResult.u_l, -11.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, seg.p0)));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(2.0, 0.0, 0.0))));

  //Segment parallel to line
  line.SetFromDirection({3.0, 3.0, 4.0}, {1.0, 0.0, 0.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Parallel);
  EXPECT_EQ(cpLineSegmentResult.u_l, -1.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, seg.p0)));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(2.0, 3.0, 4.0))));

  //Segment parallel to line, opposite direction
  line.SetFromDirection({3.0, 3.0, 4.0}, {-1.0, 0.0, 0.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Parallel);
  EXPECT_EQ(cpLineSegmentResult.u_l, 1.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, seg.p0)));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(2.0, 3.0, 4.0))));

  //Segment-p0 closest point
  line.SetFromDirection({-1.0, 4.0, 3.0}, {0.0, 0.0, -1.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Success);
  EXPECT_EQ(cpLineSegmentResult.u_l, 3.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 0.0);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, seg.p0)));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(-1.0, 4.0, 0.0))));

  //Segment-p1 closest point
  line.SetFromDirection({9.0, 4.0, 3.0}, {0.0, 0.0, -1.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Success);
  EXPECT_EQ(cpLineSegmentResult.u_l, 3.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 1.0);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, seg.p1)));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(9.0, 4.0, 0.0))));

  //Closest point along Segment
  line.SetFromDirection({3.0, 4.0, 3.0}, {0.0, 0.0, 1.0});
  EXPECT_EQ(udGeometry_CPLineSegment(line, seg, &cpLineSegmentResult), udR_Success);
  EXPECT_EQ(cpLineSegmentResult.code, udGC_Success);
  EXPECT_EQ(cpLineSegmentResult.u_l, -3.0);
  EXPECT_EQ(cpLineSegmentResult.u_s, 0.25);
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_s, udDouble3::create(3.0, 0.0, 0.0))));
  EXPECT_TRUE((udAreEqual<double, 3>(cpLineSegmentResult.cp_l, udDouble3::create(3.0, 4.0, 0.0))));
}

TEST(GeometryTests, Query_Point_Segment)
{
  udSegment<double, 3> seg ={};
  seg.p0 ={1.0, 1.0, 1.0};
  seg.p1 ={3.0, 1.0, 1.0};
  udCPPointSegmentResult<double, 3> cpPointSegmentResult ={};
  udDouble3 point ={};

  //Point lies 'before' segment 0
  point ={-1.0, 1.0, 1.0};
  EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
  EXPECT_EQ(cpPointSegmentResult.u, 0.0);
  EXPECT_EQ(cpPointSegmentResult.point, seg.p0);

  //Point lies 'after' segment 1
  point ={5.0, 1.0, 1.0};
  EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
  EXPECT_EQ(cpPointSegmentResult.u, 1.0);
  EXPECT_EQ(cpPointSegmentResult.point, seg.p1);

  //Point lies along the segment line
  point ={2.0, 10.0, 42.0};
  EXPECT_EQ(udGeometry_CPPointSegment(point, seg, &cpPointSegmentResult), udR_Success);
  EXPECT_EQ(cpPointSegmentResult.u, 0.5);
  EXPECT_EQ(cpPointSegmentResult.point, udDouble3::create(2.0, 1.0, 1.0));
}

TEST(GeometryTests, Query_Segment_Segment)
{
  udSegment<double, 3> seg_a = {};
  udSegment<double, 3> seg_b = {};
  udCPSegmentSegmentResult<double, 3> cpResult = {};

  seg_a.p0 = {2.0, 0.0, 0.0};
  seg_a.p1 = {6.0, 0.0, 0.0};

  //Segments parallel, no overlap, closest points a0, seg_b.p0
  seg_b.p0 = {-1.0, -4.0, 12.0};
  seg_b.p1 = {-5.0, -4.0, 12.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments parallel, no overlap, closest points a0, seg_b.p1
  seg_b.p0 = {-5.0, -4.0, 12.0};
  seg_b.p1 = {-1.0, -4.0, 12.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 1.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, seg_b.p1);

  //Segments parallel, no overlap, closest points a1, seg_b.p0
  seg_b.p0 = {9.0, -4.0, 12.0};
  seg_b.p1 = {18.0, -4.0, 12.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 1.0);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p1);
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments parallel, no overlap, closest points a1, seg_b.p1
  seg_b.p0 = {10.0, -4.0, 12.0};
  seg_b.p1 = {9.0, -4.0, 12.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 1.0);
  EXPECT_EQ(cpResult.u_b, 1.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p1);
  EXPECT_EQ(cpResult.cp_b, seg_b.p1);

  //Segments parallel, overlap, a0---seg_b.p0---a1---seg_b.p1
  seg_b.p0 = {4.0, -3.0, 4.0};
  seg_b.p1 = {10.0, -3.0, 4.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  //Why udQCOverlapping and not udQC_Parallel? Because overlapping segments are already parallel.
  EXPECT_EQ(cpResult.code, udGC_Overlapping);
  EXPECT_EQ(cpResult.u_a, 0.5);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments parallel, overlap, a1---seg_b.p0---a0---seg_b.p1
  seg_b.p0 = {4.0, -3.0, 4.0};
  seg_b.p1 = {0.0, -3.0, 4.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Overlapping);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 0.5);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(2.0, -3.0, 4.0));

  //Segments parallel, overlap, a0---seg_b.p0---seg_b.p1---a1
  seg_b.p0 = {4.0, -3.0, 4.0};
  seg_b.p1 = {5.0, -3.0, 4.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Overlapping);
  EXPECT_EQ(cpResult.u_a, 0.5);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments parallel, overlap, a1---seg_b.p0---seg_b.p1---a0
  seg_b.p0 = {4.0, -3.0, 4.0};
  seg_b.p1 = {8.0, -3.0, 4.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Overlapping);
  EXPECT_EQ(cpResult.u_a, 0.5);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments not parallel, closest points: a0, seg_b.p0
  seg_b.p0 = {-2.0, -5.0, 20.0};
  seg_b.p1 = {-2.0, -5.0, 23.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments not parallel, closest points: a0, seg_b.p1
  seg_b.p0 = {-2.0, -5.0, 23.0};
  seg_b.p1 = {-2.0, -5.0, 20.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 1.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, seg_b.p1);

  //Segments not parallel, closest points: a1, seg_b.p0
  seg_b.p0 = {10.0, -5.0, 20.0};
  seg_b.p1 = {10.0, -5.0, 23.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 1.0);
  EXPECT_EQ(cpResult.u_b, 0.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p1);
  EXPECT_EQ(cpResult.cp_b, seg_b.p0);

  //Segments not parallel, closest points: a1, seg_b.p1
  seg_b.p0 = {10.0, -5.0, 23.0};
  seg_b.p1 = {10.0, -5.0, 20.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 1.0);
  EXPECT_EQ(cpResult.u_b, 1.0);
  EXPECT_EQ(cpResult.cp_a, seg_a.p1);
  EXPECT_EQ(cpResult.cp_b, seg_b.p1);

  //Segments not parallel, closest points: a0, ls1-along ls
  seg_b.p0 = {-1.0, 4.0, -3.0};
  seg_b.p1 = {-1.0, 4.0, 3.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.0);
  EXPECT_EQ(cpResult.u_b, 0.5);
  EXPECT_EQ(cpResult.cp_a, seg_a.p0);
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(-1.0, 4.0, 0.0));

  //Segments not parallel, closest points: a1, ls1-along ls
  seg_b.p0 = {9.0, 4.0, -3.0};
  seg_b.p1 = {9.0, 4.0, 3.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 1.0);
  EXPECT_EQ(cpResult.u_b, 0.5);
  EXPECT_EQ(cpResult.cp_a, seg_a.p1);
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(9.0, 4.0, 0.0));

  //Segments not parallel, closest points: ls0-along ls, ls1-along ls
  seg_b.p0 = {4.0, 4.0, -3.0};
  seg_b.p1 = {4.0, -4.0, -3.0};
  EXPECT_EQ(udGeometry_CPSegmentSegment(seg_a, seg_b, &cpResult), udR_Success);
  EXPECT_EQ(cpResult.code, udGC_Success);
  EXPECT_EQ(cpResult.u_a, 0.5);
  EXPECT_EQ(cpResult.u_b, 0.5);
  EXPECT_EQ(cpResult.cp_a, udDouble3::create(4.0, 0.0, 0.0));
  EXPECT_EQ(cpResult.cp_b, udDouble3::create(4.0, 0.0, -3.0));
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
TEST(GeometryTests, Query_Point_Triangle)
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

TEST(GeometryTests, Query_Point_Polygon)
{
  // Point in triangle
  {
    udGeometryCode code;
    udDouble2 points[] ={{-1.0, -1.0}, {-1.0, 1.0}, {1.0, 0.0}};
    udDouble2 point ={0.0, 0.0};
    EXPECT_EQ(udGeometry_TI2PointPolygon(point, points, UDARRAYSIZE(points), &code), udR_Success);
    EXPECT_EQ(code, udGC_CompletelyInside);
  }

  // Point outside triangle
  {
    udGeometryCode code;
    udDouble2 points[] ={{-1.0, -1.0}, {-1.0, 1.0}, {1.0, 0.0}};
    udDouble2 point ={2.0, 1.0};
    EXPECT_EQ(udGeometry_TI2PointPolygon(point, points, UDARRAYSIZE(points), &code), udR_Success);
    EXPECT_EQ(code, udGC_CompletelyOutside);
  }

  // Point in triangle, point aligned with triangle vertex
  {
    udGeometryCode code;
    udDouble2 points[] ={{-1.0, 0.0}, {1.0, 1.0}, {1.0, -1.0}};
    udDouble2 point ={0.0, 0.0};
    EXPECT_EQ(udGeometry_TI2PointPolygon(point, points, UDARRAYSIZE(points), &code), udR_Success);
    EXPECT_EQ(code, udGC_CompletelyInside);
  }

  // Point in triangle, point aligned with triangle vertices
  {
    udGeometryCode code;
    udDouble2 points[] =
    {
      {-3.0, 0.0}, {-1.0, 0.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, -1.0}, {-3.0, -1.0}
    };
    udDouble2 point ={0.0, 0.0};
    EXPECT_EQ(udGeometry_TI2PointPolygon(point, points, UDARRAYSIZE(points), &code), udR_Success);
    EXPECT_EQ(code, udGC_CompletelyInside);
  }

  // Point in triangle, point aligned with triangle vertices
  {
    udGeometryCode code;
    udDouble2 points[] =
    {
      {-3.0, 0.0}, {-2.0, 0.0}, {-1.0, 0.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, -1.0}, {-3.0, -1.0}
    };
    udDouble2 point ={0.0, 0.0};
    EXPECT_EQ(udGeometry_TI2PointPolygon(point, points, UDARRAYSIZE(points), &code), udR_Success);
    EXPECT_EQ(code, udGC_CompletelyInside);
  }
}
TEST(GeometryTests, Query_Segment_Triangle)
{
  udTriangle3<double> tri = {};
  udSegment3<double> seg = {};
  udFI3SegmentTriangleResult<double> data = {};

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

TEST(GeometryTests, Query_Point_AABB)
{
  udAABB3<double> aabb ={};
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

TEST(GeometryTests, Query_AABB_AABB)
{
  udAABB3<double> aabb0 ={};
  udAABB3<double> aabb1 ={};
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

TEST(GeometryTests, GeometryAABB)
{
  // Merging AABB
  {
    udAABB3<double> aabb0 = {};
    udAABB3<double> aabb1 = {};

    EXPECT_EQ(aabb0.Set({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}), udR_Success);
    EXPECT_EQ(aabb1.Set({-1.0, -2.0, -1.0}, {1.0, 2.0, 0.0}), udR_Success);

    aabb0.Merge(aabb1);

    EXPECT_EQ(aabb0.minPoint.x, -1.0);
    EXPECT_EQ(aabb0.minPoint.y, -2.0);
    EXPECT_EQ(aabb0.minPoint.z, -1.0);

    EXPECT_EQ(aabb0.maxPoint.x, 1.0);
    EXPECT_EQ(aabb0.maxPoint.y, 2.0);
    EXPECT_EQ(aabb0.maxPoint.z, 1.0);
  }
}
