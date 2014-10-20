#ifndef UDDEBUG_H
#define UDDEBUG_H

// *********************************************************************
// Outputs a string to debug console
// *********************************************************************
void udDebugPrintf(const char *format, ...);

class udTrace
{
public:
  udTrace(const char *);
  ~udTrace();
  static void Message(const char *pFormat, ...); // Print a message at the indentation level of the current trace

  const char *functionName;
  udTrace *next;
  static udTrace *head;
  static int depth;
};

template <typename T>
inline void udTrace_Variable(const char *pName, T value) { udTrace::Message("%s has unknown type", pName); }
template <typename T>
inline void udTrace_Variable(const char *pName, const T *value) { udTrace::Message("%s = %p", pName, value); }
template <>
inline void udTrace_Variable<int>(const char *pName, int value) { udTrace::Message("%s = %d (int)", pName, value); }
template <>
inline void udTrace_Variable<double>(const char *pName, double value) { udTrace::Message("%s = %f (float)", pName, value); }
template <>
inline void udTrace_Variable<float>(const char *pName, float value) { udTrace::Message("%s = %f (double)", pName, value); }
template <>
inline void udTrace_Variable<bool>(const char *pName, bool value) { udTrace::Message("%s = %s", pName, value ? "true" : "false"); }



#if UD_DEBUG

# define UDTRACE_ON     0
# define UDASSERT_ON    1
# define UDRELASSERT_ON 1

#elif UD_RELEASE

# define UDTRACE_ON     0
# define UDASSERT_ON    0
# define UDRELASSERT_ON 1

#endif

#if defined(__GNUC__)
# if UD_DEBUG
#   include <signal.h>
#   define __debugbreak() raise(SIGTRAP)
#   define DebugBreak() raise(SIGTRAP)
# else
#   define __debugbreak()
#   define DebugBreak()
# endif
#endif


#if UDTRACE_ON
# define UDTRACE() udTrace __udtrace##__LINE__(__FUNCTION__)
# define UDTRACE_SCOPE(id) udTrace __udtrace##__LINE__(id)
# define UDTRACE_MESSAGE(format,...) udTrace::Message(format,__VA_ARGS__)
# define UDTRACE_VARIABLE(var) udTrace_Variable(#var, var)
#else
# define UDTRACE()
# define UDTRACE_SCOPE(id)
# define UDTRACE_MESSAGE(format,...)
# define UDTRACE_VARIABLE(var)
#endif // UDTRACE_ON

// TODO: Make assertion system handle pop-up window where possible
#if UDASSERT_ON
# define UDASSERT(condition, ...) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(__VA_ARGS__); DebugBreak(); udDebugPrintf("\n"); } }
# define IF_UDASSERT(x) x
#else
# define UDASSERT(condition, ...) // TODO: Make platform-specific __assume(condition)
# define IF_UDASSERT(x)
#endif // UDASSERT_ON

#if UDRELASSERT_ON
# define UDRELASSERT(condition, ...) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(__VA_ARGS__); DebugBreak(); udDebugPrintf("\n"); } }
# define IF_UDRELASSERT(x) x
#else
# define UDRELASSERT(condition, ...) // TODO: Make platform-specific __assume(condition)
# define IF_UDRELASSERT(x)
#endif //UDRELASSERT_ON

#define UDCOMPILEASSERT(a_condition, a_error) typedef char UDCOMPILEASSERT##a_error[(a_condition)?1:-1]


#if UD_DEBUG
# define OUTPUT_ERROR_STRINGS (1)
#else
# define OUTPUT_ERROR_STRINGS (0)
#endif

#define UD_BREAK_ON_ERROR_STRING (0)

#if OUTPUT_ERROR_STRINGS
# if UD_BREAK_ON_ERROR_STRING
#   define __breakOnErrorString() __debugbreak()
# else
#   define __breakOnErrorString()
# endif 
# define PRINT_ERROR_STRING(...) do {udDebugPrintf("%s : ", __FUNCTION__); udDebugPrintf(__VA_ARGS__); __breakOnErrorString(); } while (false)

#else
# define PRINT_ERROR_STRING(...)
#endif

#endif // UDDEBUG_H
