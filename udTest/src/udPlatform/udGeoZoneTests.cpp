#include "gtest/gtest.h"
#include "udGeoZone.h"

TEST(udGeoZoneTests, Init)
{
  // Ensure all fields are initialised
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
  udDouble3 latLong = udDouble3::create(-29.013435591336, 152.46461046582, 0);
  int64_t testPrecision = 100000000;

  result = udGeoZone_SetFromSRID(&zone, 32756);
  EXPECT_EQ(udR_Success, result);
  udDouble3 pos = udGeoZone_ToCartesian(zone, latLong);
  udDouble3 latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(47.869421767328, -121.02374445344, 0);

  result = udGeoZone_SetFromSRID(&zone, 2285);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(47.86900408566, -124.56927806139, 0);

  result = udGeoZone_SetFromSRID(&zone, 2285);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(48.93793227151, 1.0107421875, 0);

  result = udGeoZone_SetFromSRID(&zone, 3949);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(48.261566162109, -118.88442993164, 0);

  result = udGeoZone_SetFromSRID(&zone, 2285);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(48.606262207031, -117.52075195313, 0);

  result = udGeoZone_SetFromSRID(&zone, 2285);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-66.280516237021, 153.00181113163, 0);

  result = udGeoZone_SetFromSRID(&zone, 32756);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-21.919611308953, 142.35533595086, 0);

  result = udGeoZone_SetFromSRID(&zone, 32756);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-42.284585972703, 173.02903848714, 0);

  result = udGeoZone_SetFromSRID(&zone, 32756);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(52.811011948165, -123.11526606218, 0);

  result = udGeoZone_SetFromSRID(&zone, 26910);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(35.537093649874, -122.04241991043, 0);

  result = udGeoZone_SetFromSRID(&zone, 26910);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(69.02553236112, 147.18853443861, 0);

  result = udGeoZone_SetFromSRID(&zone, 32655);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(81.790122721387, 146.94860787551, 0);

  result = udGeoZone_SetFromSRID(&zone, 32655);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-33.5338767941, 115.00626123502, 0);

  result = udGeoZone_SetFromSRID(&zone, 28350);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-41.953243413307, 146.54738222308, 0);

  result = udGeoZone_SetFromSRID(&zone, 28350);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

  latLong = udDouble3::create(-70.463470357348, 119.69509496061, 0);

  result = udGeoZone_SetFromSRID(&zone, 28350);
  EXPECT_EQ(udR_Success, result);
  pos = udGeoZone_ToCartesian(zone, latLong);
  latLong2 = udGeoZone_ToLatLong(zone, pos);
  EXPECT_EQ(int64_t(latLong.x * testPrecision), int64_t(latLong2.x * testPrecision));
  EXPECT_EQ(int64_t(latLong.y * testPrecision), int64_t(latLong2.y * testPrecision));

}
