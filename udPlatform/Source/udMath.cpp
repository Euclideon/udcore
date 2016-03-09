#include "stdlib.h"
#include "udPlatform.h"
#include "udMath.h"

#define EXHAUSTIVE_TESTS 0
#define TEST_ASSERTS 0

// This test slerps 2 quaterions rotating about the -Y axis (backwards) q0 theta PI/4 and q1 theta 3PI/4
template <typename T>
bool udQuaternion_SlerpBasicUnitTest()
{
  udVector3<T> axis = { T(0.0), T(-1.0), T(0.0) };
  const T rad0 = T(UD_PI / 4.0);
  const T rad1 = T(3.0 * UD_PI / 4.0);
  udQuaternion<T> q0 = udQuaternion<T>::create(axis, rad0);
  udQuaternion<T> q1 = udQuaternion<T>::create(axis, rad1);
  udVector3<T> v = { T(1.0), T(0.0),  T(0.0) };

  const int32_t count = 1024;
  const T tCount = T(count);
  for (int32_t i = 0; i <= count; ++i)
  {
    T inc = T(i) / tCount;
    udQuaternion<T> qr = udSlerp(q0, q1, T(inc));
    udMatrix4x4<T> mr = udMatrix4x4<T>::rotationAxis(axis, rad0 * (T(1) - inc) + inc * rad1);
    udVector3<T> vqr = qr.apply(v);
    udVector3<T> vmr = udMul(mr, v);

    if (!udEqualApprox(vqr, vmr, T(1.0 / 262144)))
      return false;
  }
  return true;
}

// This function is the foundation of all but one of the slerp unit tests. It works as
// follows, the rotation that represents rotating from q0 to q1 is found, we'll call this qi.
// We then extract the axis of rotation (A) and the angle (theta) from qi.  We then iterate
// from 0 to 1 in steps of 1/incrementCount using this for 't' in the slerp of q0 and q1. At
// the same time using the same 't' we linearly interpolate theta and build a rotation
// matrix from A.  This matrix is then concatenated with the matrix built from q0. This new
// matrix represents the same rotation as the result of the slerped quaternions, each of which
// is now applied to a vector and the results compared with an error tollerance.

template <typename T>
bool udQuaternion_SlerpAxisAngleUnitTest(udQuaternion<T> q0, udQuaternion<T> q1, udVector3<T> v, int incrementCount, const T epsilon)
{
  const T thetaEpsilon = T(UD_PI / (180.0 * 100.0)); // 1/100 of a degree
  const T tIncrementCount = T(incrementCount);

  T cosHalfTheta = udDotQ(q0, q1); // Dot product of 2 quaterions results in cos(theta/2)

  // udSlerp will use a normalized lerp if the absolute of the dot of the two quaternions is within 1/100 of a degree
  if ((T(1) - udAbs(cosHalfTheta)) < thetaEpsilon)
  {
    for (int i = 0; i < incrementCount; ++i)
    {
      T inc = T(i) / tIncrementCount;
      udQuaternion<T> nl = udNormalize(udLerp(q0, q1, inc));
      udQuaternion<T> q = udSlerp(q0, q1, inc);

      udVector3<T> vnl = nl.apply(v);
      udVector3<T> vq = q.apply(v);

      if (!udEqualApprox(vnl, vq, epsilon))
        return false;
    }

    return true;
  }

  udMatrix4x4<T> m0 = udMatrix4x4<T>::rotationQuat(q0);

  if (!udEqualApprox(q0.apply(v), udMul(m0, v), epsilon))
    return false;

  T q0SinHalfTheta = udSqrt(T(1) - q0.w * q0.w);
  udVector3<T> q0Axis = { q0.x / q0SinHalfTheta, q0.y / q0SinHalfTheta, q0.z / q0SinHalfTheta };

  T q0Theta = udACos(q0.w) * T(2.0);

  udMatrix4x4<T> q0AxisM = udMatrix4x4<T>::rotationAxis(q0Axis, q0Theta);

  {
    udVector3<T> v0 = udMul(m0, v);
    udVector3<T> v1 = udMul(q0AxisM, v);
    if (!udEqualApprox(v0, v1, epsilon))
      return false;
  }

  T halfTheta = udACos(cosHalfTheta);

  // If the angle between q1 and q2 is too close to PI to slerp just return true as this is an invalid test
  if (udAbs(2.0 * halfTheta - T(UD_PI)) < thetaEpsilon)
    return true;

  // If the rotation is greater than PI (180°) neqate q1 to take the shortest path along the arc
  udQuaternion<T> q1c = cosHalfTheta < T(0) ? -q1 : q1;
  udQuaternion<T> qr = udMul(udConjugate(q0), q1c);

  {
    udVector3<T> vt0 = qr.apply(v);
    vt0 = q0.apply(vt0);

    udVector3<T> vt1 = q1.apply(v);
    if (!udEqualApprox(vt0, vt1, epsilon))
      return false;
  }

  T sinHalfTheta = udSqrt(T(1) - cosHalfTheta * cosHalfTheta);
  udVector3<T> axis = { qr.x / sinHalfTheta, qr.y / sinHalfTheta, qr.z / sinHalfTheta };

  T theta = udACos(qr.w) * T(2.0);

  for (int i = 0; i < incrementCount; ++i)
  {
    T inc = T(i) / tIncrementCount;
    udMatrix4x4<T> m = udMatrix4x4<T>::rotationAxis(axis, theta * inc);
    udQuaternion<T> q = udSlerp(q0, q1, inc);

    // Concatenate the interpolated rotation
    m = udMul(m0, m);

    udVector3<T> vm = udMul(m, v);
    udVector3<T> vq = q.apply(v);

    if (!udEqualApprox(vm, vq, epsilon))
      return false;
  }

  return true;
}

