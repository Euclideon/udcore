#if !defined(_UD_SLICE)
#define _UD_SLICE

#include "udPlatform.h"
#include "udResult.h"

// slices are bounded arrays, unlike C's conventional unbounded pointers (typically, with separate length stored in parallel)
// no attempt is made to create a one-size-fits-all implementation, as it is recognised that usages offer distinct advantages/disadvantages
// slise is the basis of the suite however, and everything is based on udSlice. derived types address specifics in usage and/or ownership patterns

// declare an iterator so it works with standard range pased functions (foreach!)
template<typename T>
class udIterator
{
  // TODO: this could be made safer by storing a ref to the parent slice, and keeping an offset
  T *pI;

public:
  udIterator(T *pI) : pI(pI) {}
  bool operator!=(udIterator<T> rh) const { return pI != rh.pI; } // compare
  udIterator<T> operator++() { ++pI; return *this; }              // increment
  T& operator*() const { return *pI; }                            // value
};

// udSlice does not retain ownership of it's memory, it is used for temporary ownership; working locals, function args, etc
template<typename T>
struct udSlice
{
  size_t length;
  T *ptr;

  // constructors
  udSlice<T>();
  udSlice<T>(T* ptr, size_t length);
  template <typename U> udSlice<T>(udSlice<U> rh);

  // assignment
  template <typename U> udSlice<T>& operator =(udSlice<U> rh);

  // contents
  T& operator[](size_t i) const;

  udSlice<T> slice(size_t first, size_t last) const;

  bool empty() const;

  // comparison
  bool operator ==(udSlice<const T> rh) const;
  bool operator !=(udSlice<const T> rh) const;

  template <typename U> bool eq(udSlice<U> rh) const;

  template <typename U> bool beginsWith(udSlice<U> rh) const;
  template <typename U> bool endsWith(udSlice<U> rh) const;

  // iterators
  udIterator<T> begin() const;
  udIterator<T> end() const;

  // useful functions
  T& front() const;
  T& back() const;
  T& popFront();
  T& popBack();
  udSlice<T> stripFront(size_t n);
  udSlice<T> stripBack(size_t n);

  ptrdiff_t offsetOf(T c) const;
  ptrdiff_t offsetOfLast(T c) const;

  bool canFind(T c) const;

  udSlice<T> find(T c) const;
  udSlice<T> findBack(T c) const;

  template<bool skipEmptyTokens = false>
  udSlice<T> popToken(udSlice<T> delimiters);

  template<bool skipEmptyTokens = false>
  udSlice<udSlice<T>> tokenise(udSlice<udSlice<T>> tokens, udSlice<T> delimiters);
};


// udFixedSlice introduces static-sized and/or stack-based ownership. this is useful anywhere that fixed-length arrays are appropriate
// udFixedSlice will fail-over to an allocated buffer if the contents exceed the fixed size
template <typename T, size_t Count = 64>
struct udFixedSlice : public udSlice <T>
{
  size_t numAllocated;
  union
  {
    char buffer[sizeof(T) * Count];
    T *pAllocation;
  };

  // constructors
  udFixedSlice<T, Count>();
  udFixedSlice<T, Count>(udFixedSlice<T, Count> &&rval);
  template <typename U> udFixedSlice<T, Count>(U *ptr, size_t length);
  template <typename U> udFixedSlice<T, Count>(udSlice<U> slice);
  ~udFixedSlice<T, Count>();

  void reserve(size_t count);

  // assignment
  udFixedSlice<T, Count>& operator =(udFixedSlice<T, Count> &&rval);
  template <typename U> udFixedSlice<T, Count>& operator =(udSlice<U> rh);

  // manipulation
  void clear();
  template<typename... Things> udFixedSlice<T, Count>& concat(const Things&... things);

  template <typename U> udFixedSlice<T, Count>& pushBack(const U &item);

  udSlice<T> getBuffer() const;

protected:
  static size_t numToAlloc(size_t i);
};


// udRCSlice is a reference counted slice, used to retain ownership of some memory, but will not duplicate when copies are made
// useful for long-living data that doesn't consume a fixed amount of their containing struct
// also useful for sharing between systems, passing between threads, etc.
// slices of udRCSlice's increment the RC, so that slices can outlive the original owner, but without performing additional allocations and copies
template <typename T>
struct udRCSlice : public udSlice<T>
{
  struct udRC
  {
    size_t refCount;
    size_t allocatedCount;
  } *rc;

  // constructors
  udRCSlice<T>();
  udRCSlice<T>(udRCSlice<T> &&rval);
  udRCSlice<T>(const udRCSlice<T> &rcslice);
  template <typename U> udRCSlice<T>(U *ptr, size_t length);
  template <typename U> udRCSlice<T>(udSlice<U> slice);
  ~udRCSlice<T>();

  // static constructors (make proper constructors?)
  template<typename... Things> static udRCSlice<T> concat(const Things&... things);

  // assignment
  udRCSlice<T>& operator =(const udRCSlice<T> &rh);
  udRCSlice<T>& operator =(udRCSlice<T> &&rval);
  template <typename U> udRCSlice<T>& operator =(udSlice<U> rh);

  // contents
  udRCSlice<T> slice(size_t first, size_t last) const;

protected:
  udRCSlice<T>(T *ptr, size_t length, udRC *rc);
  static size_t numToAlloc(size_t i);
  template <typename U> static udSlice<T> alloc(U *ptr, size_t length);
  template <typename U> void init(U *ptr, size_t length);
};

// unit tests
udResult udSlice_Test();


#include "udSlice.inl"

#endif // _UD_SLICE
