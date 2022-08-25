#include "udScopedKeepAwake.h"

thread_local int udScopedKeepAwake::refCount = 0;
