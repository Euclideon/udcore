#ifndef UDDEBUG_H
#define UDDEBUG_H

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
#elif defined(UD_RELEASE)
#endif


#ifdef UDTRACE_ON
#define UDTRACE() udTrace __udtrace##__LINE__(__FUNCTION__)
#else
#define UDTRACE()
#endif


#ifdef UDASSERT_ON
#define UDASSERT(condition) { if (!(condition)) DebugBreak(); }
#else
#define UDASSERT(condition) // TODO: Make platform-specific __assume(condition)
#endif

#endif // UDDEBUG_H
