#include "gtest/gtest.h"
#include "udCompression.h"
#include "udMath.h"

TEST(udCompressionTests, Basic)
{
  for (int i = 1; i < udCT_Count; ++i) //Skip udCT_None
  {
    udCompressionType compressionType = (udCompressionType)i;

    const char input[] = "This is the best string I could think of.";
    void *pDeflated = nullptr;
    size_t deflatedSize = 0;
    char inflated[UDARRAYSIZE(input)] = {};
    size_t inflatedSize = 0;

    // Compress
    EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input), compressionType));
    EXPECT_NE(input, pDeflated);
    EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
    EXPECT_NE(0, memcmp(input, pDeflated, udMin(UDARRAYSIZE(input), deflatedSize)));

    // Decompress
    EXPECT_EQ(udR_Success, udCompression_Inflate(inflated, UDARRAYSIZE(input), pDeflated, deflatedSize, &inflatedSize, compressionType));
    EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
    EXPECT_EQ(0, memcmp(input, inflated, udMin(UDARRAYSIZE(input), inflatedSize)));

    udFree(pDeflated);
  }
}

TEST(udCompressionTests, EnsureBufferTooSmallResult)
{
  for (int i = 1; i < udCT_Count; ++i) //Skip udCT_None
  {
    udCompressionType compressionType = (udCompressionType)i;

    const char input[] = "This string is a string specifically for the EnsureBufferTooSmallResult test. And some UTF8- 你好！ Everything is good!";
    uint8_t inflated[64];
    size_t inflatedSize = 0;
    void *pDeflated = nullptr;
    size_t deflatedSize = 0;

    // Compress
    EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input), compressionType));
    EXPECT_NE(input, pDeflated);
    EXPECT_NE(UDARRAYSIZE(input), deflatedSize);
    EXPECT_NE(0, memcmp(input, pDeflated, udMin(UDARRAYSIZE(input), deflatedSize)));

    // Decompress
    EXPECT_EQ(udR_BufferTooSmall, udCompression_Inflate(inflated, UDARRAYSIZE(inflated), pDeflated, deflatedSize, &inflatedSize, compressionType));

    udFree(pDeflated);
  }
}

TEST(udCompressionTests, Complex)
{
  for (int i = 1; i < udCT_Count; ++i) //Skip udCT_None
  {
    udCompressionType compressionType = (udCompressionType)i;

    const char input[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. In elementum, arcu a elementum volutpat, velit orci ultrices nisi, id iaculis purus ligula et ex. Donec pulvinar maximus ultrices. Sed commodo condimentum consequat. Etiam vitae sagittis purus. Cras sit amet iaculis magna, in tincidunt leo. Cras ac nisi vitae nunc consequat dictum non nec ex. Nullam vel accumsan sem. Pellentesque bibendum nibh est, a fringilla arcu aliquet a. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. In augue risus, ultricies non posuere quis, eleifend non tellus. Praesent molestie ornare neque ut euismod. Sed ac orci in arcu congue tincidunt in nec ex. Phasellus ut nisi ultricies quam congue commodo et vel velit. Pellentesque vel dictum purus. Mauris et felis ut ex cursus euismod in ac ex.\n\nUt est erat, convallis non ultricies eu, interdum nec felis. Quisque fringilla massa nisl, ac tincidunt sem aliquet non. Quisque eleifend interdum sollicitudin. Nullam mattis lorem nec ultrices suscipit. Aliquam auctor augue sed nisl interdum, vitae interdum elit imperdiet. Sed vel consequat dui. Etiam dapibus metus non purus sagittis, at malesuada turpis gravida. Suspendisse feugiat venenatis dolor, sed tristique sem varius elementum.\n\nDonec posuere eget tortor a suscipit. Sed tincidunt orci non diam egestas tristique. Fusce rutrum tincidunt scelerisque. Etiam vitae ipsum sit amet leo semper sodales. Curabitur facilisis, erat aliquam dapibus tristique, mauris odio vulputate diam, nec aliquam enim sapien sed sapien. Aliquam vitae erat in eros elementum consectetur vel lacinia velit. Nullam non nulla consectetur, ultricies est mollis, ullamcorper erat. Fusce eu sapien vitae turpis tristique tempor. Nam laoreet gravida consequat. Morbi in massa auctor, vulputate erat ut, vulputate est.\n\nPellentesque sit amet suscipit justo. Nulla accumsan volutpat dolor a pharetra. Curabitur dapibus mauris consequat pharetra gravida. Nunc molestie suscipit iaculis. Vestibulum nibh enim, tristique lobortis efficitur scelerisque, commodo eleifend lectus. Mauris posuere condimentum tellus vitae pharetra. Aenean malesuada porttitor leo, a aliquet lacus sodales ut. Sed ut malesuada magna. Vivamus nec nulla felis. Sed non mi sed leo malesuada gravida sit amet ut dolor.\n\nSed pretium velit sem, a suscipit velit ornare vitae. Donec sed neque ullamcorper, posuere sem sed, dignissim ligula. Curabitur mauris felis, facilisis non accumsan eget, dictum eget ex. Nulla facilisi. Etiam rhoncus, risus sit amet porttitor sagittis, massa velit volutpat diam, ornare suscipit tellus tortor et metus. Nunc a sapien vel dolor fermentum auctor. Sed et orci dapibus, fermentum diam id, mattis nunc. Duis et mauris et lacus placerat volutpat id eget arcu. Maecenas fermentum odio at arcu efficitur pharetra.\n\nProin vel sapien sed ipsum venenatis elementum quis cursus ex. Duis suscipit, mi vitae pretium sollicitudin, nunc elit euismod lectus, vitae fermentum lorem sapien vitae mauris. Sed tempor id ex sed consectetur. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus in nunc ac felis elementum luctus et vel sapien. Vivamus condimentum fringilla rutrum. Morbi hendrerit eros ipsum, eget eleifend elit viverra sit amet. Nam ut nulla id lectus rutrum ullamcorper. Fusce dapibus augue massa, blandit egestas sapien finibus vitae.\n\nNunc feugiat neque vitae quam venenatis aliquam. Curabitur finibus, quam sed ultricies rutrum, arcu libero sollicitudin neque, ac dapibus urna quam nec diam. In rutrum ex sed nisl efficitur, eu accumsan mauris euismod. Maecenas quis ligula volutpat, rhoncus ligula at, volutpat volutpat.";

    uint8_t inflated[4096];
    size_t inflatedSize = 0;
    uint8_t deflated[4096];
    void *pDeflated = nullptr;
    size_t deflatedSize = 0;

    // Compress
    EXPECT_EQ(udR_Success, udCompression_Deflate(&pDeflated, &deflatedSize, input, UDARRAYSIZE(input), compressionType));
    // To save issues of leaking, copy to local array and free allocated memory
    if (deflatedSize < UDARRAYSIZE(deflated))
    {
      memcpy(deflated, pDeflated, deflatedSize);
      udFree(pDeflated);
    }

    EXPECT_LT(deflatedSize, UDARRAYSIZE(input)); // Expect some compression to have occured
    EXPECT_NE(0, memcmp(input, deflated, udMin(UDARRAYSIZE(input), deflatedSize)));

    // Decompress
    EXPECT_EQ(udR_Success, udCompression_Inflate(inflated, UDARRAYSIZE(inflated), deflated, deflatedSize, &inflatedSize, compressionType));
    EXPECT_EQ(UDARRAYSIZE(input), inflatedSize);
    EXPECT_EQ(0, memcmp(input, inflated, inflatedSize));
  }
}

