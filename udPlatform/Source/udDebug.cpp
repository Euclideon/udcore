#define _CRT_SECURE_NO_WARNINGS
#include "udDebug.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <stdarg.h>

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

UDTHREADLOCAL udTrace *udTrace::head = NULL;
UDTHREADLOCAL int udTrace::depth = 0;
UDTHREADLOCAL int udTrace::threadId = 0;
static udInterlockedInt32 nextThreadId;

// ***************************************************************************************
udTrace::udTrace(const char *a_functionName)
{
  if (!threadId)
  {
    threadId = nextThreadId++;
#if UDTRACE_ON
    udDebugPrintf("%02d> Thread started with %s\n", threadId, a_functionName);
#endif
  }
  functionName = a_functionName;
  next = head;
  head = this;
  entryPrinted = false;
#if UDTRACE_ON > 1
  udDebugPrintf("%02d>%*.s Entering %s\n", threadId, depth*2, "", functionName);
  entryPrinted = true;
#endif
  ++depth;
}

// ***************************************************************************************
udTrace::~udTrace()
{
  --depth;
  head = next;
  if (entryPrinted)
    udDebugPrintf("%02d>%*.s Exiting  %s\n", threadId, depth*2, "", functionName);
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
  if (head && !head->entryPrinted)
  {
    const char *pParent0 = "";
    const char *pParent1 = "";
    const char *pParent2 = "";
    if (head->next)
    {
      pParent0 = head->next->functionName;
      if (head->next->next)
      {
        pParent1 = head->next->next->functionName;
        if (head->next->next->next)
          pParent1 = head->next->next->next->functionName;
      }
    }
    udDebugPrintf("%02d>%*.s Within  %s->%s->%s->%s\n", threadId, (depth-1)*2, "", pParent2, pParent1, pParent0, head->functionName);
    head->entryPrinted = true;
  }
  udDebugPrintf("%02d>%*.s %s\n", threadId, head ? head->depth*2 : 0, "", buffer);
}
