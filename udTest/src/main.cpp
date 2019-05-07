#include "gtest/gtest.h"

#include "udPlatform.h"
#include "udThread.h"
#include "udFile.h"

#if UDPLATFORM_WINDOWS && UD_DEBUG
#  define _CRT_SECURE_NO_WARNINGS
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#  include <stdio.h>
#endif

#if UDPLATFORM_EMSCRIPTEN
#include <emscripten.h>
int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);

  udFile_RegisterHTTP();
  udOctree_Init(nullptr); // Initialise with defaults
  int testResult = 0;
  emscripten_set_main_loop_arg([](void *pArg) { int *pTestResult = (int*)pArg; *pTestResult = RUN_ALL_TESTS(); emscripten_cancel_main_loop(); }, &testResult, 60, 1);
  udOctree_Deinit();
  udThread_DestroyCached(); // Destroy cached threads to prevent reporting of memory leak

  return testResult;
}
#else
int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);

#if UDPLATFORM_WINDOWS && UD_DEBUG
  _CrtMemState m1, m2, diff;
  // To see allocation details (file and line number) you must
  // define '__MEMORY_DEBUG__' for all projects you wish to track
  // and you must also set the CRT debug flag below
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
  _CrtMemCheckpoint(&m1);
#endif //UDPLATFORM_WINDOWS && UD_DEBUG

  int testResult = RUN_ALL_TESTS();
  udThread_DestroyCached(); // Destroy cached threads to prevent reporting of memory leak

#if UDPLATFORM_WINDOWS && UD_DEBUG
  udSleep(500); // A little extra time for threads to destroy
  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 1) // gtest leaks 1 allocation, let's ignore it!
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks in test found");
    testResult = -1;
  }
#endif //UDPLATFORM_WINDOWS && UD_DEBUG

  return testResult;
}
#endif
