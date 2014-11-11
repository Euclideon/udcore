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
  int prefix = 0;

#if UDDEBUG_PRINTF_SHOW_THREAD_ID
  udSprintf(buffer, sizeof(buffer), "%02d>", udTrace::GetThreadId());
  prefix = (int)strlen(buffer);
#endif
  va_start(args, format);
#if UDPLATFORM_NACL
  vsprintf(buffer + prefix, format, args);
#else
  vsnprintf(buffer + prefix, sizeof(buffer)-prefix, format, args);
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
int udTrace::GetThreadId()
{
  if (!threadId)
    threadId = nextThreadId++;

  return threadId;
}

// ***************************************************************************************
udTrace::udTrace(const char *a_functionName, int traceLevel)
{
  if (!threadId)
  {
    threadId = nextThreadId++;
    if (traceLevel)
      udDebugPrintf("%02d> Thread started with %s\n", threadId, a_functionName);
  }
  functionName = a_functionName;
  next = head;
  head = this;
  entryPrinted = false;
  if (traceLevel > 1)
  {
    udDebugPrintf("%02d>%*.s Entering %s\n", threadId, depth*2, "", functionName);
    entryPrinted = true;
  }
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


// ***************************************************************************************
void udTrace_Memory(const char *pName, void *pMem, int length, int line)
{
  char format[100];
  udTrace::Message("Dump of memory for %s (%d bytes at %p, line #%d)", pName, length, pMem, line);
  unsigned char p[16];
  udStrcpy(format, sizeof(format), "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x");
  while (length > 0)
  {
    int n = (length > 16) ? 16 : length;
    memset(p, 0, sizeof(p));
    memcpy(p, pMem, n);
    format[n * 5] = 0; // nul terminate in the correct spot
    udTrace::Message(format, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[14], p[14], p[15]);
    pMem = ((char*)pMem)+n;
    length -= n;
  }
}
