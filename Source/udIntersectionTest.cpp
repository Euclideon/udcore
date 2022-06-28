/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(double boxcenter[3],      */
/*          double boxHalfSize[3],double triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#include "udIntersectionTest.h"

static bool PlaneBoxOverlap(const udDouble3 &normal, const udDouble3 &vert, const udDouble3 &maxbox)	// -NJMP-
{
  int q;
  double v;
  udDouble3 vmin, vmax;
  for (q = 0; q < 3; ++q)
  {
    v = vert[q];					// -NJMP-
    if (normal[q] > 0.0)
    {
      vmin[q] = -maxbox[q] - v;	// -NJMP-
      vmax[q] =  maxbox[q] - v;	// -NJMP-
    }
    else
    {
      vmin[q] =  maxbox[q] - v;	// -NJMP-
      vmax[q] = -maxbox[q] - v;	// -NJMP-
    }
  }
  if (udDot(normal, vmin) > 0.0) return false;	// -NJMP-
  if (udDot(normal, vmax) >= 0.0) return true;	// -NJMP-

  return false;
}

//=================== Find min/max ========================
#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if (x1 < min) min=x1;\
  if (x1 > max) max=x1;\
  if (x2 < min) min=x2;\
  if (x2 > max) max=x2;

//======================== X-tests ========================
#define AXISTEST_X01(a, b, fa, fb)			   \
  p0 = a*v0.y - b*v0.z;			       	   \
  p2 = a*v2.y - b*v2.z;			       	   \
  if (p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
  rad = fa * boxHalfSize.y + fb * boxHalfSize.z;   \
  if (min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)			   \
  p0 = a*v0.y - b*v0.z;			           \
  p1 = a*v1.y - b*v1.z;			       	   \
  if (p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxHalfSize.y + fb * boxHalfSize.z;   \
  if (min>rad || max<-rad) return 0;

//======================== Y-tests ========================
#define AXISTEST_Y02(a, b, fa, fb)			   \
  p0 = -a*v0.x + b*v0.z;		      	   \
  p2 = -a*v2.x + b*v2.z;	       	       	   \
  if (p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
  rad = fa * boxHalfSize.x + fb * boxHalfSize.z;   \
  if (min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)			   \
  p0 = -a*v0.x + b*v0.z;		      	   \
  p1 = -a*v1.x + b*v1.z;	     	       	   \
  if (p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxHalfSize.x + fb * boxHalfSize.z;   \
  if (min>rad || max<-rad) return 0;

//======================== Z-tests ========================

#define AXISTEST_Z12(a, b, fa, fb)			   \
  p1 = a*v1.x - b*v1.y;			           \
  p2 = a*v2.x - b*v2.y;			       	   \
  if (p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
  rad = fa * boxHalfSize.x + fb * boxHalfSize.y;   \
  if (min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)			   \
  p0 = a*v0.x - b*v0.y;				   \
  p1 = a*v1.x - b*v1.y;			           \
  if (p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxHalfSize.x + fb * boxHalfSize.y;   \
  if (min>rad || max<-rad) return 0;

// ****************************************************************************
// Author: Dave Pevreal, February 2019 (slightly translated from above credited authors)
bool udIntersectionTest_AABBTriangle(const udDouble3 &boxCenter, const udDouble3 &boxHalfSize, const udDouble3 triVerts[3])
{
  //    use separating axis theorem to test overlap between triangle and box
  //    need to test for overlap in these directions:
  //    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle
  //       we do not even need to test these)
  //    2) normal of the triangle
  //    3) crossproduct(edge from tri, {x,y,z}-direction)
  //       this gives 3x3=9 more tests
  udDouble3 v0,v1,v2;
  double min,max,p0,p1,p2,rad,fex,fey,fez;		// -NJMP- "d" local variable removed
  udDouble3 normal,e0,e1,e2;

  // This is the fastest branch on Sun
  // move everything so that the boxcenter is in (0,0,0)
  v0 = triVerts[0] - boxCenter;
  v1 = triVerts[1] - boxCenter;
  v2 = triVerts[2] - boxCenter;

  // compute triangle edges
  e0 = v1 - v0;      // tri edge 0
  e1 = v2 - v1;      // tri edge 1
  e2 = v0 - v2;      // tri edge 2

  // Bullet 3:
  //  test the 9 tests first (this was faster)
  fex = fabs(e0.x);
  fey = fabs(e0.y);
  fez = fabs(e0.z);
  AXISTEST_X01(e0.z, e0.y, fez, fey);
  AXISTEST_Y02(e0.z, e0.x, fez, fex);
  AXISTEST_Z12(e0.y, e0.x, fey, fex);

  fex = fabs(e1.x);
  fey = fabs(e1.y);
  fez = fabs(e1.z);
  AXISTEST_X01(e1.z, e1.y, fez, fey);
  AXISTEST_Y02(e1.z, e1.x, fez, fex);
  AXISTEST_Z0(e1.y, e1.x, fey, fex);

  fex = fabs(e2.x);
  fey = fabs(e2.y);
  fez = fabs(e2.z);
  AXISTEST_X2(e2.z, e2.y, fez, fey);
  AXISTEST_Y1(e2.z, e2.x, fez, fex);
  AXISTEST_Z12(e2.y, e2.x, fey, fex);

  // Bullet 1:
  //  first test overlap in the {x,y,z}-directions
  //  find min, max of the triangle each direction, and test for overlap in
  //  that direction -- this is equivalent to testing a minimal AABB around
  //  the triangle against the AABB

  // test in X-direction
  FINDMINMAX(v0.x, v1.x, v2.x, min, max);
  if (min > boxHalfSize.x || max < -boxHalfSize.x)
    return false;

  // test in Y-direction
  FINDMINMAX(v0.y, v1.y, v2.y, min, max);
  if (min > boxHalfSize.y || max < -boxHalfSize.y)
    return false;

  // test in Z-direction
  FINDMINMAX(v0.z, v1.z, v2.z, min, max);
  if (min > boxHalfSize.z || max < -boxHalfSize.z)
    return false;

  // Bullet 2:
  //  test if the box intersects the plane of the triangle
  //  compute plane equation of triangle: normal*x+d=0
  normal = udCross3(e0, e1);
  // -NJMP- (line removed here)
  if (!PlaneBoxOverlap(normal, v0, boxHalfSize))
    return false;	// -NJMP-

  return true;   // box and triangle overlaps
}

// AABB vs line segment test. By Miguel Gomez
// Taken from http://www.gamasutra.com/view/feature/131790/simple_intersection_tests_for_games.php?page=6
// TODO: Some optimization opportunity on this one, particularly precomputing the abs linedir
static bool AABB_LineSegmentOverlap(udDouble3 lineDir,	    // line direction
                                    udDouble3 mid,	  // midpoint of the line segment
                                    double halfLength,	    // segment half-length
                                    udDouble3 boxCenter, // box center/extents
                                    udDouble3 boxExtents)
{
  // ALGORITHM: Use the separating axis theorem to see if the line segment
  // and the box overlap. A line segment is a degenerate OBB.

  double r;
  const udDouble3 t = boxCenter - mid;

  // do any of the principal axes form a separating axis?

  if( fabs(t.x) > boxExtents.x + halfLength * fabs(lineDir.x) )
    return 0;

  if( fabs(t.y) > boxExtents.y + halfLength * fabs(lineDir.y) )
    return 0;

  if( fabs(t.z) > boxExtents.z + halfLength * fabs(lineDir.z) )
    return 0;

  /* NOTE: Since the separating axis is perpendicular to the line in these
  last four cases, the line does not contribute to the projection. */

  //lineDir.cross(x-axis)?

  r = boxExtents.y * fabs(lineDir.z) + boxExtents.z * fabs(lineDir.y);

  if (fabs(t.y*lineDir.z - t.z*lineDir.y) > r)
    return 0;

  //lineDir.cross(y-axis)?

  r = boxExtents.x * fabs(lineDir.z) + boxExtents.z * fabs(lineDir.x);

  if (fabs(t.z * lineDir.x - t.x * lineDir.z) > r)
    return 0;

  //lineDir.cross(z-axis)?

  r = boxExtents.x * fabs(lineDir.y) + boxExtents.y * fabs(lineDir.x);

  if (fabs(t.x * lineDir.y - t.y * lineDir.x) > r)
    return false;

  return true;
}

bool udIntersectionTest_AABBLine(const udDouble3 &boxCenter, const udDouble3 &boxExtents, const udDouble3 lineVerts[2])
{
  // TODO: Optimize by moving these calculations into DXFParser so they are only done once per line
  udDouble3 lineDir;
  udDouble3 mid;
  double halfLength;
  mid = (lineVerts[0] + lineVerts[1]) * 0.5f;
  lineDir = mid - lineVerts[0];
  halfLength = udMag3(lineDir);
  lineDir = lineDir * (1.f / halfLength);
  return AABB_LineSegmentOverlap(lineDir, mid, halfLength, boxCenter, boxExtents);
}
