#include "gtest/gtest.h"
#include "udChunkedArray.h"

TEST(udChunkedArrayTests, ToArray)
{
  const char *pTestString = "Hello World!";
  char buffer[32];
  udChunkedArray<char> array;
  size_t len = strlen(pTestString);

  array.Init(8);
  for (size_t i = 0; i < len; ++i)
    array.PushBack(pTestString[i]);
  EXPECT_EQ(len, array.length);

  // Basic extraction of entire contents
  memset(buffer, 0, sizeof(buffer));
  array.ToArray(buffer, UDARRAYSIZE(buffer), 0, len);
  EXPECT_EQ(0, memcmp(buffer, pTestString, len));

  // Extraction of a subset
  memset(buffer, 0, sizeof(buffer));
  array.ToArray(buffer, UDARRAYSIZE(buffer), 1, len-1);
  EXPECT_EQ(0, memcmp(buffer, pTestString + 1, len - 1));
  EXPECT_EQ(0, buffer[len-1]);

  // Extraction with an inset
  array.PopFront(buffer);
  EXPECT_EQ(1, array.inset);
  memset(buffer, 0, sizeof(buffer));
  array.ToArray(buffer, UDARRAYSIZE(buffer), 0, len - 1);
  EXPECT_EQ(0, memcmp(buffer, pTestString + 1, len - 1));
  EXPECT_EQ(0, buffer[len - 1]);

  EXPECT_EQ(udR_OutOfRange, array.ToArray(buffer, UDARRAYSIZE(buffer), len, 1));
  EXPECT_EQ(udR_OutOfRange, array.ToArray(buffer, UDARRAYSIZE(buffer), 0, len));
  EXPECT_EQ(udR_BufferTooSmall, array.ToArray(buffer, 4, 0, 5));

  array.Deinit();
}

TEST(udChunkedArrayTests, Basic)
{
  udChunkedArray<int> array;
  int temp;

  array.Init(8);

  array.PushBack(10);         // 10
  EXPECT_EQ(1, array.length);
  array.PushBack(20);         // 10,20
  EXPECT_EQ(2, array.length);
  array.PushBack(40);         // 10,20,40
  EXPECT_EQ(3, array.length);
  array.PushBack(50);         // 10,20,40,50
  EXPECT_EQ(4, array.length);
  temp = 30;
  array.Insert(2, &temp);     // 10,20,30,40,50
  EXPECT_EQ(30, array[2]);
  EXPECT_EQ(5, array.length);

  array.PopFront(&temp);      // 20,30,40,50
  EXPECT_EQ(10, temp);
  EXPECT_EQ(4, array.length);
  EXPECT_EQ(1, array.inset);

  array.PopBack(&temp);       // 20,30,40
  EXPECT_EQ(50, temp);
  EXPECT_EQ(3, array.length);
  EXPECT_EQ(1, array.inset);

  // Add 8 more so there's 2 chunks
  for (temp = 0; temp < 8; ++temp)
    array.PushBack(50 + temp * 10); // 20,30,40,50,60,70,80,90,100,110,120
  EXPECT_EQ(11, array.length);
  EXPECT_EQ(2, array.chunkCount);

  for (size_t index = 0; index < array.length; ++index)
    EXPECT_EQ(20 + index * 10, array[index]);

  // Expecting two chunks, the first one with an inset of 1
  EXPECT_EQ(7, array.GetElementRunLength(0));
  EXPECT_EQ(4, array.GetElementRunLength(7));

  // Check GetElementRunLength with elementsBehind true
  EXPECT_EQ(0, array.GetElementRunLength(0, true));  // first element first chunk
  EXPECT_EQ(3, array.GetElementRunLength(3, true));  // middle of first chunk
  EXPECT_EQ(2, array.GetElementRunLength(9, true));  // middle of second chunk
  EXPECT_EQ(3, array.GetElementRunLength(10, true)); // last element second chunk

  // Now insert again, but this time with an inset, in second chunk
  temp = 105;
  array.Insert(9, &temp);     // 20,30,40,50,60,70,80,90,100,105,110,120
  EXPECT_EQ(105, array[9]);
  EXPECT_EQ(1, array.inset);

  // Now insert again, but this time with an inset, in first chunk (special case to eliminate inset)
  temp = 65;
  array.Insert(5, &temp);     // 20,30,40,50,60,65,70,80,90,100,105,110,120
  EXPECT_EQ(65, array[5]);
  EXPECT_EQ(0, array.inset);

  // Insert at start
  temp = 15;
  array.Insert(0, &temp);     // 15,20,30,40,50,60,65,70,80,90,100,105,110,120
  EXPECT_EQ(15, array[0]);
  EXPECT_EQ(0, array.inset);

  // Insert at end
  temp = 125;
  array.Insert(14, &temp);     // 15,20,30,40,50,60,65,70,80,90,100,105,110,120,125
  EXPECT_EQ(125, array[14]);
  EXPECT_EQ(0, array.inset);

  array.Deinit();
}
