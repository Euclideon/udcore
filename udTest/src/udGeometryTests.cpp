#include "udGeometry.h"
#include "gtest/gtest.h"
#include "udPlatform.h"

// Due to IEEE-754 supporting two different ways to specify 8.f and 2.f, the next three EXPECT_* calls are different
// This appears to only occur when using Clang in Release, and only for udFloat4 - SIMD optimizations perhaps?
#define EXPECT_UDFLOAT2_EQ(expected, _actual) { udFloat2 actual = _actual; EXPECT_FLOAT_EQ(expected.x, actual.x); EXPECT_FLOAT_EQ(expected.y, actual.y); }
#define EXPECT_UDFLOAT3_EQ(expected, _actual) { udFloat3 actual = _actual; EXPECT_FLOAT_EQ(expected.x, actual.x); EXPECT_FLOAT_EQ(expected.y, actual.y); EXPECT_FLOAT_EQ(expected.z, actual.z); }
#define EXPECT_UDFLOAT4_EQ(expected, _actual) { udFloat4 actual = _actual; EXPECT_FLOAT_EQ(expected.x, actual.x); EXPECT_FLOAT_EQ(expected.y, actual.y); EXPECT_FLOAT_EQ(expected.z, actual.z); EXPECT_FLOAT_EQ(expected.w, actual.w); }


