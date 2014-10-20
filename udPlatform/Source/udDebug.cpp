#define _CRT_SECURE_NO_WARNINGS
#include "udDebug.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <stdarg.h>

//#define PRINTF_TRACE    // Very slow but useful when tracking exceptions that don't stop in the debugger


// *********************************************************************
void udDebugPrintf(const char *format, ...)
{
  va_list args;
  char buffer[300];

  va_start(args, format);
#if UDPLATFORM_NACL
  vsprintf(buffer, format, args);
#else
  vsnprintf(buffer, sizeof(buffer), format, args);
#endif
#ifdef _WIN32
  OutputDebugStringA(buffer);
#else
  fprintf(stderr, "%s", buffer);
#endif
}

// TODO: Use TLS so there is one stack per thread
udTrace *udTrace::head = NULL;
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

// ***************************************************************************************
void udTrace::Message(const char *pFormat, ...)
{
  va_list args;
  char buffer[300];

  va_start(args, pFormat);
#if UDPLATFORM_NACL
  vsprintf(buffer, pFormat, args);
#else
  vsnprintf(buffer, sizeof(buffer), pFormat, args);
#endif
  udDebugPrintf("%*.s %s\n", head->depth*2, "", buffer);
}
