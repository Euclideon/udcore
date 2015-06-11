#include <stdio.h>
#include "udString.h"

#if defined(_MSC_VER)
// _CRT_SECURE_NO_WARNINGS warning (vsprintf_s)
# pragma warning(disable: 4996)
#endif

// 1 = alpha, 2 = numeric, 4 = white, 8 = newline
const char s_charDetails[256] =
{
  0,0,0,0,0,0,0,0,0,4,8,0,0,8,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

udRCString udRCString::format(const char *pFormat, ...)
{
  va_list args;
  va_start(args, pFormat);

  size_t len = vsprintf(nullptr, pFormat, args) + 1;
  len = numToAlloc(len);

  udRCString r;
  r.rc = (udRC*)udAlloc(sizeof(udRC) + len);
  r.rc->refCount = 1;
  r.rc->allocatedCount = len;
  r.ptr = ((const char*)r.rc)+sizeof(udRC);
#if UDPLATFORM_NACL
  r.length = vsprintf((char*)r.ptr, pFormat, args);
#else
  r.length = vsnprintf((char*)r.ptr, len, pFormat, args);
#endif

  va_end(args);

  return r;
}

int64_t udString::parseInt(bool bDetectBase, int base) const
{
  udString s = trim(true, false);
  if (s.length == 0)
    return 0; // this isn't really right!

  int number = 0;

  if (base == 16 || (bDetectBase && (s.eqi("0x") || s.eq("$"))))
  {
    // hex number
    if (s.eqi("0x"))
      s.stripFront(2);
    else if (s.eq("$"))
      s.popFront();

    while (s.length > 0)
    {
      int digit = s.popFront();
      if (!isHex(digit))
        return number;
      number <<= 4;
      number += isNumeric(digit) ? digit - '0' : (digit|0x20) - 'a' + 10;
    }
  }
  else if (base == 2 || (bDetectBase && s.eqi("b")))
  {
    if (s.eqi("b"))
      s.popFront();

    while (s.length > 0 && (s.ptr[0] == '0' || s.ptr[0] == '1'))
    {
      number <<= 1;
      number |= s.ptr[0] - '0';
    }
  }
  else if (base == 10)
  {
    // decimal number
    bool neg = false;
    if (s.ptr[0] == '-' || s.ptr[0] == '+')
    {
      neg = s.ptr[0] == '-';
      s.popFront();
    }

    while (s.length > 0)
    {
      unsigned char c = s.popFront();
      if (!isNumeric(c))
        break;
      number = number*10 + c - '0';
    }
    if (neg)
      number = -number;
  }

  return number;
}

double udString::parseFloat() const
{
  udString s = trim(true, false);
  if (s.length == 0)
    return 0; // this isn't really right!

  int64_t number = 0;
  double frac = 1;

  // floating poiont number
  bool neg = false;
  if (s.ptr[0] == '-' || s.ptr[0] == '+')
  {
    neg = s.ptr[0] == '-';
    s.popFront();
  }

  bool bHasDot = false;
  while (s.length > 0)
  {
    int digit = s.popFront();
    if (!isNumeric(digit) && (bHasDot || digit != '.'))
      break;
    if (digit == '.')
      bHasDot = true;
    else
    {
      number = number*10 + digit - '0';
      if (bHasDot)
        frac *= 0.1f;
    }
  }

  if (neg)
    number = -number;

  return (double)number * frac;
}

udRCString udRCString::concat(udString *pStrings, size_t numStrings)
{
  // calculate total length
  size_t len = 0;
  for (size_t i = 0; i < numStrings; ++i)
    len += pStrings[i].length;

  // allocate a new udRCString
  udRC *pRC = (udRC*)udAlloc(sizeof(udRC) + sizeof(char)*len);
  pRC->refCount = 0;
  pRC->allocatedCount = len;
  char *ptr = (char*)(pRC + 1);

  // concatenate the strings
  char *pC = ptr;
  for (size_t i = 0; i < numStrings; ++i)
  {
    for (size_t j = 0; j < pStrings[i].length; ++j)
      *pC++ = pStrings[i].ptr[j];
  }

  return udRCString(ptr, len, pRC);
}


udResult udSlice_Test()
{
  // usSlice<> tests
  int i[100];

  udSlice<int> i1(i, 100);        // slice from array
  udSlice<const int> ci1(i, 100); // const slice from mutable array
  ci1 = i1;                       // assign mutable to const

  ci1 == i1;                      // compare pointer AND length are equal
  ci1 != i1;                      // compare pointer OR length are not equal

  i1.eq(ci1);                     // test elements for equality

  short s[100];
  udSlice<short> s1(s, 100);

  i1.eq(s1);                      // test elements of different (compatible) types for equality (ie, int == short)

  auto slice = i1.slice(10, 20);  // 'slice' is a slice of i1 from elements [10, 20)  (exclusive: ie, [10, 19])
  slice[0];                       // element 0 from slice (ie, element 10 from i1)

  slice.empty();                  // ptr == nullptr || length == 0


  // udFixedSlice<>
  udFixedSlice<int> s_i(i1);
  udFixedSlice<const int> s_ci(s1);

  udSlice<int> s_slice = s_i.slice(1, 3); // slices of udFixedSlice are not owned; they die when the parent allocation dies


  // udRCSlice<> tests
  udRCSlice<int> rc_i1(i1);       // rc_i1 is an allocated, ref-counted copy of the slice i1
  udRCSlice<const int> rc_ci1(s1);// rc_ci1 initialised from different (compatible) types (ie, short -> int, float -> double)

  // TODO: assign mutable RCSlice to const RCSlice without copy, should only bump RC

  auto rc_i2 = rc_i1;             // just inc the ref count

  auto rc_slice = rc_i1.slice(10, 20);  // slice increments the rc too

  rc_i1 = rc_slice;               // assignment between RC slices of the same array elide rc fiddling; only updates the offset/length

  i1 == rc_i2;                    // comparison of udSlice and udRCSlice; compares ptr && length as usual

  rc_i1.eq(i1);                   // element comparison between udSlice and udRCSlice

  for (auto i : i1)
  {
    // iterate the elements in i1
    //...
  }

  return udR_Success;
}

void receivesString(udString)
{
}
void receivesCString(const char*)
{
}

udResult udString_Test()
{
  // udString
  char buffer[] = "world";

  udString s1 = "hello";        // initialise from string
  udString s2(buffer, 3);       // initialise to sub-string; ie, "wor"

  s1.eq(s2);                    // strcmp() == 0
  s1.eqi(s2);                   // case insensitive; stricmp() == 0
  s1.eq("hello");               // compare with c-string

  udRCSlice<wchar_t> wcs(s1);   // init w_char string from c-string! yay unicode! (except we probably also want to decode utf-8...)

  wcs.eq(s1);                   // compare wide-char and ascii strings

  auto subStr = s1.slice(1, 4); // string slice; "ell"

  s2.toStringz(buffer, sizeof(buffer)); // write udString to c-string


  // udFixedString
  udFixedString<64> s_s1(s1);
  udString s_slice = s_s1.slice(1, 4); // slices of udFixedSlice are not owned; they die when the parent allocation dies

  s_s1.eqi("HELLO");            // string comparison against string literals

  receivesString(s_s1);         // pass to functions

  s_s1.reserve(100);            // reserve a big buffer
  UDASSERT(s_s1.eq(s1), "!");   // the existing contents is preserved

  s_s1.concat(s1, "!!", udString("world"));


  // udRCString
  udRCString rcs1(s1);          // RC string initialised from some slice
  udRCString rcs2("string");    // also from literal
  udRCString rcs3(buffer, 4);   // also from c-string (and optionally a slice thereof)

  receivesString(rcs1);         // pass to functions

  udFixedString<64> ss2 = rcs2; // stack string takes copy of a udRCString
  udRCString rcs4 = ss2;        // rc strings take copy of stack strings too

  rcs1 == s1;                   // compare udRCString and udString pointers

  rcs1.eqi(s1);                 // string comparison works too between udRCString and udString

//  udRCString::format("Format: %s", "hello");  // create from format string

//  char temp[256];
//  fopen(rcs1.toStringz(temp, sizeof(temp)), "ro"); // write udRCString to c-string, for passing to OS functions or C api's
                                                   // unlike c_str(), user supplies buffer (saves allocations)

  udRCString r2 = udRCString::concat(s1, "!!", rcs1, udString("world"), s_s1);
//  udRCString::format("x{1}_{2}", "10", "200");

  receivesCString(r2.toStringz());

  return udR_Success;
}
