#include "gtest/gtest.h"

#include "udUUID.h"

TEST(udUUID, ValidationTests)
{
  //Some Valid UUIDs
  EXPECT_TRUE(udUUID_IsValid("00000000-0000-0000-0000-000000000000"));
  EXPECT_TRUE(udUUID_IsValid("11111111-1111-1111-1111-111111111111"));
  EXPECT_TRUE(udUUID_IsValid("22222222-2222-2222-2222-222222222222"));
  EXPECT_TRUE(udUUID_IsValid("33333333-3333-3333-3333-333333333333"));
  EXPECT_TRUE(udUUID_IsValid("44444444-4444-4444-4444-444444444444"));
  EXPECT_TRUE(udUUID_IsValid("55555555-5555-5555-5555-555555555555"));
  EXPECT_TRUE(udUUID_IsValid("66666666-6666-6666-6666-666666666666"));
  EXPECT_TRUE(udUUID_IsValid("77777777-7777-7777-7777-777777777777"));
  EXPECT_TRUE(udUUID_IsValid("88888888-8888-8888-8888-888888888888"));
  EXPECT_TRUE(udUUID_IsValid("99999999-9999-9999-9999-999999999999"));

  EXPECT_TRUE(udUUID_IsValid("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"));
  EXPECT_TRUE(udUUID_IsValid("bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"));
  EXPECT_TRUE(udUUID_IsValid("cccccccc-cccc-cccc-cccc-cccccccccccc"));
  EXPECT_TRUE(udUUID_IsValid("dddddddd-dddd-dddd-dddd-dddddddddddd"));
  EXPECT_TRUE(udUUID_IsValid("eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee"));
  EXPECT_TRUE(udUUID_IsValid("ffffffff-ffff-ffff-ffff-ffffffffffff"));
  EXPECT_TRUE(udUUID_IsValid("01234567-89ab-cdef-0123-456789abcdef"));

  EXPECT_TRUE(udUUID_IsValid("AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA"));
  EXPECT_TRUE(udUUID_IsValid("BBBBBBBB-BBBB-BBBB-BBBB-BBBBBBBBBBBB"));
  EXPECT_TRUE(udUUID_IsValid("CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC"));
  EXPECT_TRUE(udUUID_IsValid("DDDDDDDD-DDDD-DDDD-DDDD-DDDDDDDDDDDD"));
  EXPECT_TRUE(udUUID_IsValid("EEEEEEEE-EEEE-EEEE-EEEE-EEEEEEEEEEEE"));
  EXPECT_TRUE(udUUID_IsValid("FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"));
  EXPECT_TRUE(udUUID_IsValid("01234567-89AB-CDEF-0123-456789ABCDEF"));

  EXPECT_TRUE(udUUID_IsValid("dc8ea1a9-becd-4534-adad-900b3972ee8c"));
  EXPECT_TRUE(udUUID_IsValid("DC8EA1A9-BECD-4534-ADAD-900B3972EE8C"));

  //Some only just invalid UUIDs
  EXPECT_FALSE(udUUID_IsValid("gggggggg-gggg-gggg-gggg-gggggggggggg")); //outside range
  EXPECT_FALSE(udUUID_IsValid("dc8ea1a9_becd_4534_adad_900b3972ee8c")); //underscores
  EXPECT_FALSE(udUUID_IsValid("dc8ea1a9 becd 4534 adad 900b3972ee8c")); //spaces
  EXPECT_FALSE(udUUID_IsValid("dc8ea1a90becd045340adad0900b3972ee8c"));
  EXPECT_FALSE(udUUID_IsValid("dc8ea1a9becd4534adad900b3972ee8c"));
  EXPECT_FALSE(udUUID_IsValid("DC8EA1A9-BECD-4534-ADAD-900B3972EE8")); // Missing the last character

  //Some very invalid UUIDs
  EXPECT_FALSE(udUUID_IsValid((const char *)nullptr));
  EXPECT_FALSE(udUUID_IsValid(""));
  EXPECT_FALSE(udUUID_IsValid("1234-"));
}

TEST(udUUID, ComparisonTests)
{
  udUUID a, b, c;

  EXPECT_EQ(udR_Success, udUUID_SetFromString(&a, "DC8EA1A9-BECD-4534-ADAD-900B3972EE8C"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&b, "dc8ea1a9-becd-4534-adad-900b3972ee8c"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&c, "01234567-89AB-CDEF-0123-456789ABCDEF"));

  EXPECT_TRUE(a == b);
  EXPECT_TRUE(b == a);
  EXPECT_TRUE(a != c);
  EXPECT_TRUE(c != a);
}

TEST(udUUID, NonceTests)
{
  udUUID a, b, c, d, e;

  EXPECT_EQ(udR_Success, udUUID_SetFromString(&a, "DC8EA1A9-BECD-4534-ADAD-900B3972EE8C"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&b, "dc8ea1a9-becd-4534-adad-900b3972ee8c"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&c, "01234567-89AB-CDEF-0123-456789ABCDEF"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&d, "76543210-BA98-CDEF-7654-3210BA980000"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&e, "00000000-0000-CDEF-0000-000000000000"));

  EXPECT_NE(0, udUUID_ToNonce(&a));
  EXPECT_NE(0, udUUID_ToNonce(&c));

  EXPECT_EQ(udUUID_ToNonce(&a), udUUID_ToNonce(&b));
  EXPECT_NE(udUUID_ToNonce(&a), udUUID_ToNonce(&c));

  EXPECT_EQ(8152414324602695308U, udUUID_ToNonce(&a));
  EXPECT_EQ(8152414324602695308U, udUUID_ToNonce(&b));
  EXPECT_EQ(52719U, udUUID_ToNonce(&c));
  EXPECT_EQ(0U, udUUID_ToNonce(&d));
  EXPECT_EQ(0U, udUUID_ToNonce(&e));
}

TEST(udUUID, GenerationTests)
{
  udUUID generated, expected;

  // Version 5 from string (https://de.wikipedia.org/wiki/Universally_Unique_Identifier#Namensbasierte_UUIDs_.28Version_3_und_5.29)
  EXPECT_EQ(udR_Success, udUUID_GenerateFromString(&generated, "www.example.org"));
  EXPECT_EQ(udR_Success, udUUID_SetFromString(&expected, "74738ff5-5367-5958-9aee-98fffdcd1876"));
  EXPECT_EQ(expected, generated);

  // Version 4 (random)
  EXPECT_EQ(udR_Success, udUUID_GenerateFromRandom(&generated));
  EXPECT_EQ(udR_Success, udUUID_GenerateFromRandom(&expected));
  EXPECT_NE(expected, generated);
}
