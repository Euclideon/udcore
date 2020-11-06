#ifndef UDCALLBACK_H
#define UDCALLBACK_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Samuel Surtees, November 2019
//
// Very simple callback API to allow for lambdas with captures.
//

#include "udNew.h"
#include <utility>
#include <type_traits>

template<typename T>
struct udAbstractCallback;

template<typename Result, typename ...Args>
struct udAbstractCallback<Result(Args...)>
{
  virtual Result call(Args... args) const = 0;
  virtual ~udAbstractCallback() = default;
};

template<typename T, typename U>
struct udConcreteCallback;

template<typename T, typename Result, typename ...Args>
struct udConcreteCallback<T, Result(Args...)> : udAbstractCallback<Result(Args...)>
{
  T callback;
  explicit udConcreteCallback(T &&callback) : callback(std::move(callback)) {}
  Result call(Args... args) const override { return callback(args...); }
};

template<typename T>
struct udCallback;

template<typename Result, typename ...Args>
struct udCallback<Result(Args...)>
{
  uint8_t buffer[256];
  udAbstractCallback<Result(Args...)> *pPtr;

  udCallback() noexcept : buffer(), pPtr(nullptr) { }
  udCallback(std::nullptr_t) noexcept : buffer(), pPtr(nullptr) { }

  udCallback(const udCallback &other) { memcpy(buffer, other.buffer, sizeof(buffer)); if (other.pPtr) pPtr = (decltype(pPtr))buffer; else pPtr = nullptr; };
  udCallback(udCallback &&other) noexcept { memmove(buffer, other.buffer, sizeof(buffer)); if (other.pPtr) pPtr = (decltype(pPtr))buffer; else pPtr = nullptr; other.pPtr = nullptr; }

  template<typename T>
  udCallback(T callback)
  {
    UDCOMPILEASSERT(sizeof(T) <= sizeof(buffer), "Provided function is larger than buffer!");
    using udCallbackT = udConcreteCallback<T, Result(Args...)>;
    pPtr = new (buffer) udCallbackT(std::move(callback));
  }

  ~udCallback() { if (pPtr) pPtr->~udAbstractCallback(); }

  udCallback &operator=(const udCallback &other) { memcpy(buffer, other.buffer, sizeof(buffer)); if (other.pPtr) pPtr = (decltype(pPtr))buffer; else pPtr = nullptr; return *this; }
  udCallback &operator=(udCallback &&other) noexcept { memmove(buffer, other.buffer, sizeof(buffer)); if (other.pPtr) pPtr = (decltype(pPtr))buffer; else pPtr = nullptr; other.pPtr = nullptr; return *this; }
  udCallback &operator=(std::nullptr_t) noexcept { pPtr = nullptr; return *this; }
  template<typename T>
  udCallback &operator=(T callback)
  {
    UDCOMPILEASSERT(sizeof(T) <= sizeof(buffer), "Provided function is larger than buffer!");
    using udCallbackT = udConcreteCallback<T, Result(Args...)>;
    pPtr = new (buffer) udCallbackT(std::move(callback));
    return *this;
  }

  bool operator==(std::nullptr_t) noexcept { return pPtr == nullptr; }
  bool operator!=(std::nullptr_t) noexcept { return pPtr != nullptr; }

  Result operator()(Args... args) const { return pPtr->call(args...); }
  explicit operator bool() const noexcept { return pPtr != nullptr; }
};

#endif //UDCALLBACK_H
