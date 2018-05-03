#ifndef UDNEW_H
#define UDNEW_H

#include <new>
#include "udPlatform.h"


#define udNew(type, ...) new (_udAlloc(sizeof(type), udAF_None IF_MEMORY_DEBUG(__FILE__, __LINE__))) type(__VA_ARGS__)
#define udNewNoParams(type) new (_udAlloc(sizeof(type), udAF_None IF_MEMORY_DEBUG(__FILE__, __LINE__))) type()
#define udNewFlags(type, extra, flags, ...) new (_udAlloc(sizeof(type) + extra, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))) type(__VA_ARGS__)
#define udNewFlagsNoParams(type, extra, flags) new (_udAlloc(sizeof(type) + extra, flags IF_MEMORY_DEBUG(__FILE__, __LINE__))) type()

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
