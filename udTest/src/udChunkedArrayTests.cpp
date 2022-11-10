#include "gtest/gtest.h"
#include "udChunkedArray.h"
#include "udVirtualChunkedArray.h"
#include <iterator>
#include <algorithm>

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
  array.ToArray(buffer, UDARRAYSIZE(buffer), 1, len - 1);
  EXPECT_EQ(0, memcmp(buffer, pTestString + 1, len - 1));
  EXPECT_EQ(0, buffer[len - 1]);

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

TEST(udChunkedArrayTests, ZeroMemory)
{
  udChunkedArray<int> array;
  int *pTemp;

  array.Init(8);

  // First push unique non-zero numbers and then pop them to make sure the chunk is non-zero
  for (int i = 1; i <= 8; ++i)
    array.PushBack(i);
  for (int i = 1; i <= 8; ++i)
    array.PopBack();

  // Test pushfronts without zeroing work
  array.PushFront(&pTemp, false);
  EXPECT_NE(*pTemp, 0);
  array.PopFront();

  // Test pushbacks without zeroing work
  array.PushBack(&pTemp, false);
  EXPECT_NE(*pTemp, 0);
  array.PopBack();

  // Test pushfronts with zeroing work
  array.PushFront(&pTemp, true);
  EXPECT_EQ(*pTemp, 0);

  // Test pushbacks with zeroing work
  array.PushBack(&pTemp, true);
  EXPECT_EQ(*pTemp, 0);

  array.PopFront();
  array.PopBack();

  array.Deinit();
}


TEST(udChunkedArrayTests, FullChunkInsert)
{
  // Test to insert at all positions with the chunk being completely full
  udChunkedArray<int> array;

  for (size_t insertPos = 0; insertPos <= 8; ++insertPos)
  {
    array.Init(8);
    // Fill the first chunk with values 1..8
    for (int i = 0; i < 8; ++i)
      array.PushBack(i);
    for (int i = 0; i < 8; ++i)
      EXPECT_EQ(i, array[i]);
    // Now insert
    int temp = -1;
    array.Insert(insertPos, &temp);
    for (int i = 0; i < 9; ++i)
    {
      if (i < insertPos)
        EXPECT_EQ(i, array[i]);
      else if (i == insertPos)
        EXPECT_EQ(-1, array[i]);
      else
        EXPECT_EQ(i - 1, array[i]);
    }
    array.Deinit();
  }
}

template<typename T>
bool iteratorIsValid(const udChunkedArrayIterator<T> &it, const udChunkedArray<T> array)
{
  udResult result = udR_Success;
  UD_ERROR_IF(it.ppCurrChunk < array.ppChunks, udR_Failure);
  UD_ERROR_IF(it.ppCurrChunk > array.ppChunks + array.chunkCount, udR_Failure);
  if (it.ppCurrChunk == array.ppChunks)
  {
    UD_ERROR_IF(it.currChunkElementIndex < array.inset, udR_Failure);
  }
  else if (it.ppCurrChunk == array.ppChunks + array.chunkCount - 1)
  {
    UD_ERROR_IF(it.currChunkElementIndex > array.length % array.chunkElementCount, udR_Failure);
  }
  else
  {
    UD_ERROR_IF(it.currChunkElementIndex < 0, udR_Failure);
    UD_ERROR_IF(it.currChunkElementIndex >= array.chunkElementCount, udR_Failure);
  }
epilogue:
  return result == udR_Success;
}