// This simple test checks clockwise, counter clockwise spherical interpolations.  The angle between the quaternions
// has been chosen such that half the tests must negate the second quaternion to ensure the shortest path is taken.
// This test overlaps with udQuaternion_SlerpRandomInputsTest(), however if a serious error occurs its likly to be
// caught in this test first and with its constant inputs it will be much easer to debug the problem.
template <typename T>
bool udQuaternion_SlerpDefinedInputsTest(int incrementCount)
{
  // The positive rotations in this test are counter clockwise about the Y axis
  // Note that rotating counter clockwise requires the axis of rotation to point out of the screen thus -Y.

  udVector3<T> v = { T(1.0), T(1.0), T(1.0) };
  udVector3<T> axis = { T(0.0), T(-1.0), T(0.0) };
  const T epsilon = sizeof(T) == 4 ? T(1.0 / 1024.0) : T(1.0 / 4096.0);

  T tPI = T(UD_PI);
  // Counter clockwise
  {
    T theta = tPI / T(8.0);
    udQuaternion<T> q0 = udQuaternion<T>::create(axis, theta);
    udQuaternion<T> q1 = udQuaternion<T>::create(axis, theta * T(2.0));

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q1, v, incrementCount, epsilon))
      return false;
  }

  // Clockwise
  {
    T theta = tPI / T(-8.0);
    udQuaternion<T> q0 = udQuaternion<T>::create(axis, theta);
    udQuaternion<T> q1 = udQuaternion<T>::create(axis, theta * T(2.0));

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q1, v, incrementCount, epsilon))
      return false;
  }

  // Clockwise because the shortest path is the opposite direction
  {
    T theta = tPI / T(8.0);
    udQuaternion<T> q0 = udQuaternion<T>::create(axis, theta);
    udQuaternion<T> q1 = udQuaternion<T>::create(axis, theta + tPI * T(1.25));

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q1, v, incrementCount, epsilon))
      return false;
  }

  // Counter clockwise because the shortest path is the opposite direction
  {
    T theta = tPI / T(-8.0);
    udQuaternion<T> q0 = udQuaternion<T>::create(axis, theta);
    udQuaternion<T> q1 = udQuaternion<T>::create(axis, theta + tPI * T(1.25));

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q1, v, incrementCount, epsilon))
      return false;
  }

  // Test when quaternions are equal or approximately equal
  {
    T theta = tPI / T(-8.0);
    const T thetaApprox = theta - T(UD_PI / (180.0 * 100.0) * 0.5); // 1/200 of a degree
    udQuaternion<T> q0 = udQuaternion<T>::create(axis, theta);
    udQuaternion<T> q1 = udQuaternion<T>::create(axis, thetaApprox);

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q0, v, incrementCount, epsilon))
      return false;

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, q1, v, incrementCount, epsilon))
      return false;

    if (!udQuaternion_SlerpAxisAngleUnitTest<T>(q0, -q1, v, incrementCount, epsilon))
      return false;
  }


  return true;
}

template <typename T>
bool EqualApproxUnitTest()
{
  typedef typename T::ElementType ET;
  const ET epsilon = ET(0.01);
  T a = T::zero();
  T b = T::zero();
  T c = T::zero();

  const int count = T::ElementCount;

  for (int i = 0; i < count; ++i)
  {
    b[i] = ET(0.001) / ET(count);
    c[i] = ET(0.02) / ET(count);
  }

  if (!udEqualApprox(a, a, epsilon))
    return false;

  if (!udEqualApprox(a, b, epsilon))
    return false;

  if (udEqualApprox(a, c, epsilon))
    return false;

  return true;
}

template <typename T>
bool IsUnitLengthUnitTest()
{
  typedef typename T::ElementType ET;
  const ET epsilon = ET(0.01);

  T unitLength = T::zero();
  unitLength.x = ET(1);
  if (!udIsUnitLength(unitLength, epsilon))
    return false;

  T v = T::zero();
  v.x = 0.999;
  if (!udIsUnitLength(v, epsilon))
    return false;

  v.x = 1.011;
  if (udIsUnitLength(v, epsilon))
    return false;

  return true;
}

