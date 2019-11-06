#include "udNew.h"
#include <utility>

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
  udAbstractCallback<Result(Args...)> *pPtr;

  template<typename T>
  udCallback(T callback)
  {
    using udCallbackT = udConcreteCallback<T, Result(Args...)>;
    pPtr = udNew(udCallbackT, std::move(callback));
  }

  ~udCallback() { udDelete(pPtr); }
  Result operator()(Args... args) const { return pPtr->call(args...); }
};
