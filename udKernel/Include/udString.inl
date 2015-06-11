#include "udPlatform.h"
#include "udPlatformUtil.h"

#include <string.h>
#include <stdarg.h>

class udCString
{
  friend struct udString;
public:
  operator const char*() const { return pCStr; }
  ~udCString()
  {
    udFree(pCStr);
  }

private:
  const char *pCStr;
  udCString(udString str)
  {
    pCStr = (const char*)udAlloc(str.length + 1);
    udStrncpy((char*)pCStr, str.length + 1, str.ptr, str.length);
  }
};


// udString
inline udString::udString()
{}

inline udString::udString(const char *ptr, size_t length)
  : udSlice<const char>(ptr, length)
{}

template<typename C>
inline udString::udString(udSlice<C> rh)
  : udSlice<const char>(rh)
{}

inline udString::udString(const char *pString)
  : udSlice<const char>(pString, pString ? strlen(pString) : 0)
{}

inline udString& udString::operator =(udSlice<const char> rh)
{
  length = rh.length;
  ptr = rh.ptr;
  return *this;
}

inline udString& udString::operator =(const char *pString)
{
  ptr = pString;
  length = pString ? strlen(pString) : (size_t)0;
  return *this;
}

inline udString udString::slice(size_t first, size_t last) const
{
  UDASSERT(last <= length && first <= last, "Index out of range!");
  return udString(ptr + first, last - first);
}

inline bool udString::eq(udString rh) const
{
  return udSlice<const char>::eq(rh);
}
inline bool udString::eqi(udString rh) const
{
  if(length != rh.length)
    return false;
  for (size_t i = 0; i<length; ++i)
    if (toLower(ptr[i]) != toLower(rh.ptr[i]))
      return false;
  return true;
}

inline bool udString::beginsWith(udString rh) const
{
  return udSlice<const char>::beginsWith(rh);
}
inline bool udString::endsWith(udString rh) const
{
  return udSlice<const char>::endsWith(rh);
}
inline bool udString::beginsWithInsensitive(udString rh) const
{
  if (length < rh.length)
    return false;
  return slice(0, rh.length).eqi(rh);
}
inline bool udString::endsWithInsensitive(udString rh) const
{
  if (length < rh.length)
    return false;
  return slice(length - rh.length, length).eqi(rh);
}

inline char* udString::toStringz(char *pBuffer, size_t bufferLen) const
{
  size_t len = length < bufferLen-1 ? length : bufferLen-1;
  memcpy(pBuffer, ptr, len);
  pBuffer[len] = 0;
  return pBuffer;
}

inline udCString udString::toStringz() const
{
  return udCString(*this);
}

inline udString udString::trim(bool front, bool back) const
{
  size_t first = 0, last = length;
  if (front)
  {
    while (isWhitespace(ptr[first]) && first < length)
      ++first;
  }
  if (back)
  {
    while (last > first && isWhitespace(ptr[last - 1]))
      --last;
  }
  return slice(first, last);
}

template<bool skipEmptyTokens>
inline udString udString::popToken(udString delimiters)
{
  return udSlice<const char>::popToken<skipEmptyTokens>(delimiters);
}

template<bool skipEmptyTokens>
inline udSlice<udString> udString::tokenise(udSlice<udString> tokens, udString delimiters)
{
  return udSlice<const char>::tokenise<skipEmptyTokens>(tokens, delimiters);
}


// udFixedString
template<size_t Size>
inline udFixedString<Size>::udFixedString()
{}

template<size_t Size>
inline udFixedString<Size>::udFixedString(udFixedSlice<const char, Size> &&rval)
  : udFixedSlice<char, Size>(rval)
{}

template<size_t Size>
template <typename U>
inline udFixedString<Size>::udFixedString(U *ptr, size_t length)
  : udFixedSlice<char, Size>(ptr, length)
{}

template<size_t Size>
template <typename U>
inline udFixedString<Size>::udFixedString(udSlice<U> slice)
  : udFixedSlice<char, Size>(slice)
{}

template<size_t Size>
inline udFixedString<Size>::udFixedString(const char *pString)
  : udFixedSlice<char, Size>(pString, pString ? strlen(pString) : 0)
{}

template<size_t Size>
inline udFixedString<Size>& udFixedString<Size>::operator =(udFixedSlice<const char, Size> &&rval)
{
  udFixedSlice<char, Size>::operator=(rval);
  return *this;
}
template<size_t Size>
template <typename U>
inline udFixedString<Size>& udFixedString<Size>::operator =(udSlice<U> rh)
{
  udFixedSlice<char, Size>::operator=(rh);
  return *this;
}
template<size_t Size>
inline udFixedString<Size>& udFixedString<Size>::operator =(const char *pString)
{
  udFixedSlice<char, Size>::operator=(pString, pString ? strlen(pString) : 0);
  return *this;
}

