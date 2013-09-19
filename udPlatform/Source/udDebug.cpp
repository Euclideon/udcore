#include "udDebug.h"
#include "udPlatformUtil.h"

//#define PRINTF_TRACE

// TODO: Use TLS so there is one stack per thread
udTrace *udTrace::head = nullptr;
int udTrace::depth = 0;


// ***************************************************************************************
udTrace::udTrace(const char *a_functionName)
{
  functionName = a_functionName;
  next = head;
  head = this;
#ifdef PRINTF_TRACE
  udDebugPrintf("%*.s Entering %s\n", depth*2, "                                                ", functionName);
#endif
  ++depth;
}

// ***************************************************************************************
udTrace::~udTrace()
{
  --depth;
  head = next;
#ifdef PRINTF_TRACE
  udDebugPrintf("%*.s Exiting  %s\n", depth*2, "                                                ", functionName);
#endif
}
