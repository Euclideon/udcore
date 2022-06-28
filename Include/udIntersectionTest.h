#ifndef UDINTERSECTIONTEST_H
#define UDINTERSECTIONTEST_H

// Some helper functions for simply intersection tests

#include "udMath.h"

bool udIntersectionTest_AABBTriangle(const udDouble3 &boxCenter, const udDouble3 &boxHalfSize, const udDouble3 triVerts[3]);
bool udIntersectionTest_AABBLine(const udDouble3 &boxCenter, const udDouble3 &boxExtents, const udDouble3 lineVerts[2]);

#endif // UDINTERSECTIONTEST_H