template<size_t Size>
inline udFixedString<Size> udFixedString<Size>::format(const char *pFormat, ...)
{
  va_list args;
  va_start(args, pFormat);

  size_t len = vsprintf(nullptr, pFormat, args) + 1;

  udFixedString<Size> r;
  r.length = len;
  if (len < Size)
  {
    r.numAllocated = 0;
    r.ptr = (const char*)r.buffer;
  }
  else
  {
    r.numAllocated = udFixedSlice<char, Size>::numToAlloc(len + 1);
    r.ptr = (const char*)udAlloc(sizeof(const char) * r.numAllocated);
  }

  r.length = vsnprintf((char*)r.ptr, len, pFormat, args);

  va_end(args);

  return r;
}

template<size_t Size>
inline udCString udFixedString<Size>::toStringz() const
{
  return ((udString*)this)->toStringz();
}

template<size_t Size>
template<typename... Strings>
inline udFixedString<Size>& udFixedString<Size>::concat(const Strings&... strings)
{
  // flatten the args to an array
  udString args[] = { udString(strings)... };

  // call the (non-template) array version
  concat(args, sizeof(args) / sizeof(args[0]));
  return *this;
}

template<size_t Size>
inline void udFixedString<Size>::concat(udString *pStrings, size_t numStrings)
{
  // calculate total length
  size_t len = this->length;
  for (size_t i = 0; i < numStrings; ++i)
    len += pStrings[i].length;

  this->reserve(len);

  // concatenate the strings
  char *pC = this->ptr + this->length;
  for (size_t i = 0; i < numStrings; ++i)
  {
    for (size_t j = 0; j < pStrings[i].length; ++j)
      *pC++ = pStrings[i].ptr[j];
  }

  this->length = len;
}


// udRCString
inline udRCString::udRCString()
{}

inline udRCString::udRCString(udRCSlice<const char> &&rval)
  : udRCSlice<const char>(rval)
{}

inline udRCString::udRCString(const udRCSlice<const char> &rcstr)
  : udRCSlice<const char>(rcstr)
{}

template <typename U>
inline udRCString::udRCString(U *ptr, size_t length)
  : udRCSlice<const char>(ptr, length)
{}

template <typename U>
inline udRCString::udRCString(udSlice<U> slice)
  : udRCSlice<const char>(slice)
{}

inline udRCString::udRCString(const char *pString)
  : udRCSlice<const char>(pString, pString ? strlen(pString) : 0)
{}

inline udRCString& udRCString::operator =(const udRCSlice<const char> &rh)
{
  udRCSlice<const char>::operator=(rh);
  return *this;
}
inline udRCString& udRCString::operator =(udRCSlice<const char> &&rval)
{
  udRCSlice<const char>::operator=(rval);
  return *this;
}
template <typename U>
inline udRCString& udRCString::operator =(udSlice<U> rh)
{
  *this = udRCSlice<U>(rh);
  return *this;
}
inline udRCString& udRCString::operator =(const char *pString)
{
  *this = udRCString(pString);
  return *this;
}

inline udRCString udRCString::slice(size_t first, size_t last) const
{
  UDASSERT(last <= length && first <= last, "Index out of range!");
  return udRCString(ptr + first, last - first, rc);
}

inline udCString udRCString::toStringz() const
{
  return ((udString*)this)->toStringz();
}

inline udRCString::udRCString(const char *ptr, size_t length, udRC *rc)
  : udRCSlice<const char>(ptr, length, rc)
{}

template<typename... Strings>
inline udRCString udRCString::concat(const Strings&... strings)
{
  // flatten the args to an array
  udString args[] = { udString(strings)... };

  // call the (non-template) array version
  return concat(args, sizeof(args) / sizeof(args[0]));
}
/*
template<typename... Things>
inline udRCString udRCString::format(const char *pFormat, const Things&... things)
{
  udString args[] = { udString(things)... };
  size_t numArgs = sizeof(args) / sizeof(args[0]);

//  for(

  size_t len = 0;
  const char *pC = pFormat;
  while (*pC)
  {
    if (*pC == '\\' && pC[1] != 0)
      ++pC;
    else if (*pC == '{')
    {
      ++pC;
      while(isNumeric(*pC))
      size_t index;
      while (*pC && *pC != '}')
        ++pC;
      continue;
    }
    ++len;
    ++pC;
  }

  return udRCString();
}
*/
