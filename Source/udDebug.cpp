#include "udDebug.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#include <atomic>
#include <stdio.h>

void (*gpudDebugPrintfOutputCallback)(const char *pString) = nullptr;

// *********************************************************************
void udDebugPrintf(const char *format, ...)
{
  va_list args;
  char bufferStackMem[80];
  int prefix = 0;
  char *pBuffer = bufferStackMem;
  int bufferLen = (int)sizeof(bufferStackMem);
  int required = 0;

  static bool multiThreads = false;
  static int lastThread = -1;

  if (!multiThreads)
  {
    multiThreads = (lastThread != -1) && (lastThread != udTrace::GetThreadId());
    lastThread = udTrace::GetThreadId();
  }

  if (multiThreads)
  {
    udSprintf(pBuffer, bufferLen, "%02d>", udTrace::GetThreadId());
    prefix = (int)strlen(pBuffer);
  }

  va_start(args, format);
  required = udSprintfVA(pBuffer + prefix, bufferLen - prefix, format, args);
  va_end(args);
  if (required >= (bufferLen - prefix))
  {
    // The string is bigger than the temp on-stack buffer, so allocate a bigger one
    pBuffer[prefix] = 0;
    bufferLen = required + prefix + 1;
    char *pNewBuf = udAllocStack(char, bufferLen, udAF_None);
    if (!pNewBuf)
      return;
    udStrcpy(pNewBuf, bufferLen, pBuffer);
    pBuffer = pNewBuf;

    va_start(args, format);
    udSprintfVA(pBuffer + prefix, bufferLen - prefix, format, args);
    va_end(args);
  }

  if (gpudDebugPrintfOutputCallback)
  {
    gpudDebugPrintfOutputCallback(pBuffer);
  }
  else
  {
#ifdef _WIN32
    OutputDebugStringA(pBuffer);
#else
    fprintf(stderr, "%s", pBuffer);
#endif
  }
}

UDTHREADLOCAL udTrace *udTrace::head = NULL;
UDTHREADLOCAL int udTrace::depth = 0;
UDTHREADLOCAL int udTrace::threadId = -1;
static std::atomic<int32_t> nextThreadId;

// ***************************************************************************************
int udTrace::GetThreadId()
{
  if (threadId == -1)
    threadId = nextThreadId++;

  return threadId;
}

// ***************************************************************************************
udTrace::udTrace(const char *a_functionName, int traceLevel)
{
  if (threadId == -1)
  {
    threadId = nextThreadId++;
    if (traceLevel)
      udDebugPrintf("Thread %d started with %s\n", threadId, a_functionName);
  }
  functionName = a_functionName;
  next = head;
  head = this;
  entryPrinted = false;
  if (traceLevel > 1)
  {
    udDebugPrintf("%*.s Entering %s\n", depth * 2, "", functionName);
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
    udDebugPrintf("%*.s Exiting  %s\n", depth * 2, "", functionName);
}

// ***************************************************************************************
void udTrace::Message(const char *pFormat, ...)
{
  va_list args;
  char buffer[300];

  va_start(args, pFormat);
  udSprintfVA(buffer, pFormat, args);
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
    udDebugPrintf("%*.s Within  %s->%s->%s->%s\n", (depth - 1) * 2, "", pParent2, pParent1, pParent0, head->functionName);
    head->entryPrinted = true;
  }
  udDebugPrintf("%*.s %s\n", head ? head->depth * 2 : 0, "", buffer);
}

// ***************************************************************************************
void udTrace::ShowCallstack()
{
  udDebugPrintf("Callstack:\n");
  for (udTrace *p = head; p; p = p->next)
    udDebugPrintf("  %s\n", p->functionName);
}

