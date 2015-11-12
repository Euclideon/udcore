#ifndef UDNEW_H
#define UDNEW_H

#if defined(UDPLATFORM_H)
// udPlatform includes one or more system header files which indirectly include vcruntime.h, which defines _HAS_EXCEPTIONS
#error "udNew.h must be included before udPlatform.h"
#endif

#if _MSC_VER && _MSC_VER > 1800
#define _HAS_EXCEPTIONS 0
#endif

#include <new>
#include "udPlatform.h"


#define udNew(type, ...) new (_udAlloc(sizeof(type), udAF_None IF_MEMORY_DEBUG(__FILE__, __LINE__))) type(__VA_ARGS__)
#define udNewFlags(type, extra, flags, ...) new (_udAlloc(sizeof(type) + extra, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))) type(__VA_ARGS__)

template <typename T>
void _udDelete(T *&pMemory IF_MEMORY_DEBUG(const char * pFile = __FILE__, int  line = __LINE__))
{
  if (pMemory)
  {
    pMemory->~T();
    _udFree(pMemory IF_MEMORY_DEBUG(pFile, line));
  }
}
#define udDelete(pMemory) _udDelete(pMemory IF_MEMORY_DEBUG(__FILE__, __LINE__))


#endif // UDNEW_H
