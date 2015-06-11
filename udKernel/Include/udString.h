#if !defined(_UD_STRING)
#define _UD_STRING

#include "udSlice.h"


extern const char s_charDetails[256];

#define isNewline(c) (s_charDetails[(uint8_t)c] & 8)
#define isWhitespace(c) (s_charDetails[(uint8_t)c] & 0xC)
#define isAlpha(c) (s_charDetails[(uint8_t)c] & 1)
#define isNumeric(c) (s_charDetails[(uint8_t)c] & 2)
#define isAlphaNumeric(c) ( s_charDetails[(uint8_t)c] & 3 )
#define isHex(c) (isAlphaNumeric(c) && (uint8_t(c)|0x20) <= 'F')

#define toLower(c) (isAlpha(c) ? c|0x20 : c)
#define toUpper(c) (isAlpha(c) ? c&~0x20 : c)

#if !defined(__cplusplus)

// C compilers just define udString as a simple struct
struct udString
{
  size_t length;
  const char *ptr;
};

#else

class udCString;

// udString is a layer above udSlice<>, defined for 'const char' (utf-8) and adds string-specific methods
// udString and udSlice<> are fully compatible

// udString implements an immutable string
// does not retain ownership; useful for local temporaries, passing as arguments, etc... analogous to using 'const char*'
// udString adds typical string functions, including conversion to zero-terminated strings for passing to foreign APIs, akin to c_str()
struct udString : public udSlice<const char>
{
  // constructors
  udString();
  udString(const char *ptr, size_t length);
  template<typename C>
  udString(udSlice<C> rh);
  udString(const char *pString);

  // assignment
  udString& operator =(udSlice<const char> rh);
  udString& operator =(const char *pString);

  // contents
  udString slice(size_t first, size_t last) const;

  // comparison
  bool eq(udString rh) const;
  bool eqi(udString rh) const;

  bool beginsWith(udString rh) const;
  bool endsWith(udString rh) const;
  bool beginsWithInsensitive(udString rh) const;
  bool endsWithInsensitive(udString rh) const;

  // c-string compatibility
  char* toStringz(char *pBuffer, size_t bufferLen) const;
  udCString toStringz() const;

  // useful functions
  udString trim(bool front = true, bool back = true) const;

  template<bool skipEmptyTokens = true>
  udString popToken(udString delimiters = " \t\r\n");
  template<bool skipEmptyTokens = true>
  udSlice<udString> tokenise(udSlice<udString> tokens, udString delimiters = " \t\r\n");

  int64_t parseInt(bool bDetectBase = true, int base = 10) const;
  double parseFloat() const;
};


// a static-length and/or stack-allocated string, useful for holding constructured temporaries (ie, target for sprintf/format)
// alternatively useful for holding a string as a member of an aggregate without breaking out to separate allocations
// useful in cases where 'char buffer[len]' is typically found
template<size_t Size>
struct udFixedString : public udFixedSlice<char, Size>
{
  // constructors
  udFixedString();
  udFixedString(udFixedSlice<const char, Size> &&rval);
  template <typename U>
  udFixedString(U *ptr, size_t length);
  template <typename U>
  udFixedString(udSlice<U> slice);
  udFixedString(const char *pString);

  // assignment
  udFixedString& operator =(udFixedSlice<const char, Size> &&rval);
  template <typename U>
  udFixedString& operator =(udSlice<U> rh);
  udFixedString& operator =(const char *pString);

  // comparison
  bool eq(udString rh) const                                                            { return ((udString*)this)->eq(rh); }
  bool eqi(udString rh) const                                                           { return ((udString*)this)->eqi(rh); }

  bool beginsWith(udString rh) const                                                    { return ((udString*)this)->beginsWith(rh); }
  bool endsWith(udString rh) const                                                      { return ((udString*)this)->endsWith(rh); }
  bool beginsWithInsensitive(udString rh) const                                         { return ((udString*)this)->beginsWithInsensitive(rh); }
  bool endsWithInsensitive(udString rh) const                                           { return ((udString*)this)->endsWithInsensitive(rh); }

  // construction
  static udFixedString format(const char *pFormat, ...);

  // modification
  template<typename... Strings> udFixedString& concat(const Strings&... strings);

