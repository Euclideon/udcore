#ifndef UDINTERSECTIONTEST_H
#define UDINTERSECTIONTEST_H

// Some helper functions for simply intersection tests

#include "udMath.h"

bool udIntersectionTest_AABBTriangle(const udFloat3 &boxCenter, const udFloat3 &boxHalfSize, const udFloat3 triVerts[3]);
bool udIntersectionTest_AABBLine(const udFloat3 &boxCenter, const udFloat3 &boxExtents, const udFloat3 lineVerts[2]);

#endif // UDINTERSECTIONTEST_H
