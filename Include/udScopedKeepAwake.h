#ifndef UDSCOPEDEKEEPAWAKE_H
#define UDSCOPEDEKEEPAWAKE_H

#include "udPlatform.h"
#include <stdio.h>

// ----------------------------------------------------------------------------
// A class to handle setting and restoring o/s state to prevent the computer sleeping
// Currently windows only, and local to udConvert_Internal pending further testing
class udScopedKeepAwake
{
public:
  // ----------------------------------------------------------------------------
  // Author: Dave Pevreal, July 2022
  udScopedKeepAwake()
  {
    refCount++;
#if UDPLATFORM_WINDOWS && !UDPLATFORM_UWP
    // Request the computer not sleep/hibernate in the absence of user input
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
#endif
  }

  // ----------------------------------------------------------------------------
  // Author: Dave Pevreal, July 2022
  ~udScopedKeepAwake()
  {
    refCount--;
    if (refCount <= 0)
    {
#if UDPLATFORM_WINDOWS && !UDPLATFORM_UWP
      // Restore correct execution state
      SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }
  }

private:
  static thread_local int refCount;
};

#endif
