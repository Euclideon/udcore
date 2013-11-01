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


#if defined(UD_DEBUG)
//#define UDTRACE_ON
#define UDASSERT_ON
#define UDRELASSERT_ON
#elif defined(UD_RELEASE)
#define UDRELASSERT_ON
#endif


#ifdef UDTRACE_ON
#define UDTRACE() udTrace __udtrace##__LINE__(__FUNCTION__)
#else
#define UDTRACE()
#endif

// TODO: Make assertion system handle pop-up window where possible
#ifdef UDASSERT_ON
#define UDASSERT(condition, message) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(message); DebugBreak(); } }
#else
#define UDASSERT(condition, message) // TODO: Make platform-specific __assume(condition)
#endif

#ifdef UDRELASSERT_ON
#define UDRELASSERT(condition, message) { bool testCondition = !!(condition); if (!testCondition) { udDebugPrintf(message); /*DebugBreak();*/ } }
#else
#define UDRELASSERT(condition, message) // TODO: Make platform-specific __assume(condition)
#endif

#define UDCOMPILEASSERT(a_condition, a_error) typedef char UDCOMPILEASSERT##a_error[(a_condition)?1:-1]

#endif // UDDEBUG_H