TEST(GeometryTests, GeometryLines)
{
  //point vs line
  {
    udDouble3 lineOrigin = {1.0, 1.0, 1.0};
    udDouble3 lineDirection = {1.0, 0.0, 0.0};
    double u;
    udDouble3 cp;
    udDouble3 point;
    udGeometryCode result;

    //Point lies 'before' line origin
    point = {-3.0, 1.0, 2.0};
    result = udCP_PointLine3(lineOrigin, lineDirection, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, -4.0);
    EXPECT_EQ(cp, udDouble3::create(-3.0, 1.0, 1.0));

    //Point lies perpendicular to line origin
    point = {1.0, 1.0, 2.0};
    result = udCP_PointLine3(lineOrigin, lineDirection, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, 0.0);
    EXPECT_EQ(cp, udDouble3::create(1.0, 1.0, 1.0));

    //Point lies 'after' line origin
    point = {7.0, 1.0, 2.0};
    result = udCP_PointLine3(lineOrigin, lineDirection, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, 6.0);
    EXPECT_EQ(cp, udDouble3::create(7.0, 1.0, 1.0));
  }

  //point v segment
  {
    udDouble3 s0 = {1.0, 1.0, 1.0};
    udDouble3 s1 = {3.0, 1.0, 1.0};
    double u;
    udDouble3 cp;
    udDouble3 point;
    udGeometryCode result;

    //Point lies 'before' segment 0
    point = {-1.0, 1.0, 1.0};
    result = udCP_PointSegment3(s0, s1, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, 0.0);
    EXPECT_EQ(cp, s0);

    //Point lies 'after' segment 1
    point = {5.0, 1.0, 1.0};
    result = udCP_PointSegment3(s0, s1, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, 1.0);
    EXPECT_EQ(cp, s1);

    //Point lies along the segment line
    point = {2.0, 10.0, 42.0};
    result = udCP_PointSegment3(s0, s1, point, cp, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u, 0.5);
    EXPECT_EQ(cp, udDouble3::create(2.0, 1.0, 1.0));
  }

  //segment v segment
  {
    udDouble3 a0, a1, b0, b1, cpa, cpb;
    udDouble2 u;
    udGeometryCode result;

    //Segments have zero length
    a0 = b0 = {1.0, 4.0, 12.0};
    a1 = b1 = {1.0, 4.0, 12.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, b0);

    a0 = {2.0, 0.0, 0.0};
    a1 = {6.0, 0.0, 0.0};

    //LineSegs parallel, no overlap, closest points a0, b0
    b0 = {-1.0, -4.0, 12.0};
    b1 = {-5.0, -4.0, 12.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, b0);

    //LineSegs parallel, no overlap, closest points a0, b1
    b0 = {-5.0, -4.0, 12.0};
    b1 = {-1.0, -4.0, 12.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 1.0);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, b1);

    //LineSegs parallel, no overlap, closest points a1, b0
    b0 = {9.0, -4.0, 12.0};
    b1 = {18.0, -4.0, 12.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 1.0);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, a1);
    EXPECT_EQ(cpb, b0);

    //LineSegs parallel, no overlap, closest points a1, b1
    b0 = {10.0, -4.0, 12.0};
    b1 = {9.0, -4.0, 12.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 1.0);
    EXPECT_EQ(u[1], 1.0);
    EXPECT_EQ(cpa, a1);
    EXPECT_EQ(cpb, b1);

    //LineSegs parallel, overlap, a0---b0---a1---b1
    b0 = {4.0, -3.0, 4.0};
    b1 = {10.0, -3.0, 4.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    //Why udQCOverlapping and not udQC_Parallel? Because overlapping segments are already parallel.
    EXPECT_EQ(result, udGC_Overlapping);
    EXPECT_EQ(u[0], 0.5);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpb, b0);

    //LineSegs parallel, overlap, a1---b0---a0---b1
    b0 = {4.0, -3.0, 4.0};
    b1 = {0.0, -3.0, 4.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Overlapping);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 0.5);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, udDouble3::create(2.0, -3.0, 4.0));

    //LineSegs parallel, overlap, a0---b0---b1---a1
    b0 = {4.0, -3.0, 4.0};
    b1 = {5.0, -3.0, 4.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Overlapping);
    EXPECT_EQ(u[0], 0.5);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpb, b0);

    //LineSegs parallel, overlap, a1---b0---b1---a0
    b0 = {4.0, -3.0, 4.0};
    b1 = {8.0, -3.0, 4.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Overlapping);
    EXPECT_EQ(u[0], 0.5);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpb, b0);

    //LineSegs not parallel, closest points: a0, b0
    b0 = {-2.0, -5.0, 20.0};
    b1 = {-2.0, -5.0, 23.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, b0);

    //LineSegs not parallel, closest points: a0, b1
    b0 = {-2.0, -5.0, 23.0};
    b1 = {-2.0, -5.0, 20.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1], 1.0);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, b1);

    //LineSegs not parallel, closest points: a1, b0
    b0 = {10.0, -5.0, 20.0};
    b1 = {10.0, -5.0, 23.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 1.0);
    EXPECT_EQ(u[1], 0.0);
    EXPECT_EQ(cpa, a1);
    EXPECT_EQ(cpb, b0);

    //LineSegs not parallel, closest points: a1, b1
    b0 = {10.0, -5.0, 23.0};
    b1 = {10.0, -5.0, 20.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 1.0);
    EXPECT_EQ(u[1], 1.0);
    EXPECT_EQ(cpa, a1);
    EXPECT_EQ(cpb, b1);

    //LineSegs not parallel, closest points: a0, ls1-along ls
    b0 = {-1.0, 4.0, -3.0};
    b1 = {-1.0, 4.0, 3.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.0);
    EXPECT_EQ(u[1],0.5);
    EXPECT_EQ(cpa, a0);
    EXPECT_EQ(cpb, udDouble3::create(-1.0, 4.0, 0.0));

    //LineSegs not parallel, closest points: a1, ls1-along ls
    b0 = {9.0, 4.0, -3.0};
    b1 = {9.0, 4.0, 3.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 1.0);
    EXPECT_EQ(u[1], 0.5);
    EXPECT_EQ(cpa, a1);
    EXPECT_EQ(cpb, udDouble3::create(9.0, 4.0, 0.0));

    //LineSegs not parallel, closest points: ls0-along ls, ls1-along ls
    b0 = {4.0, 4.0, -3.0};
    b1 = {4.0, -4.0, -3.0};
    result = udCP_SegmentSegment3(a0, a1, b0, b1, cpa, cpb, &u);
    EXPECT_EQ(result, udGC_Success);
    EXPECT_EQ(u[0], 0.5);
    EXPECT_EQ(u[1], 0.5);
    EXPECT_EQ(cpa, udDouble3::create(4.0, 0.0, 0.0));
    EXPECT_EQ(cpb, udDouble3::create(4.0, 0.0, -3.0));
  }
}
