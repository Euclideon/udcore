#include "gtest/gtest.h"
#include "udGeoZone.h"

TEST(udGeoZoneTests, Init)
{
  // Ensure all fields are initialised
  udResult result;
  udGeoZone zone1, zone2;

  memset(&zone1, 0x55, sizeof(zone1));
  memset(&zone2, 0xaa, sizeof(zone1));

  udGeoZone_SetFromSRID(&zone1, 32756);
  udGeoZone_SetFromSRID(&zone2, 32756);
  EXPECT_EQ(0, memcmp(&zone1, &zone2, sizeof(zone1)));
}

TEST(udGeoZoneTests, Basic)
{
  udResult result;
  udGeoZone zone;
  udDouble3 latLong = udDouble3::create(-27.460375, 153.099019, 0);
  int testPrecision = 1000000;

  result = udGeoZone_SetFromSRID(&zone, 32756);
  EXPECT_EQ(udR_Success, result);
  udDouble3 pos = udGeoZone_ToCartesian(zone, latLong);
  udDouble3 latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int(latLong.x * testPrecision), int(latLong2.x * testPrecision));
  EXPECT_EQ(int(latLong.y * testPrecision), int(latLong2.y * testPrecision));

  latLong = udDouble3::create(-8.8098979, 115.1671961, 0);
  testPrecision = 100000;

  result = udGeoZone_SetFromSRID(&zone, 28350);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int(latLong.x * testPrecision), int(latLong2.x * testPrecision));
  EXPECT_EQ(int(latLong.y * testPrecision), int(latLong2.y * testPrecision));

  latLong = udDouble3::create(18.4638338, -66.1051628, 0);
  testPrecision = 1000;

  result = udGeoZone_SetFromSRID(&zone, 32620);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int(latLong.x * testPrecision), int(latLong2.x * testPrecision));
  EXPECT_EQ(int(latLong.y * testPrecision), int(latLong2.y * testPrecision));
}