  // c-string compatibility
  char* toStringz(char *pBuffer, size_t bufferLen) const                                { return ((udString*)this)->toStringz(pBuffer, bufferLen); }
  udCString toStringz() const;

  // useful functions
  udString trim(bool front = true, bool back = true) const                              { return ((udString*)this)->trim(front, back); }

  template<bool skipEmptyTokens = true>
  udString popToken(udString delimiters = " \t\r\n")                                    { return ((udString*)this)->popToken<skipEmptyTokens>(delimiters); }
  template<bool skipEmptyTokens = true>
  udSlice<udString> tokenise(udSlice<udString> tokens, udString delimiters = " \t\r\n") { return ((udString*)this)->tokenise<skipEmptyTokens>(tokens, delimiters); }

  int64_t parseInt(bool bDetectBase = true, int base = 10) const                        { return ((udString*)this)->parseInt(bDetectBase, base); }
  double parseFloat() const                                                             { return ((udString*)this)->parseFloat(); }

private:
  void concat(udString *pStrings, size_t numStrings);
};

// we'll typedef these such that the desired size compensates for the other internal members
typedef udFixedString<64 - sizeof(size_t)*3> udFixedString64;
typedef udFixedString<128 - sizeof(size_t)*3> udFixedString128;
typedef udFixedString<256 - sizeof(size_t)*3> udFixedString256;


// reference-counted allocated string, used to retain ownership of arbitrary length strings
// reference counting allows allocations to be shared between multiple owners
// useful in situations where std::string would usually be found, but without the endless allocation and copying or linkage problems
struct udRCString : public udRCSlice<const char>
{
  // constructors
  udRCString();
  udRCString(udRCSlice<const char> &&rval);
  udRCString(const udRCSlice<const char> &rcstr);
  template <typename U>
  udRCString(U *ptr, size_t length);
  template <typename U>
  udRCString(udSlice<U> slice);
  udRCString(const char *pString);

  // static constructors (make proper constructors?)
  template<typename... Strings> static udRCString concat(const Strings&... strings);
//  template<typename... Things> static udRCString format(const char *pFormat, const Things&...);

  // assignment
  udRCString& operator =(const udRCSlice<const char> &rh);
  udRCString& operator =(udRCSlice<const char> &&rval);
  template <typename U>
  udRCString& operator =(udSlice<U> rh);
  udRCString& operator =(const char *pString);

  // contents
  udRCString slice(size_t first, size_t last) const;

  // comparison
  bool eq(udString rh) const                                                            { return ((udString*)this)->eq(rh); }
  bool eqi(udString rh) const                                                           { return ((udString*)this)->eqi(rh); }

  bool beginsWith(udString rh) const                                                    { return ((udString*)this)->beginsWith(rh); }
  bool endsWith(udString rh) const                                                      { return ((udString*)this)->endsWith(rh); }
  bool beginsWithInsensitive(udString rh) const                                         { return ((udString*)this)->beginsWithInsensitive(rh); }
  bool endsWithInsensitive(udString rh) const                                           { return ((udString*)this)->endsWithInsensitive(rh); }

  // construction
  static udRCString format(const char *pFormat, ...);

  // c-string compatibility
  char* toStringz(char *pBuffer, size_t bufferLen) const                                { return ((udString*)this)->toStringz(pBuffer, bufferLen); }
  udCString toStringz() const;

  // useful functions
  udString trim(bool front = true, bool back = true) const                              { return ((udString*)this)->trim(front, back); }

  template<bool skipEmptyTokens = true>
  udString popToken(udString delimiters = " \t\r\n")                                    { return ((udString*)this)->popToken<skipEmptyTokens>(delimiters); }
  template<bool skipEmptyTokens = true>
  udSlice<udString> tokenise(udSlice<udString> tokens, udString delimiters = " \t\r\n") { return ((udString*)this)->tokenise<skipEmptyTokens>(tokens, delimiters); }

  int64_t parseInt(bool bDetectBase = true, int base = 10) const                        { return ((udString*)this)->parseInt(bDetectBase, base); }
  double parseFloat() const                                                             { return ((udString*)this)->parseFloat(); }

private:
  udRCString(const char *ptr, size_t length, udRC *rc);
  static udRCString concat(udString *pStrings, size_t numStrings);
};

// unit tests
udResult udString_Test();


#include "udString.inl"

#endif

#endif // _UD_STRING