TEST(udChunkedArrayTests, Iterator)
{
  udChunkedArray<int> array;
  array.Init(8);

  // Iterates across chunk boundaries correctly
  int i;
  for (i = 0; i < 32; ++i)
    array.PushBack(i);

  i = 0;
  for (auto x : array)
  {
    EXPECT_EQ(i, x);
    ++i;
  }
  EXPECT_EQ(array.length, i);


  // Iterates partially filled chunks correct
  array.PopBack();

  i = 0;
  for (auto x : array)
  {
    EXPECT_EQ(i, x);
    ++i;
  }
  EXPECT_EQ(array.length, i);

  // Iterates with inset chunk correctly
  array.PopFront();

  i = 1;
  for (auto x : array)
  {
    EXPECT_EQ(i, x);
    ++i;
  }
  EXPECT_EQ(array.length, i - 1);
  // array.end() is over the end of the array and not valid by this definition (dereferencable)
  EXPECT_TRUE(iteratorIsValid(array.end() - 1, array)); 
  udChunkedArrayIterator<int> it = array.begin();
  EXPECT_TRUE(iteratorIsValid(it, array));

  EXPECT_EQ(it.currChunkElementIndex, array.inset);
  EXPECT_EQ(it.ppCurrChunk, array.ppChunks);

  udChunkedArrayIterator<int> copy = ++it;
  EXPECT_EQ(copy, it);
  EXPECT_EQ(*it, 2);

  copy = --it;
  EXPECT_EQ(copy, it);
  EXPECT_EQ(*it, 1);

  it += 3;
  EXPECT_EQ(*it, 4);

  it -= 2;
  EXPECT_EQ(*it, 2);

  EXPECT_EQ(it[3], 5);
  EXPECT_EQ(*it, 2); //unchanged

  //defintion of comparisons
  copy = it;

  EXPECT_EQ(it < copy, false);
  EXPECT_EQ(it > copy, false);
  EXPECT_EQ(it <= copy, true);
  EXPECT_EQ(it >= copy, true);

  EXPECT_EQ(it != copy, false);
  EXPECT_EQ(it == copy, true);

  EXPECT_EQ(it < (it + 3), true);
  EXPECT_EQ(it < (it - 1), false);

  EXPECT_EQ(it > (it + 3), false);
  EXPECT_EQ(it > (it - 1), true);

  auto it2 = it - 2;
  --it;
  EXPECT_EQ(it2, --it);

  i = 1;
  for (auto iter = array.begin(); iter < array.end(); ++iter)
  {
    EXPECT_EQ(*iter, i);
    auto iter2 = array.begin() + i;
    //test that indexing into the array by adding to an interator will give the same result as dereferencing the iterator
    EXPECT_EQ(*(iter2 - 1), *iter); 
    ++i;
  }
  EXPECT_EQ(i, array.length + 1);

  for (auto iter = array.end() - 1; iter > array.begin(); --iter)
  {
    --i;
    EXPECT_EQ(*iter, i);
    //test that indexing into the array using an interator will give the same result as dereferencing the iterator
    EXPECT_EQ(array.begin()[i - 1], *iter); 
  }

  EXPECT_EQ(array.end() - array.begin(), (int)array.length);

  array.Deinit();
  array.Init(16);
  for (i = 0; i < 32; ++i) // at least 2 chunks
  {
    array.PushBack(i % 4);
  }

  // do the test on an array with an inset:
  array.PopFront();

  std::sort(array.begin(), array.end());
  int previous = array[0];
  for (int el : array)
  {
    EXPECT_TRUE(previous <= el);
    previous = el;
  }

  //testing std::reverse_iterator: should return reverse order
  auto reverseStart = std::reverse_iterator<udChunkedArrayIterator<int>>(array.end());
  auto reverseEnd = std::reverse_iterator<udChunkedArrayIterator<int>>(array.begin());

  EXPECT_EQ(reverseEnd - reverseStart, (int)array.length);
  previous = *reverseStart;
  for (auto iter = reverseStart; iter < reverseEnd; iter++)
  {
    EXPECT_TRUE(*iter <= previous);
    previous = *iter;
  }

  array.Deinit();
}

TEST(udChunkedArrayTests, udVirtualChunkedArray)
{
  // udVirtualChunkedArray is in udOBJ for the moment, and may move to udCore in the future
  udVirtualChunkedArray<int> array;
  const char *pTempFilename = nullptr;
  udFile *pTempFile = nullptr;
  int64_t tempFileLen = -1;

  ASSERT_EQ(udR_Success, udFile_GenerateRawFilename(&pTempFilename, nullptr, 0, udCT_None, "TestConvertOutput.uds", 256 * 1024)); // 256k
  ASSERT_EQ(udR_Success, udFile_Open(&pTempFile, pTempFilename, udFOF_Create | udFOF_Read | udFOF_Write));
  ASSERT_EQ(udR_Success, array.Init(8, 1, pTempFile));

  // Push first page worth and ensure nothing spilled to temp
  for (int i = 0; i < 8; ++i)
    array.PushBack(i);
  udFile_Read(pTempFile, &tempFileLen, 0, 0, udFSW_SeekEnd, nullptr, &tempFileLen);
  EXPECT_EQ(0, tempFileLen);

  // Next page should spill first page
  for (int i = 8; i < 16; ++i)
    array.PushBack(i);
  udFile_Read(pTempFile, &tempFileLen, 0, 0, udFSW_SeekEnd, nullptr, &tempFileLen);
  EXPECT_EQ(32, tempFileLen);
  EXPECT_EQ(false, array.IsChunkLoaded(0));

  // And a third page should spill the second page
  for (int i = 16; i < 24; ++i)
    array.PushBack(i);
  udFile_Read(pTempFile, &tempFileLen, 0, 0, udFSW_SeekEnd, nullptr, &tempFileLen);
  EXPECT_EQ(64, tempFileLen);
  EXPECT_EQ(false, array.IsChunkLoaded(0));
  EXPECT_EQ(false, array.IsChunkLoaded(8));

  // All should be retrievable, but reading first page again should spill the third page
  for (int i = 0; i < 24; ++i)
    EXPECT_EQ(array[i], i);
  udFile_Read(pTempFile, &tempFileLen, 0, 0, udFSW_SeekEnd, nullptr, &tempFileLen);
  EXPECT_EQ(96, tempFileLen);

  // By the end only the 3rd page should be in memory
  EXPECT_EQ(false, array.IsChunkLoaded(0));
  EXPECT_EQ(false, array.IsChunkLoaded(8));
  EXPECT_EQ(true, array.IsChunkLoaded(16));

  array.Deinit();
  udFile_Close(&pTempFile);
  udFree(pTempFilename);
}
