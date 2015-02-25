#if !defined(UDMATH_H)
#define UDMATH_H

//      O_O
// YOU'RE JOKING!
#if defined(near)
# undef near
#endif
#if defined(far)
# undef far
#endif
#if defined(max)
# undef max
#endif
#if defined(min)
# undef min
#endif

#if defined(_MSC_VER)
// warning C4201: nonstandard extension used : nameless struct/union
# pragma warning(disable: 4201)
#endif

#define UD_PI            3.1415926535897932384626433832795
#define UD_2PI           6.283185307179586476925286766559
#define UD_HALF_PI       1.5707963267948966192313216916398
#define UD_ROOT_2        1.4142135623730950488016887242097
#define UD_INV_ROOT_2    0.70710678118654752440084436210485
#define UD_RAD2DEG(rad)  ((rad)*57.295779513082320876798154814105)
#define UD_DEG2RAD(deg)  ((deg)*0.01745329251994329576923690768489)

#if defined(__cplusplus)

// prototypes
template <typename T> struct udVector2;
template <typename T> struct udVector3;
template <typename T> struct udVector4;
template <typename T> struct udQuaternion;
template <typename T> struct udMatrix4x4;

// math functions
float udPow(float f, float n);
double udPow(double d, double n);
float udRSqrt(float f);
double udRSqrt(double d);
float udSqrt(float f);
double udSqrt(double d);
float udSin(float f);
double udSin(double d);
float udCos(float f);
double udCos(double d);
float udTan(float f);
double udTan(double d);
float udACos(float f);
double udACos(double d);
float udATan(float f);
double udATan(double d);

// typical linear algebra functions
template <typename T> udVector2<T> abs(const udVector2<T> &v);
template <typename T> udVector3<T> abs(const udVector3<T> &v);
template <typename T> udVector4<T> abs(const udVector4<T> &v);

