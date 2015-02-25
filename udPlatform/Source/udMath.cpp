#include "udResult.h"
#include "udMath.h"

udResult udMath_Test()
{
  udFloat4 x;
  udFloat4x4 m;
//  udDouble4x4 dm;

  udFloat3 v;
  udFloat4 v4;

  float f = dot3(v, v);
  udPow(f, (float)UD_PI);

  v = 2.f*v;
  v = v*v+v;

  m = udFloat4x4::create(x, x, x, x);

  m.transpose();
  m.determinant();
  m.inverse();

  cross3(v, v4.toVector3());

  v += v;
  v /= 2.f;

  v *= v.one();

  mul(m, 1.f);
  mul(m, v);
  mul(m, v4);
  mul(m, m);


  return udR_Success;
}

