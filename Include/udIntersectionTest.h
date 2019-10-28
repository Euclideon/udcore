#ifndef UDINTERSECTIONTEST_H
#define UDINTERSECTIONTEST_H

// Some helper functions for simply intersection tests

#include "udMath.h"

bool udIntersectionTest_AABBTriangle(const udDouble3 &boxMin, double boxExtents, const udDouble3 &a, const udDouble3 &b, const udDouble3 &c);
bool udIntersectionTest_AABBLine(const udDouble3 &boxMin, double boxExtents, const udDouble3 &a, const udDouble3 &b);

#endif // UDINTERSECTIONTEST_H
