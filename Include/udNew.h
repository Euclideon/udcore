#ifndef UDNEW_H
#define UDNEW_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, November 2015
//
// Wrap up C++ new operator to us udAlloc internally
//

#include <new>
#include "udPlatform.h"


#define udNew(type, ...) new (udAlloc(sizeof(type), udAF_None)) type(__VA_ARGS__)
#define udNewNoParams(type) new (udAlloc(sizeof(type), udAF_None)) type()
#define udNewFlags(type, extra, flags, ...) new (udAlloc(sizeof(type) + extra, flags)) type(__VA_ARGS__)
#define udNewFlagsNoParams(type, extra, flags) new (udAlloc(sizeof(type) + extra, flags)) type()

template <typename T>
void _udDelete(T *&pMemory, const char *pFile, int line)
{
  if (pMemory)
  {
    pMemory->~T();
    _udFree(pMemory, pFile, line);
  }
}
#define udDelete(pMemory) _udDelete(pMemory, IF_MEMORY_DEBUG(__FILE__, __LINE__))


#endif // UDNEW_H
