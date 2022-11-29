#include "gtest/gtest.h"
#include "udChunkedArray.h"
#include "udVirtualChunkedArray.h"
#include <iterator>
#include <algorithm>

TEST(udVirtualChunkedArray, StandardRun)
{
  udVirtualChunkedArray<int> array = {};

  ASSERT_EQ(udR_Success, array.Init(8, "TestExport.dmp", 1));

  // Push first page worth and ensure nothing spilled to temp
  for (int i = 0; i < 8; ++i)
    array.PushBack(i);

  EXPECT_TRUE(array.IsElementInMemory(0));

  // Next page should spill first page
  for (int i = 8; i < 16; ++i)
    array.PushBack(i);

  EXPECT_FALSE(array.IsElementInMemory(0));
  EXPECT_TRUE(array.IsElementInMemory(10));

  // And a third page should spill the second page
  for (int i = 16; i < 24; ++i)
    array.PushBack(i);

  EXPECT_FALSE(array.IsElementInMemory(0));
  EXPECT_FALSE(array.IsElementInMemory(10));
  EXPECT_TRUE(array.IsElementInMemory(20));

  // All should be retrievable, but reading first page again should spill the third page
  for (int i = 0; i < 24; ++i)
    EXPECT_EQ(array[i], i);

  // By the end only the 3rd page should be in memory
  EXPECT_EQ(false, array.IsElementInMemory(0));
  EXPECT_EQ(false, array.IsElementInMemory(8));
  EXPECT_EQ(true, array.IsElementInMemory(16));

  array.Deinit();
}
