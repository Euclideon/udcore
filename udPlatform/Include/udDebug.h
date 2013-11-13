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

  const char *functionName;
  udTrace *next;
  static udTrace *head;
  static int depth;
};


#if UD_DEBUG

# define UDTRACE_ON     (0)
# define UDASSERT_ON    (1)
# define UDRELASSERT_ON (1)

#elif UD_RELEASE

# define UDTRACE_ON     (0)
# define UDASSERT_ON    (0)
# define UDRELASSERT_ON (1)

#endif


#if UDTRACE_ON
# define UDTRACE() udTrace __udtrace##__LINE__(__FUNCTION__)
#else
# define UDTRACE()
#endif // UDTRACE_ON

// TODO: Make assertion system handle pop-up window where possible
#if UDASSERT_ON
# define UDASSERT(condition, message) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(message); DebugBreak(); } }
#else
# define UDASSERT(condition, message) // TODO: Make platform-specific __assume(condition)
#endif // UDASSERT_ON

#if UDRELASSERT_ON
# define UDRELASSERT(condition, message) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(message); /*DebugBreak();*/ } }
#else
# define UDRELASSERT(condition, message) // TODO: Make platform-specific __assume(condition)
#endif //UDRELASSERT_ON

#define UDCOMPILEASSERT(a_condition, a_error) typedef char UDCOMPILEASSERT##a_error[(a_condition)?1:-1]

#endif // UDDEBUG_H