// ***************************************************************************************
void udTrace_Memory(const char *pName, const void *pMem, int length, int line)
{
  char format[100];
  if (pName)
    udTrace::Message("Dump of memory for %s (%d bytes at %p, line #%d)", pName, length, pMem, line);
  unsigned char p[16];
  udStrcpy(format, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x");
  while (length > 0)
  {
    int n = (length > 16) ? 16 : length;
    memset(p, 0, sizeof(p));
    memcpy(p, pMem, n);
    format[n * 5] = 0; // nul terminate in the correct spot
    udTrace::Message(format, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    pMem = ((const char *)pMem) + n;
    length -= n;
  }
}

#if UDPLATFORM_WINDOWS && defined(__MEMORY_DEBUG__)

struct Entry
{
  char *pLineInfo;
  uint64_t total;
  uint32_t crc;
  uint32_t count;
};
static Entry *pEntries = nullptr;
static size_t entryCount;
static size_t entriesAllocated;
static size_t entryTotal;
static int64_t activeEntry;

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2022
static int __CRTDECL udDebugReportHook(int /*reportType*/, char *pLineStr, int *pReturnValue)
{
  *pReturnValue = 0;
  if (udStrEndsWith(pLineStr, ") : "))
  {
    activeEntry = -1;
    uint32_t crc = udCrc32c(pLineStr, udStrlen(pLineStr));
    size_t j;
    for (j = 0; j < entryCount; ++j)
    {
      if (crc == pEntries[j].crc && udStrcmp(pLineStr, pEntries[j].pLineInfo) == 0)
      {
        activeEntry = (int64_t)j;
        break;
      }
    }
    if (j == entryCount)
    {
      if (entryCount == entriesAllocated)
      {
        entriesAllocated += 4096;
        Entry *pNewEntries = (Entry *)udRealloc(pEntries, entriesAllocated * sizeof(Entry));
        if (pNewEntries)
          pEntries = pNewEntries;
        else
          entriesAllocated = entryCount; // Allocation failed, kinda to be expected since we're trying to figure out where the memory went
      }
      if (entryCount < entriesAllocated)
      {
        pEntries[entryCount].pLineInfo = udStrdup(pLineStr);
        pEntries[entryCount].crc = crc;
        pEntries[entryCount].count = 0;
        pEntries[entryCount].total = 0;
        ++entryCount;
        activeEntry = (int64_t)j;
      }
    }
  }

  if (activeEntry >= 0 && udStrEndsWith(pLineStr, "bytes long.\n"))
  {
    uint64_t allocAmount = udStrAtou64(udStrrchr(pLineStr, ",") + 1);
    pEntries[activeEntry].count++;
    pEntries[activeEntry].total += allocAmount;
    entryTotal += allocAmount;
  }
  return 1; // Return non-zero to prevent it from displaying the report
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2022
int EntrySortByTotal(const void *pA, const void *pB)
{
  int64_t r = ((const Entry *)pB)->total - ((const Entry *)pA)->total;
  // Casting to an int can be a problem with large numbers.
  if (r < 0)
    return -1;
  else if (r > 0)
    return 1;
  else
    return 0;
}

#endif // UDPLATFORM_WINDOWS && defined(__MEMORY_DEBUG__)

// ***************************************************************************************
// Author: Dave Pevreal, August 2022
bool udDebugMemoryReport(const char *pHeader, size_t minimumUsageForReport, size_t *pActualUsage)
{
#if UDPLATFORM_WINDOWS && defined(__MEMORY_DEBUG__)
  entryCount = entriesAllocated = entryTotal = 0;
  activeEntry = -1;

  _CRT_REPORT_HOOK prevHook = _CrtSetReportHook(udDebugReportHook);
  _CrtMemDumpAllObjectsSince(nullptr);
  _CrtSetReportHook(prevHook);

  if (pActualUsage)
    *pActualUsage = entryTotal;
  if (entryTotal >= minimumUsageForReport)
  {
    qsort(pEntries, entryCount, sizeof(Entry), EntrySortByTotal);
    if (pHeader)
      udDebugPrintf("%s\n", pHeader);
    for (int i = 0; i < entryCount; ++i)
      udDebugPrintf("%s %sKB total in %s allocations\n", pEntries[i].pLineInfo, udCommaInt(pEntries[i].total / 1024), udCommaInt(pEntries[i].count));
    udDebugPrintf("%sKB total allocated\n", udCommaInt(entryTotal / 1024));
  }

  for (int i = 0; i < entryCount; ++i)
    udFree(pEntries[i].pLineInfo);
  udFree(pEntries);
  entryCount = entriesAllocated = entryTotal = 0;
  return (entryTotal >= minimumUsageForReport);
#else
  udUnused(pHeader);
  udUnused(minimumUsageForReport);
  udUnused(pActualUsage);
  udDebugPrintf("udDebugMemoryReport requires UDPLATFORM_WINDOWS __MEMORY_DEBUG__ defined\n");
  return false;
#endif // UDPLATFORM_WINDOWS && defined(__MEMORY_DEBUG__)
}
