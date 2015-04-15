#include "udResult.h"
#include "udMath.h"

udResult udMath_Test()
{
  udFloat4 x;
  udFloat4x4 m;
//  udDouble4x4 dm;

  udFloat3 v;
  udFloat4 v4;

  float f = udDot3(v, v);
  udPow(f, (float)UD_PI);

  v = 2.f*v;
  v = v*v+v;

  m = udFloat4x4::create(x, x, x, x);

  m.transpose();
  m.determinant();
  m.inverse();

  udCross3(v, v4.toVector3());

  v += v;
  v /= 2.f;

  v *= v.one();

  udMul(m, 1.f);
  udMul(m, v);
  udMul(m, v4);
  udMul(m, m);

  udAbs(-1.f);
  udAbs(-1.0);
  udAbs(-1);
  udAbs(v);

  return udR_Success;
}