template <typename T> udVector2<T> max(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> udVector3<T> max(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector4<T> max(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> udVector2<T> min(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> udVector3<T> min(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector4<T> min(const udVector4<T> &v1, const udVector4<T> &v2);

template <typename T> T dot2(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> T dot2(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T dot2(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T dot3(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T dot3(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T dot4(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T doth(const udVector3<T> &v3, const udVector4<T> &v4);

template <typename T> T magSq2(const udVector2<T> &v);
template <typename T> T magSq2(const udVector3<T> &v);
template <typename T> T magSq2(const udVector4<T> &v);
template <typename T> T magSq3(const udVector3<T> &v);
template <typename T> T magSq3(const udVector4<T> &v);
template <typename T> T magSq4(const udVector4<T> &v);

template <typename T> T mag2(const udVector2<T> &v);
template <typename T> T mag2(const udVector3<T> &v);
template <typename T> T mag2(const udVector4<T> &v);
template <typename T> T mag3(const udVector3<T> &v);
template <typename T> T mag3(const udVector4<T> &v);
template <typename T> T mag4(const udVector4<T> &v);

template <typename T> T cross2(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> T cross2(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T cross2(const udVector4<T> &v1, const udVector4<T> &v2);

template <typename T> udVector3<T> cross3(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector3<T> cross3(const udVector4<T> &v1, const udVector4<T> &v2);

template <typename T> udVector2<T> normalize2(const udVector2<T> &v);
template <typename T> udVector3<T> normalize2(const udVector3<T> &v);
template <typename T> udVector4<T> normalize2(const udVector4<T> &v);
template <typename T> udVector3<T> normalize3(const udVector3<T> &v);
template <typename T> udVector4<T> normalize3(const udVector4<T> &v);
template <typename T> udVector4<T> normalize4(const udVector4<T> &v);

// matrix and quat functions
template <typename T> udVector2<T> mul(const udMatrix4x4<T> &m, const udVector2<T> &v);
template <typename T> udVector3<T> mul(const udMatrix4x4<T> &m, const udVector3<T> &v);
template <typename T> udVector4<T> mul(const udMatrix4x4<T> &m, const udVector4<T> &v);
template <typename T> udMatrix4x4<T> mul(const udMatrix4x4<T> &m, T f);
template <typename T> udMatrix4x4<T> mul(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2);

template <typename T> udVector2<T> lerp(const udVector2<T> &v1, const udVector2<T> &v2, T t);
template <typename T> udVector3<T> lerp(const udVector3<T> &v1, const udVector3<T> &v2, T t);
template <typename T> udVector4<T> lerp(const udVector4<T> &v1, const udVector4<T> &v2, T t);
template <typename T> udQuaternion<T> lerp(const udQuaternion<T> &q1, const udQuaternion<T> &q2, T t);
template <typename T> udMatrix4x4<T> lerp(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2, T t);

template <typename T> udMatrix4x4<T> transpose(const udMatrix4x4<T> &m);

template <typename T> T determinant(const udMatrix4x4<T> &m);

template <typename T> udQuaternion<T> inverse(const udQuaternion<T> &q);
template <typename T> udMatrix4x4<T> inverse(const udMatrix4x4<T> &m);

template <typename T> udQuaternion<T> conjugate(const udQuaternion<T> &q);

template <typename T> udQuaternion<T> slerp(const udQuaternion<T> &q1, const udQuaternion<T> &q2, T t);


// types
template <typename T>
struct udVector2
{
  T x, y;

  udVector2<T> operator -() const                      { udVector2<T> r = { -x, -y }; return r; }

  udVector2<T> operator +(const udVector2<T> &v) const { udVector2<T> r = { x+v.x, y+v.y }; return r; }
  udVector2<T> operator -(const udVector2<T> &v) const { udVector2<T> r = { x-v.x, y-v.y }; return r; }
  udVector2<T> operator *(const udVector2<T> &v) const { udVector2<T> r = { x*v.x, y*v.y }; return r; }
  udVector2<T> operator *(T f) const                   { udVector2<T> r = { x*f,   y*f }; return r; }
  udVector2<T> operator /(const udVector2<T> &v) const { udVector2<T> r = { x/v.x, y/v.y }; return r; }
  udVector2<T> operator /(T f) const                   { udVector2<T> r = { x/f,   y/f }; return r; }

  udVector2<T>& operator +=(const udVector2<T> &v)     { x+=v.x; y+=v.y; return *this; }
  udVector2<T>& operator -=(const udVector2<T> &v)     { x-=v.x; y-=v.y; return *this; }
  udVector2<T>& operator *=(const udVector2<T> &v)     { x*=v.x; y*=v.y; return *this; }
  udVector2<T>& operator *=(T f)                       { x*=f;   y*=f;   return *this; }
  udVector2<T>& operator /=(const udVector2<T> &v)     { x/=v.x; y/=v.y; return *this; }
  udVector2<T>& operator /=(T f)                       { x/=f;   y/=f;   return *this; }

  // static members
  static udVector2<T> zero()      { udVector2<T> r = { T(0), T(0) }; return r; }
  static udVector2<T> one()       { udVector2<T> r = { T(1), T(1) }; return r; }

  static udVector2<T> xAxis()     { udVector2<T> r = { T(1), T(0) }; return r; }
  static udVector2<T> yAxis()     { udVector2<T> r = { T(0), T(1) }; return r; }

  static udVector2<T> create(T _f)       { udVector2<T> r = { _f, _f }; return r; }
  static udVector2<T> create(T _x, T _y) { udVector2<T> r = { _x, _y }; return r; }
  template <typename U>
  static udVector2<T> create(const udVector2<U> &_v) { udVector2<T> r = { T(_v.x), T(_v.y) }; return r; }
};
template <typename T>
udVector2<T> operator *(T f, const udVector2<T> &v) { udVector2<T> r = { v.x*f, v.y*f }; return r; }

template <typename T>
struct udVector3
{
  T x, y, z;

  udVector3<T>& toVector2()               { return *(udVector2<T>*)this; }
  const udVector3<T>& toVector2() const   { return *(udVector2<T>*)this; }

  udVector3<T> operator -() const                      { udVector3<T> r = { -x, -y, -z }; return r; }

  udVector3<T> operator +(const udVector3<T> &v) const { udVector3<T> r = { x+v.x, y+v.y, z+v.z }; return r; }
  udVector3<T> operator -(const udVector3<T> &v) const { udVector3<T> r = { x-v.x, y-v.y, z-v.z }; return r; }
  udVector3<T> operator *(const udVector3<T> &v) const { udVector3<T> r = { x*v.x, y*v.y, z*v.z }; return r; }
  udVector3<T> operator *(T f) const                   { udVector3<T> r = { x*f,   y*f,   z*f }; return r; }
  udVector3<T> operator /(const udVector3<T> &v) const { udVector3<T> r = { x/v.x, y/v.y, z/v.z }; return r; }
  udVector3<T> operator /(T f) const                   { udVector3<T> r = { x/f,   y/f,   z/f }; return r; }

  udVector3<T>& operator +=(const udVector3<T> &v)     { x+=v.x; y+=v.y; z+=v.z; return *this; }
  udVector3<T>& operator -=(const udVector3<T> &v)     { x-=v.x; y-=v.y; z-=v.z; return *this; }
  udVector3<T>& operator *=(const udVector3<T> &v)     { x*=v.x; y*=v.y; z*=v.z; return *this; }
  udVector3<T>& operator *=(T f)                       { x*=f;   y*=f;   z*=f;   return *this; }
  udVector3<T>& operator /=(const udVector3<T> &v)     { x/=v.x; y/=v.y; z/=v.z; return *this; }
  udVector3<T>& operator /=(T f)                       { x/=f;   y/=f;   z/=f;   return *this; }

  // static members
  static udVector3<T> zero()  { udVector3<T> r = { T(0), T(0), T(0) }; return r; }
  static udVector3<T> one()   { udVector3<T> r = { T(1), T(1), T(1) }; return r; }

  static udVector3<T> create(T _f)                        { udVector3<T> r = { _f, _f, _f };     return r; }
  static udVector3<T> create(T _x, T _y, T _z)            { udVector3<T> r = { _x, _y, _z };     return r; }
  static udVector3<T> create(const udVector2<T> _v, T _z) { udVector3<T> r = { _v.x, _v.y, _z }; return r; }
  template <typename U>
  static udVector3<T> create(const udVector3<U> &_v) { udVector3<T> r = { T(_v.x), T(_v.y), T(_v.z) }; return r; }
};
template <typename T>
udVector3<T> operator *(T f, const udVector3<T> &v) { udVector3<T> r = { v.x*f, v.y*f, v.z*f }; return r; }

template <typename T>
struct udVector4
{
  T x, y, z, w;

  udVector3<T>& toVector3()               { return *(udVector3<T>*)this; }
  const udVector3<T>& toVector3() const   { return *(udVector3<T>*)this; }
  udVector2<T>& toVector2()               { return *(udVector2<T>*)this; }
  const udVector2<T>& toVector2() const   { return *(udVector2<T>*)this; }

  udVector4<T> operator -() const                      { udVector4<T> r = { -x, -y, -z, -w }; return r; }

  udVector4<T> operator +(const udVector4<T> &v) const { udVector4<T> r = { x+v.x, y+v.y, z+v.z, w+v.w }; return r; }
  udVector4<T> operator -(const udVector4<T> &v) const { udVector4<T> r = { x-v.x, y-v.y, z-v.z, w-v.w }; return r; }
  udVector4<T> operator *(const udVector4<T> &v) const { udVector4<T> r = { x*v.x, y*v.y, z*v.z, w*v.w }; return r; }
  udVector4<T> operator *(T f) const                   { udVector4<T> r = { x*f,   y*f,   z*f,   w*f }; return r; }
  udVector4<T> operator /(const udVector4<T> &v) const { udVector4<T> r = { x/v.x, y/v.y, z/v.z, w/v.w }; return r; }
  udVector4<T> operator /(T f) const                   { udVector4<T> r = { x/f,   y/f,   z/f,   w/f }; return r; }

  udVector4<T>& operator +=(const udVector4<T> &v)     { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
  udVector4<T>& operator -=(const udVector4<T> &v)     { x-=v.x; y-=v.y; z-=v.z; w-=v.w; return *this; }
  udVector4<T>& operator *=(const udVector4<T> &v)     { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
  udVector4<T>& operator *=(T f)                       { x*=f;   y*=f;   z*=f;   w*=f;   return *this; }
  udVector4<T>& operator /=(const udVector4<T> &v)     { x/=v.x; y/=v.y; z/=v.z; w/=v.w; return *this; }
  udVector4<T>& operator /=(T f)                       { x/=f;   y/=f;   z/=f;   w/=f;   return *this; }

  // static members
  static udVector4<T> zero()      { udVector4<T> r = { T(0), T(0), T(0), T(0) }; return r; }
  static udVector4<T> one()       { udVector4<T> r = { T(1), T(1), T(1), T(1) }; return r; }
  static udVector4<T> identity()  { udVector4<T> r = { T(0), T(0), T(0), T(1) }; return r; }

  static udVector4<T> create(T _f)                               { udVector4<T> r = {   _f,   _f,   _f, _f }; return r; }
  static udVector4<T> create(T _x, T _y, T _z, T _w)             { udVector4<T> r = {   _x,   _y,   _z, _w }; return r; }
  static udVector4<T> create(const udVector3<T> &_v, T _w)       { udVector4<T> r = { _v.x, _v.y, _v.z, _w }; return r; }
  static udVector4<T> create(const udVector2<T> &_v, T _z, T _w) { udVector4<T> r = { _v.x, _v.y,   _z, _w }; return r; }
  template <typename U>
  static udVector4<T> create(const udVector4<U> &_v) { udVector4<T> r = { T(_v.x), T(_v.y), T(_v.z), T(_v.w) }; return r; }
};
template <typename T>
udVector4<T> operator *(T f, const udVector4<T> &v) { udVector4<T> r = { v.x*f, v.y*f, v.z*f, v.w*f }; return r; }

template <typename T>
struct udQuaternion
{
  T x, y, z, w;

  udQuaternion<T> operator *(const udQuaternion<T> &q) const { return mul(*this, q); }
  udQuaternion<T> operator *(T f) const                      { udQuaternion<T> r = { x*f, y*f, z*f, w*f }; return r; }

  udQuaternion<T>& operator *=(const udQuaternion<T> &q)     { *this = mul(*this, q); return *this; }
  udQuaternion<T>& operator *=(T f)                          { x*=f; y*=f; z*=f; w*=f; return *this; }

  udQuaternion<T>& inverse();
  udQuaternion<T>& conjugate();

  udVector3<T> apply(const udVector3<T> &v);

  // static members
  static udQuaternion<T> identity()  { udQuaternion<T> r = { T(0), T(0), T(0), T(1) }; return r; }

  static udQuaternion<T> create(const udVector3<T> &axis, T rad);
  template <typename U>
  static udQuaternion<T> create(const udQuaternion<U> &_q) { udQuaternion<T> r = { T(_q.x), T(_q.y), T(_q.z), T(_q.w) }; return r; }
};
template <typename T>
udQuaternion<T> operator *(T f, const udQuaternion<T> &q) { udQuaternion<T> r = { q.x*f, q.y*f, q.z*f, q.w*f }; return r; }

template <typename T>
struct udMatrix4x4
{
  union
  {
    T a[16];
    struct
    {
      udVector4<T> c[4];
    };
    struct
    {
      udVector4<T> x;
      udVector4<T> y;
      udVector4<T> z;
      udVector4<T> t;
    } axis;
    struct
    {
      T // remember, we store columns (axiis) sequentially! (so appears transposed here)
        _00, _10, _20, _30,
        _01, _11, _21, _31,
        _02, _12, _22, _32,
        _03, _13, _23, _33;
    } m;
  };

  udMatrix4x4<T> operator *(const udMatrix4x4<T> &m) const { return mul(*this, m); }
  udMatrix4x4<T> operator *(T f) const                     { return mul(*this, f); }
  udVector4<T> operator *(const udVector4<T> &v) const     { return mul(*this, v); }

  udMatrix4x4<T>& operator *=(const udMatrix4x4<T> &m)     { *this = mul(*this, m); return *this; }
  udMatrix4x4<T>& operator *=(T f)                         { *this = mul(*this, f); return *this; }

  udMatrix4x4<T>& transpose();
  T determinant();
  udMatrix4x4<T>& inverse();

  // static members
  static udMatrix4x4<T> identity();

  static udMatrix4x4<T> create(const T m[16]);
  static udMatrix4x4<T> create(T _00, T _10, T _20, T _30, T _01, T _11, T _21, T _31, T _02, T _12, T _22, T _32, T _03, T _13, T _23, T _33);
  static udMatrix4x4<T> create(const udVector4<T> &xColumn, const udVector4<T> &yColumn, const udVector4<T> &zColumn, const udVector4<T> &wColumn);
  template <typename U>
  static udMatrix4x4<T> create(const udMatrix4x4<U> &_m);

  static udMatrix4x4<T> rotationX(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationY(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationZ(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationAxis(const udVector3<T> &axis, T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationPYR(const udVector3<T> &pyr, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationQ(const udQuaternion<T> &q, const udVector3<T> &t = udVector3<T>::zero());

  static udMatrix4x4<T> translation(const udVector3<T> &t);

  static udMatrix4x4<T> perspective(T fovY, T aspectRatio, T near, T far);
  static udMatrix4x4<T> ortho(T left, T right, T bottom, T top, T near = T(0), T far = T(1));
  static udMatrix4x4<T> orthoForScreeen(T width, T height, T near = T(0), T far = T(1));
};
template <typename T>
udMatrix4x4<T> operator *(T f, const udMatrix4x4<T> &m) { return m*f; }


// typedef's for typed vectors/matices
typedef udVector4<float>  udFloat4;
typedef udVector3<float>  udFloat3;
typedef udVector2<float>  udFloat2;
typedef udVector4<double> udDouble4;
typedef udVector3<double> udDouble3;
typedef udVector2<double> udDouble2;

typedef udMatrix4x4<float>  udFloat4x4;
typedef udMatrix4x4<double> udDouble4x4;
//typedef udMatrix3x4<float>  udFloat3x4;
//typedef udMatrix3x4<double> udDouble3x4;
//typedef udMatrix3x3<float>  udFloat3x3;
//typedef udMatrix3x3<double> udDouble3x3;

typedef udQuaternion<float>  udFloatQuat;
typedef udQuaternion<double>  udDoubleQuat;


// unit tests
udResult udMath_Test();

#include "udMath_Inl.h"

#endif // __cplusplus

#endif // UDMATH_H
