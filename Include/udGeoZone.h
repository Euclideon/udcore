#ifndef UDGEOZONE_H
#define UDGEOZONE_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, June 2018
//
// Module for dealing with geolocation and transformation between geodetic and cartesian coordinates
//

#include "udMath.h"

enum udGeoZoneEllipsoid
{
  udGZE_WGS84,          //EPSG:7030
  udGZE_Airy1830,       //EPSG:7001
  udGZE_AiryModified,   //EPSG:7002
  udGZE_Bessel1841,     //EPSG:7004
  udGZE_Clarke1866,     //EPSG:7008
  udGZE_Clarke1880IGN,  //EPSG:7011
  udGZE_GRS80,          //EPSG:7019
  udGZE_Intl1924,       //EPSG:7022 (Same numbers as Hayford)
  udGZE_WGS72,          //EPSG:7043
  udGZE_CGCS2000,       //EPSG:1024
  udGZE_Clarke1858,     //EPSG:7007
  udGZE_Clarke1880FOOT, //EPSG:7055
  udGZE_Krassowsky1940, //EPSG:7024
  udGZE_Everest1930M,   //EPSG:7018
  udGZE_Mars,
  udGZE_Moon,

  udGZE_Count
};

enum udGeoZoneGeodeticDatum
{
  udGZGD_WGS84,      //EPSG:4326
  udGZGD_ED50,       //EPSG:4230
  udGZGD_ETRS89,     //EPSG:4258
  udGZGD_TM75,       //EPSG:4300
  udGZGD_NAD27,      //EPSG:4267
  udGZGD_NAD83,      //EPSG:4269
  udGZGD_NAD83_2011, //EPSG:6318
  udGZGD_NTF,        //EPSG:4275
  udGZGD_OSGB36,     //EPSG:4277
  udGZGD_Potsdam,    //EPSG:4746
  udGZGD_Tokyo,      //EPSG:7414
  udGZGD_WGS72,      //EPSG:4322
  udGZGD_JGD2000,    //EPSG:4612
  udGZGD_JGD2011,    //EPSG:6668
  udGZGD_GDA94,      //EPSG:4283
  udGZGD_GDA2020,    //EPSG:7844
  udGZGD_RGF93,      //EPSG:4171
  udGZGD_NAD83_HARN, //EPSG:4152
  udGZGD_CGCS2000,   //EPSG:4490
  udGZGD_HK1980,     //EPSG:4611
  udGZGD_SVY21,      //EPSG:4757
  udGZGD_MGI,        //EPSG:6312
  udGZGD_NZGD2000,   //EPSG:2193
  udGZGD_AMERSFOORT, //EPSG:28992
  udGZGD_TRI1903,    //EPSG:30200
  udGZGD_VANUA1915,  //EPSG:3139
  udGZGD_DEALUL1970, //EPSG:31700
  udGZGD_SINGGRID,   //EPSG:19920
  udGZGD_MARS_MERC,  //EPSG:49974,49975
  udGZGD_MARS_PCPF,  //EPSG:8705
  udGZGD_MOON_MERC,  //EPSG:30174,30175
  udGZGD_MOON_PCPF,  //EPSG:30100,30100

  udGZGD_Count
};

enum udGeoZoneProjectionType
{
  udGZPT_Unknown,

  udGZPT_ECEF,
  udGZPT_LongLat,
  udGZPT_LatLong,

  udGZPT_TransverseMercator,
  udGZPT_LambertConformalConic2SP,
  udGZPT_WebMercator,

  udGZPT_CassiniSoldner,
  udGZPT_CassiniSoldnerHyperbolic,

  udGZPT_SterographicObliqueNEquatorial,

  udGZPT_Mercator,

  udGZPT_Count
};

struct udGeoZone
{
  udGeoZoneGeodeticDatum datum;
  udGeoZoneProjectionType projection;
  udDouble2 latLongBoundMin;
  udDouble2 latLongBoundMax;
  double meridian;
  double parallel;            // Parallel of origin for Transverse Mercator
  double flattening;
  double semiMajorAxis;
  double semiMinorAxis;
  double thirdFlattening;
  double eccentricity;
  double eccentricitySq;
  double radius;
  double scaleFactor;
  double n[10];
  double alpha[9];
  double beta[9];
  double firstParallel;
  double secondParallel;
  double falseNorthing;
  double falseEasting;
  double unitMetreScale;      // 1.0 for metres, 0.3048006096012192 for feet
  int32_t zone;
  int32_t srid;
  char datumShortName[16];
  char datumName[64];
  char zoneName[64]; // Only 33 characters required for longest known name "Japan Plane Rectangular CS XVIII"
  char displayName[128]; // This is the human readable name; often just datumShortName & zoneName concatenated
};

// Stored as g_udGZ_GeodeticDatumDescriptors
struct udGeoZoneGeodeticDatumDescriptor
{
  const char *pFullName;
  const char *pShortName;
  const char *pDatumName;
  udGeoZoneEllipsoid ellipsoid;
  double paramsHelmert7[7]; //TO-WGS84 as { Tx, Ty, Tz, Rx, Ry, Rz, DS }
  int32_t epsg; // epsg code for the datum
  int32_t authority; // authority for this datum
  bool exportAxisInfo;
  bool exportToWGS84;
};

struct udGeoZoneEllipsoidInfo
{
  const char *pName;
  double semiMajorAxis;
  double flattening;
  int32_t authorityEpsg;
};

extern const udGeoZoneGeodeticDatumDescriptor g_udGZ_GeodeticDatumDescriptors[udGZGD_Count];
extern const udGeoZoneEllipsoidInfo g_udGZ_StdEllipsoids[udGZE_Count];

// Find an appropriate SRID code for a given lat/long within UTM/WGS84 (for example as a default value)
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Set the zone structure parameters from a given srid code
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode);

// Get geozone from well known text
udResult udGeoZone_SetFromWKT(udGeoZone *pZone, const char *pWKT);

// Get the Well Known Text for a zone
udResult udGeoZone_GetWellKnownText(const char **ppWKT, const udGeoZone &zone);

// Convert a point from lat/long to the cartesian coordinate system defined by the zone
udDouble3 udGeoZone_LatLongToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Convert a point from the cartesian coordinate system defined by the zone to lat/long
udDouble3 udGeoZone_CartesianToLatLong(const udGeoZone &zone, const udDouble3 &position, bool flipToLongLat = false, udGeoZoneGeodeticDatum datum = udGZGD_WGS84);

// Conversion to and from Geocentric
udDouble3 udGeoZone_LatLongToGeocentric(udDouble3 latLong, const udGeoZoneEllipsoidInfo &ellipsoid);
udDouble3 udGeoZone_GeocentricToLatLong(udDouble3 geoCentric, const udGeoZoneEllipsoidInfo &ellipsoid);

// Convert a lat/long pair in one datum to another datum
udDouble3 udGeoZone_ConvertDatum(udDouble3 latLong, udGeoZoneGeodeticDatum currentDatum, udGeoZoneGeodeticDatum newDatum, bool flipToLongLat = false);

// Transform a point from one zone to another
udDouble3 udGeoZone_TransformPoint(const udDouble3 &point, const udGeoZone &sourceZone, const udGeoZone &destZone);

// Transform a matrix from one zone to another
udDouble4x4 udGeoZone_TransformMatrix(const udDouble4x4 &matrix, const udGeoZone &sourceZone, const udGeoZone &destZone);

#endif // UDGEOZONE_H
