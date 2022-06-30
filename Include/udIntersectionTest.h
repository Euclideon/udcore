#ifndef UDINTERSECTIONTEST_H
#define UDINTERSECTIONTEST_H

// Some helper functions for intersection test of a primitive with and AABB

#include "udMath.h"

bool udIntersectionTest_AABBTriangle(const udDouble3 &boxCenter, const udDouble3 &boxHalfSize, const udDouble3 triVerts[3]);
bool udIntersectionTest_AABBLine(const udDouble3 &boxCenter, const udDouble3 &boxExtents, const udDouble3 lineVerts[2]);

// Temporary wrappers for compatibility with float vectors
// Float interface is deprecated, please use doubles
inline bool udIntersectionTest_AABBTriangle(const udFloat3 &boxCenter, const udFloat3 &boxHalfSize, const udFloat3 _triVerts[3])
{
  udDouble3 triVerts[3] = {
    udDouble3::create(_triVerts[0]),
    udDouble3::create(_triVerts[1]),
    udDouble3::create(_triVerts[2])
  };
  return udIntersectionTest_AABBTriangle(udDouble3::create(boxCenter), udDouble3::create(boxHalfSize), triVerts);
}

inline bool udIntersectionTest_AABBLine(const udFloat3 &boxCenter, const udFloat3 &boxExtents, const udFloat3 _lineVerts[2])
{
  udDouble3 lineVerts[2] = {
    udDouble3::create(_lineVerts[0]),
    udDouble3::create(_lineVerts[1])
  };
  return udIntersectionTest_AABBLine(udDouble3::create(boxCenter), udDouble3::create(boxExtents), lineVerts);
}

#endif // UDINTERSECTIONTEST_H
