#ifndef UDINTERSECTIONTEST_H
#define UDINTERSECTIONTEST_H

// Some helper functions for simply intersection tests

#include "udMath.h"

bool udIntersectionTest_AABBTriangle(const udDouble3 &boxCenter, const udDouble3 &boxHalfSize, const udDouble3 triVerts[3]);
bool udIntersectionTest_AABBLine(const udDouble3 &boxCenter, const udDouble3 &boxExtents, const udDouble3 lineVerts[2]);
//bool udIntersectionTest_AABBCapsule(const udDouble3 &boxCenter, const udDouble3 &boxExtents, const udDouble3 endPoints[2], double radius);
bool udIntersectionTest_AABBCapsule(const udDouble3  &boxCenter, const udDouble3  &boxExtents, const udDouble3 endPoints[2], const double radius);

#endif // UDINTERSECTIONTEST_H