// FAIL Test unit length asserts
template <typename T>
void udQuaternion_SlerpAssertUnitLengthQ0()
{
  udVector3<T> axis = { T(0.0), T(2.0), T(0.0) };
  udQuaternion<T> qi = udQuaternion<T>::identity();
  udQuaternion<T> q0 = udQuaternion<T>::create(axis, T(0.1));
  udSlerp(q0, qi, T(0.5)); // This should assert
}

template <typename T>
void udQuaternion_SlerpAssertUnitLengthQ1()
{
  udVector3<T> axis = { T(0.0), T(2.0), T(0.0) };
  udQuaternion<T> qi = udQuaternion<T>::identity();
  udQuaternion<T> q0 = udQuaternion<T>::create(axis, T(UD_HALF_PI));
  udSlerp(qi, q0, T(0.5)); // This should assert
}

// FAIL Test assert for slerping quaternions separated by an angle of PI (180°)
template <typename T>
void udQuaternion_SlerpAssertPITheta()
{
  udVector3<T> axis = { T(0.0), T(1.0), T(0.0) };
  udQuaternion<T> q0 = udQuaternion<T>::create(axis, T(UD_PI / 4.0));
  udQuaternion<T> q1 = udQuaternion<T>::create(axis, T(5.0 * UD_PI / 4.0));
  udSlerp(q0, q1, T(0.5)); // This should assert
}

template <typename T>
void udQuaternion_SlerpAlmostPITheta()
{
  udVector3<T> axis = { T(0.0), T(1.0), T(0.0) };
  udQuaternion<T> q0 = udQuaternion<T>::create(axis, T(UD_PI / 4.0));
  udQuaternion<T> q1 = udQuaternion<T>::create(axis, T(5.0 * UD_PI / 4.0) - T(2.0 * UD_PI / (180.0 * 100.0))); // 2/100 of a degree

  udSlerp(q0, q1, T(0.5)); // This should not assert
}

bool udMath_Test()
{
  udFloat4 x;
  udFloat4x4 m;

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

  if (!EqualApproxUnitTest<udVector2<float> >())
    return false;
  if (!EqualApproxUnitTest<udVector2<double> >())
    return false;

  if (!EqualApproxUnitTest<udVector3<float> >())
    return false;
  if (!EqualApproxUnitTest<udVector3<double> >())
    return false;

  if (!EqualApproxUnitTest<udVector4<float> >())
    return false;
  if (!EqualApproxUnitTest<udVector4<double> >())
    return false;

  if (!EqualApproxUnitTest<udQuaternion<float> >())
    return false;
  if (!EqualApproxUnitTest<udQuaternion<double> >())
    return false;


  if (!IsUnitLengthUnitTest<udVector2<float> >())
    return false;
  if (!IsUnitLengthUnitTest<udVector2<double> >())
    return false;

  if (!IsUnitLengthUnitTest<udVector3<float> >())
    return false;
  if (!IsUnitLengthUnitTest<udVector3<double> >())
    return false;

  if (!IsUnitLengthUnitTest<udVector4<float> >())
    return false;
  if (!IsUnitLengthUnitTest<udVector4<double> >())
    return false;

  if (!IsUnitLengthUnitTest<udQuaternion<float> >())
    return false;
  if (!IsUnitLengthUnitTest<udQuaternion<double> >())
    return false;

  if (TEST_ASSERTS)
  {
    // These should assert
    udQuaternion_SlerpAssertUnitLengthQ0<double>();
    udQuaternion_SlerpAssertUnitLengthQ0<float>();

    udQuaternion_SlerpAssertUnitLengthQ1<double>();
    udQuaternion_SlerpAssertUnitLengthQ1<float>();

    udQuaternion_SlerpAssertPITheta<double>();
    udQuaternion_SlerpAssertPITheta<float>();

    // These shouldn't assert
    udQuaternion_SlerpAlmostPITheta<double>();
    udQuaternion_SlerpAlmostPITheta<float>();
  }

  if (!udQuaternion_SlerpBasicUnitTest<double>())
    return false;
  if (!udQuaternion_SlerpBasicUnitTest<float>())
    return false;

  int incrementCount = EXHAUSTIVE_TESTS ? 1024 : 256;

  if (!udQuaternion_SlerpDefinedInputsTest<double>(incrementCount))
    return false;
  if (!udQuaternion_SlerpDefinedInputsTest<float>(incrementCount))
    return false;

  if (!udQuaternion_SlerpRandomInputsTest<double>(1024, incrementCount))
    return false;
  if (!udQuaternion_SlerpRandomInputsTest<float>(1024, incrementCount))
    return false;

  return true;
}
