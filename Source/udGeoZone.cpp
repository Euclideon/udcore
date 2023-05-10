#include "udGeoZone.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"
#include "udJSON.h"
#include "udFile.h"
#include "udThread.h"

const udGeoZoneEllipsoidInfo g_udGZ_StdEllipsoids[udGZE_Count] = {
  // WKT Sphere name      semiMajor    flattening           authority epsg
  { "WGS 84",             6378137.000, 1.0 / 298.257223563, 7030 }, // udGZE_WGS84
  { "Airy 1830",          6377563.396, 1.0 / 299.3249646,   7001 }, // udGZE_Airy1830
  { "Airy Modified 1849", 6377340.189, 1.0 / 299.3249646,   7002 }, // udGZE_AiryModified
  { "Bessel 1841",        6377397.155, 1.0 / 299.1528128,   7004 }, // udGZE_Bessel1841
  { "Bessel Modified",    6377492.018, 1.0 / 299.1528128,   7005 }, // udGZE_BesselModified
  { "Clarke 1866",        6378206.400, 1.0 / 294.978698214, 7008 }, // udGZE_Clarke1866
  { "Clarke 1880 (IGN)",  6378249.200, 1.0 / 293.466021294, 7011 }, // udGZE_Clarke1880IGN
  { "GRS 1980",           6378137.000, 1.0 / 298.257222101, 7019 }, // udGZE_GRS80
  { "International 1924", 6378388.000, 1.0 / 297.00,        7022 }, // udGZE_Intl1924
  { "WGS 72",             6378135.000, 1.0 / 298.26,        7043 }, // udGZE_WGS72
  { "CGCS2000",           6378137.000, 1.0 / 298.257222101, 1024 }, // udGZE_CGCS2000
  { "Clarke 1858",        6378293.64520876, 1.0 / 294.260676369, 7007 }, // udGZE_Clarke1858
  { "Clarke 1880 (international foot)", 6378306.369, 1.0 / 293.466307656, 7055 }, // udGZE_Clarke1880FOOT
  { "Krassowsky 1940",    6378245.000, 1.0 / 298.3,         7024 }, // udGZE_Krassowsky1940
  { "Everest 1830 Modified",6377304.063, 1.0 / 300.8017,    7018 }, // udGZE_Everest1930M
  { "Mars_2000_IAU_IAG",  3396190.000, 1.0 / 169.894447224, 49900 },// udGZE_MARS
  { "Moon_2000_IAU_IAG",  1737400.000, 0.0,                 39064 },// udGZE_MOON
  { "IAG 1975",           6378140.000, 1.0 / 298.257,       7049 }, // udGZE_IAG1975
  { "Everest 1830 (1967 Definition)",6377298.556, 1.0 / 300.8017,    7016 }, // udGZE_Everest1830
  { "GRS 1967",           6378160.000, 1.0 / 298.247167427, 7036 }, // udGZE_GRS67
  { "Australian National Spheroid",6378160, 1.0 / 298.25,   7003 }, //udGZE_ANS
  { "Indonesian National Spheroid",6378160, 1.0 / 298.247,  7021 }, //udGZE_INS 
};

// Data for table gathered from https://github.com/chrisveness/geodesy/blob/master/latlon-ellipsoidal.js
// and cross referenced with http://epsg.io/
const udGeoZoneGeodeticDatumDescriptor g_udGZ_GeodeticDatumDescriptors[] = {
  // Full Name,                              Short  name        Datum name                                      Ellipsoid index      // ToWGS84 parameters                                               epsg  auth, AxisInfo, ToWGS84
  { "WGS 84",                                "WGS 84",          "WGS_1984",                                     udGZE_WGS84,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4326, 6326, true,     false },
  { "ED50",                                  "ED50",            "European_Datum_1950",                          udGZE_Intl1924,      { -87.0, -98.0, -121.0, 0.0, 0.0, 0.0, 0.0 },                       4230, 6320, true,     true  },
  { "ETRS89",                                "ETRS89",          "European_Terrestrial_Reference_System_1989",   udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4258, 6258, true,     true  },
  { "TM75",                                  "TM75",            "Geodetic_Datum_of_1965",                       udGZE_AiryModified,  { 482.5, -130.6, 564.6, -1.042, -0.214, -0.631, 8.15 },             4300, 6300, true,     true  },
  { "NAD27",                                 "NAD27",           "North_American_Datum_1927",                    udGZE_Clarke1866,    { -8.0, 160.0, 176.0, 0.0, 0.0, 0.0, 0.0 },                         4267, 6267, true,     true  },
  { "NAD27(CGQ77)",                          "NAD27(CGQ77)",    "North_American_Datum_1927_CGQ77",              udGZE_Clarke1866,    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4609, 6609, true,     false  },
  { "NAD83",                                 "NAD83",           "North_American_Datum_1983",                    udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4269, 6269, true,     true  },
  { "NAD83(CORS96)",                         "NAD83(CORS96)",   "NAD83_National_Spatial_Reference_System_1996", udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              6783, 1133, true,     false },
  { "NAD83(CSRS)",                           "NAD83(CSRS)",     "NAD83_Canadian_Spatial_Reference_System",      udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              2955, 4617, true,     true  },
  { "NAD83(NSRS2007)",                       "NAD83(NSRS2007)", "NAD83_National_Spatial_Reference_System_2007", udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              3465, 4759, true,     true  },
  { "NAD83(2011)",                           "NAD83(2011)",     "NAD83_National_Spatial_Reference_System_2011", udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              6318, 1116, true,     false },
  { "NTF",                                   "NTF",             "Nouvelle_Triangulation_Francaise",             udGZE_Clarke1880IGN, { -168.0, -60.0, 320.0, 0.0, 0.0, 0.0, 0.0 },                       4275, 6275, true,     true  },
  { "OSGB 1936",                             "OSGB 1936",       "OSGB_1936",                                    udGZE_Airy1830,      { 446.448, -125.157, 542.06, 0.1502, 0.247, 0.8421, -20.4894 },     4277, 6277, true,     true  },
  { "PD / 83",                               "PD / 83",         "Potsdam_Datum_83",                             udGZE_Bessel1841,    { 582.0, 105.0, 414.0, -1.04, -0.35, 3.08, 8.3 },                   4746, 6746, true,     true  },
  { "Tokyo",                                 "Tokyo",           "Tokyo",                                        udGZE_Bessel1841,    { -146.414, 507.337, 680.507, 0.0, 0.0, 0.0, 0.0 },                 7414, 6301, true,     true  },
  { "WGS 72",                                "WGS 72",          "WGS_1972",                                     udGZE_WGS72,         { 0.0, 0.0, 4.5, 0.0, 0.0, 0.554, 0.2263 },                         4322, 6322, true,     true  },
  { "JGD2000",                               "JGD2000",         "Japanese_Geodetic_Datum_2000",                 udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4612, 6612, false,    true  },
  { "JGD2011",                               "JGD2011",         "Japanese_Geodetic_Datum_2011",                 udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              6668, 1128, false,    false },
  { "GDA94",                                 "GDA94",           "Geocentric_Datum_of_Australia_1994",           udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4283, 6283, true,     true  },
  { "GDA2020",                               "GDA2020",         "Geocentric_Datum_of_Australia_2020",           udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              7844, 1168, true,     false },
  { "RGF93",                                 "RGF93",           "Reseau_Geodesique_Francais_1993",              udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4171, 6171, true,     true  },
  { "NAD83(HARN)",                           "NAD83(HARN)",     "NAD83_High_Accuracy_Reference_Network",        udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4152, 6152, true,     true  },
  { "China Geodetic Coordinate System 2000", "CGCS2000",        "China_2000",                                   udGZE_CGCS2000,      { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4490, 1043, false,    false },
  { "Hong Kong 1980",                        "Hong Kong 1980",  "Hong_Kong_1980",                               udGZE_Intl1924,      { -162.619,-276.959,-161.764,0.067753,-2.24365,-1.15883,-1.09425 }, 4611, 6611, false,    true  },
  { "SVY21",                                 "SVY21",           "SVY21",                                        udGZE_WGS84,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4757, 6757, false,    false },
  { "MGI",                                   "MGI",             "Militar_Geographische_Institute",              udGZE_Bessel1841,    { 577.326, 90.129, 463.919, 5.137, 1.474, 5.297, 2.4232 },          4312, 6312, false,    true  },
  { "NZGD2000",                              "NZGD2000",        "New_Zealand_Geodetic_Datum_2000",              udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4167, 6167, false,    true  },
  { "Amersfoort",                            "Amersfoort",      "Amersfoort",                                   udGZE_Bessel1841,    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4289, 6289, true,     false },
  { "Trinidad 1903",                         "Trinidad_1903",   "Trinidad_1903",                                udGZE_Clarke1858,    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4302, 6302, true,     false },
  { "Vanua Levu 1915",                       "Vanua_Levu_1915", "Vanua_Levu_1915",                              udGZE_Clarke1880FOOT,{ 51.0, 391.0, -36.0, 0.0, 0.0, 0.0, 0.0 },                         4748, 6748, false,    true  },
  { "Dealul Piscului 1970",                  "Dealul_1970",     "Dealul_Piscului_1970",                         udGZE_Krassowsky1940,{ 28,-121,-77, 0, 0, 0, 0 },                                        4317, 6317, false,    true  },
  { "Singapore Grid",                        "Singapore Grid",  "Singapore Grid",                               udGZE_Everest1930M,  { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4245, 6245, false,    false },
  { "Mars 2000 Mercator",                    "Mars 2000",       "D_Mars_2000",                                  udGZE_Mars,          { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              490000, 490001, false,true  },
  { "Mars 2000 / ECEF",                      "Mars 2000",       "D_Mars_2000",                                  udGZE_Mars,          { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              490000, 490001, true, false },
  { "Moon 2000 Mercator",                    "Moon 2000",       "D_Moon_2000",                                  udGZE_Moon,          { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              39064, 39065, false,  true  },
  { "Moon 2000 / ECEF",                      "Moon 2000",       "D_Moon_2000",                                  udGZE_Moon,          { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              39064, 39065, true,   false },
  { "DB_REF",                                "DB_REF",          "Deutsche_Bahn_Reference_System",               udGZE_Bessel1841,    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              5681, 1081, true,     false },
  { "DHDN",                                  "DHDN",            "Deutsches_Hauptdreiecksnetz",                  udGZE_Bessel1841,    { 598.1, 73.7, 418.2, 0.202, 0.045, -2.455, 6.7 },                  4314, 6314, false,    true  },
  { "System of the Unified Trigonometrical Cadastral Network [JTSK03]", "JTSK03", "S-JTSK [JTSK03]",            udGZE_Bessel1841,    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              8353, 8351, true,     false },
  { "Pulkovo 1942",                          "Pulkovo_1942",    "Pulkovo_1942",                                 udGZE_Krassowsky1940,{ 23.92, -141.27, -80.9, 0, 0.35, 0.82, -0.12 },                    4284, 6284, true,     true  },
  { "Pulkovo 1942(58)",                      "Pulkovo_1942_58", "Pulkovo_1942_58",                              udGZE_Krassowsky1940,{ 33.4, -146.6, -76.3, -0.359, -0.053, 0.844, -0.84 },              4179, 6179, true,     true  },
  { "Pulkovo 1942(83)",                      "Pulkovo_1942_83", "Pulkovo_1942_83",                              udGZE_Krassowsky1940,{ 26.0, -121.0, -78.0, 0.0, 0.0, 0.0, 0 },                          4178, 6178, true,     true  },
  { "Pulkovo 1995",                          "Pulkovo_1995",    "Pulkovo_1995",                                 udGZE_Krassowsky1940,{ 24.47, -130.89, -81.56, 0, 0, 0.13, -0.22 },                      20004, 4200, true,    true  },
  { "WGS 72BE",                              "WGS_72BE",        "WGS_1972_Transit_Broadcast_Ephemeris",         udGZE_WGS72,         { 0, 0, 1.9, 0, 0, 0.814, -0.38 },                                  4324, 6324, true,     true  },
  { "Beijing 1954",                          "Beijing_1954",    "Beijing_1954",                                 udGZE_Krassowsky1940,{ 15.8, -154.4, -82.3, 0, 0, 0, 0 },                                4214, 6214, false,    true  },
  { "New Beijing",                           "New_Beijing",     "New_Beijing",                                  udGZE_Krassowsky1940,{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4555, 1045, false,    false },
  { "Xian 1980",                             "Xian_1980",       "Xian_1980",                                    udGZE_IAG1975,       { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4610, 6610, false,    false },
  { "Timbalai 1948 / Tso Borneo (m)",        "Timbalai 1948",   "Timbalai_1948",                                udGZE_Everest1830,   { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              29873, 6298, true,    false },
  { "NZGD49",                                "NZGD49",          "New_Zealand_Geodetic_Datum_1949",              udGZE_Intl1924,      { 59.47, -5.04, 187.44, 0.47, -0.1, 1.024, -4.5993 },               4272, 6272, true,     true  },
  { "SWEREF99",                              "SWEREF99",        "SWEREF99",                                     udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4619, 6619, true,     true  },
  { "SAD69",                                 "SAD69",           "South_American_Datum_1969",                    udGZE_GRS67,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4291, 6291, true,     false },
  { "GR96",                                  "GR96",            "Greenland_1996",                               udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4747, 6747, false,    true  },
  { "DGN95",                                 "DGN95",           "Datum_Geodesi_Nasional_1995",                  udGZE_WGS84,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4755, 6755, false,    true  },
  { "UCS-2000",                              "UCS-2000",        "Ukraine_2000",                                 udGZE_Krassowsky1940,{ 25.0, -141.0, -78.5, 0.0, 0.35, 0.736, 0 },                       5561, 1077, true,     true  },
  { "Hartebeesthoek94",                      "Hartebeesthoek94","Hartebeesthoek94",                             udGZE_WGS84,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4148, 6148, false,    true  },
  { "ID74",                                  "ID74",            "Indonesian_Datum_1974",                        udGZE_INS,           { -24.0, -15.0, 5.0, 0.0, 0.0, 0.0, 0 },                            4238, 6238, true,     true  },
  { "NGO 1948",                              "NGO_1948",        "NGO_1948",                                     udGZE_BesselModified,{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              4273, 6273, true,     false },
  { "RGF93 v2b",                             "RGF93 v2b",       "Reseau_Geodesique_Francais_1993_v2b",          udGZE_GRS80,         { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },                              9782, 6171, true,     true  },
};

UDCOMPILEASSERT(udLengthOf(g_udGZ_GeodeticDatumDescriptors) == udGZGD_Count, "Update above descriptor table!");

//Temp Datum descriptor table to support unknown Datum
udChunkedArray<udGeoZoneGeodeticDatumDescriptor> g_InternalDatumList = {};

const udGeoZoneDatumAlias g_udGZ_DatumAlias[] = {
  { "NTF (Paris)", udGZGD_NTF },
  { "RGRDC 2005", udGZGD_ETRS89 },
  { "SIRGAS 2000", udGZGD_JGD2000 },
  { "NAD83(PA11)", udGZGD_NAD83_2011 },
  { "DRUKREF 03", udGZGD_SWEREF99 },
  { "SIRGAS 1995", udGZGD_NAD83_1996 },
  { "NAD83(CSRS98)", udGZGD_JGD2000 },
  { "GCS_ETRF_1989", udGZGD_WGS84 },
  { "Korean 1985", udGZGD_AMERSFOORT },
  { "TUREF", udGZGD_JGD2000 },
  { "Korea 2000", udGZGD_NAD83_2011 },
};

udChunkedArray<udGeoZone> g_InternalGeoZoneList = {};
udMutex *g_pMutex = nullptr;

class udGeoZone_GlobalInit
{
public:
  udGeoZone_GlobalInit() noexcept
  {
    g_pMutex = udCreateMutex();
    g_InternalGeoZoneList.Init(64);
    g_InternalDatumList.Init(64);
  }
  ~udGeoZone_GlobalInit() noexcept
  {
    g_InternalDatumList.Deinit();
    g_InternalGeoZoneList.Deinit();
    udDestroyMutex(&g_pMutex);
  }
} g_initGeoZone;

int udGeoZone_InternalIndexOf(int srid)
{
  int left = 0;
  int right = (int)g_InternalGeoZoneList.length - 1;
  int m = (left + right) / 2;
  int compare = 0;

  while (left <= right)
  {
    m = (left + right) / 2;
    compare = g_InternalGeoZoneList[m].srid - srid;

    if (compare < 0)
      left = m + 1;
    else if (compare > 0)
      right = m - 1;
    else
      return m;
  }

  if (compare < 0)
    return ~(m + 1);

  return ~m;
}

udResult udGeoZone_LoadZonesFromJSON(const char *pJSONStr, int *pLoaded, int *pFailed)
{
  udScopeLock scopeLock(g_pMutex);

  udResult result = udR_Failure;
  udJSON zones = {};

  int loaded = 0;
  int mismatched = 0;
  int failed = 0;
  udUnused(mismatched);

  int highestLoaded = 0;
  udChunkedArray<udJSONKVPair> *pMembers = nullptr;

  UD_ERROR_CHECK(zones.Parse(pJSONStr));
  UD_ERROR_IF(!zones.IsObject(), udR_FormatVariationNotSupported);

  if (g_InternalGeoZoneList.length > 0)
    highestLoaded = g_InternalGeoZoneList[g_InternalGeoZoneList.length - 1].srid;

  pMembers = zones.AsObject();
  for (udJSONKVPair &item : *pMembers)
  {
    const udJSON *pWKT = &item.value;

    const char *pOutputWKT = nullptr;
    udGeoZone zone = {};

    int expectedSRID = 0;
    const char *pSRIDStr = nullptr;

    if (!pWKT->IsString())
    {
      ++failed;
      continue;
    }

    if (udGeoZone_SetFromWKT(&zone, pWKT->AsString()) != udR_Success)
    {
      ++failed;
      if constexpr (UD_DEBUG)
        udDebugPrintf("%s\n", pWKT->AsString());

      continue;
    }

    pSRIDStr = udStrstr(item.pKey, 0, ":");
    if (pSRIDStr != nullptr)
      expectedSRID = udStrAtoi(pSRIDStr+1);
    else
      expectedSRID = udStrAtoi(item.pKey);

    if (expectedSRID != zone.srid)
    {
      ++failed;
      continue;
    }

    if (udGeoZone_GetWellKnownText(&pOutputWKT, zone) == udR_Success)
    {
      if (!udStrEquali(pWKT->AsString(), pOutputWKT))
        ++mismatched; // This test is stupid because of all sorts of "valid" combinations here...

      udFree(pOutputWKT);
    }
    else
    {
      ++mismatched;
    }

    if (zone.srid > highestLoaded)
    {
      g_InternalGeoZoneList.PushBack(zone);
      highestLoaded = zone.srid;
    }
    else
    {
      int index = udGeoZone_InternalIndexOf(expectedSRID);

      if (index >= 0) // Already exists? Might be a different authority?
        UD_ERROR_SET(udR_Unsupported);
      else
        g_InternalGeoZoneList.Insert(~index, &zone);
    }

    ++loaded;
  }

  result = udR_Success;

epilogue:
  if (pLoaded != nullptr)
    *pLoaded = loaded;

  if (pFailed != nullptr)
    *pFailed = failed;

  return result;
}

udResult udGeoZone_UnloadZones()
{
  udScopeLock scopeLock(g_pMutex);

  udResult result;
  UD_ERROR_CHECK(g_InternalDatumList.Deinit());
  UD_ERROR_CHECK(g_InternalGeoZoneList.Deinit());
  UD_ERROR_CHECK(g_InternalGeoZoneList.Init(64));
  UD_ERROR_CHECK(g_InternalDatumList.Init(64));

  result = udR_Success;
epilogue:
  return result;
}

udDouble3 udGeoZone_LatLongToGeocentric(udDouble3 latLong, const udGeoZoneEllipsoidInfo &ellipsoid)
{
  double lat = UD_DEG2RAD(latLong.x);
  double lon = UD_DEG2RAD(latLong.y);
  double h = latLong.z;

  double eSq = ellipsoid.flattening * (2 - ellipsoid.flattening);
  double v = ellipsoid.semiMajorAxis / udSqrt(1 - eSq * udSin(lat) * udSin(lat));

  double x = (v + h) * udCos(lat) * udCos(lon);
  double y = (v + h) * udCos(lat) * udSin(lon);
  double z = (v * (1 - eSq) + h) * udSin(lat);

  return udDouble3::create(x, y, z);
}

udDouble3 udGeoZone_GeocentricToLatLong(udDouble3 geoCentric, const udGeoZoneEllipsoidInfo &ellipsoid)
{
  double semiMinorAxis = ellipsoid.semiMajorAxis * (1 - ellipsoid.flattening);

  double eSq = ellipsoid.flattening * (2 - ellipsoid.flattening);
  double e3 = eSq / (1 - eSq);
  double p = udSqrt(geoCentric.x * geoCentric.x + geoCentric.y * geoCentric.y);
  double q = udATan2(geoCentric.z * ellipsoid.semiMajorAxis, p * semiMinorAxis);

  double sinQ = udSin(q);
  double cosQ = udCos(q);

  double sinQ3 = sinQ * sinQ * sinQ;
  double cosQ3 = cosQ * cosQ * cosQ;

  double lat = udATan2(geoCentric.z + e3 * semiMinorAxis * sinQ3, p - eSq * ellipsoid.semiMajorAxis * cosQ3);
  double lon = udATan2(geoCentric.y, geoCentric.x);

  double v = ellipsoid.semiMajorAxis / udSqrt(1 - eSq * udSin(lat) * udSin(lat)); // length of the normal terminated by the minor axis
  double h = (p / udCos(lat)) - v;

  // This is an alternative method to generate the lat- don't merge until we confirm which one is 'correct'
  double lat2 = 0;
  double lat2Temp = 1;
  while (lat2 != lat2Temp && isfinite(lat2))
  {
    lat2 = lat2Temp;
    lat2Temp = udATan((geoCentric.z + eSq*v*udSin(lat2)) / udSqrt(geoCentric.x * geoCentric.x + geoCentric.y * geoCentric.y));
  }

  return udDouble3::create(UD_RAD2DEG(lat2), UD_RAD2DEG(lon), h);
}

udDouble3 udGeoZone_ApplyTransform(udDouble3 geoCentric, const udGeoZoneGeodeticDatumDescriptor &transform)
{
  // transform parameters
  double rx = UD_DEG2RAD(transform.paramsHelmert7[3] / 3600.0); // x-rotation: normalise arcseconds to radians
  double ry = UD_DEG2RAD(transform.paramsHelmert7[4] / 3600.0); // y-rotation: normalise arcseconds to radians
  double rz = UD_DEG2RAD(transform.paramsHelmert7[5] / 3600.0); // z-rotation: normalise arcseconds to radians
  double ds = transform.paramsHelmert7[6] / 1000000.0 + 1.0; // scale: normalise parts-per-million to (s+1)

  // apply transform
  double x2 = transform.paramsHelmert7[0] + (ds * geoCentric.x - geoCentric.y*rz + geoCentric.z*ry);
  double y2 = transform.paramsHelmert7[1] + (geoCentric.x*rz + ds * geoCentric.y - geoCentric.z*rx);
  double z2 = transform.paramsHelmert7[2] + (-geoCentric.x*ry + geoCentric.y*rx + ds * geoCentric.z);

  return udDouble3::create(x2, y2, z2);
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, August 2018
udDouble3 udGeoZone_ConvertDatum(udDouble3 latLong, udGeoZoneGeodeticDatum currentDatum, udGeoZoneGeodeticDatum newDatum, bool flipToLongLat /*= false*/)
{
  udDouble3 oldLatLon = latLong;
  udGeoZoneGeodeticDatum oldDatum = currentDatum;

  if (flipToLongLat)
  {
    oldLatLon.x = latLong.y;
    oldLatLon.y = latLong.x;
  }

  const udGeoZoneGeodeticDatumDescriptor *pTransform = nullptr;
  udGeoZoneGeodeticDatumDescriptor transform = {};

  if (currentDatum != udGZGD_WGS84 && newDatum != udGZGD_WGS84)
  {
    oldLatLon = udGeoZone_ConvertDatum(oldLatLon, currentDatum, udGZGD_WGS84);
    oldDatum = udGZGD_WGS84;
  }

  udGeoZoneEllipsoid oldEllipsoid;
  udGeoZoneEllipsoid newEllipsoid;
  if (currentDatum < udGZGD_Count)
  {
    oldEllipsoid = g_udGZ_GeodeticDatumDescriptors[oldDatum].ellipsoid;
    newEllipsoid = g_udGZ_GeodeticDatumDescriptors[newDatum].ellipsoid;
  }
  else
  {
    oldEllipsoid = g_InternalDatumList[oldDatum - udGZGD_Count - 1].ellipsoid;
    newEllipsoid = g_InternalDatumList[newDatum - udGZGD_Count - 1].ellipsoid;
  }

  if (newDatum == udGZGD_WGS84) // converting to WGS84; use inverse transform
  {
    if (oldDatum < udGZGD_Count)
      pTransform = &g_udGZ_GeodeticDatumDescriptors[oldDatum];
    else
      pTransform = &g_InternalDatumList[oldDatum - udGZGD_Count - 1];
  }
  else // converting from WGS84
  {
    if (oldDatum < udGZGD_Count)
      transform = g_udGZ_GeodeticDatumDescriptors[newDatum];
    else
      transform = g_InternalDatumList[newDatum - udGZGD_Count - 1];

    for (int i = 0; i < 7; i++)
      transform.paramsHelmert7[i] = -transform.paramsHelmert7[i];
    pTransform = &transform;
  }

  // Chain functions and get result
  udDouble3 geocentric = udGeoZone_LatLongToGeocentric(oldLatLon, g_udGZ_StdEllipsoids[oldEllipsoid]);
  udDouble3 transformed = udGeoZone_ApplyTransform(geocentric, *pTransform);
  udDouble3 newLatLong = udGeoZone_GeocentricToLatLong(transformed, g_udGZ_StdEllipsoids[newEllipsoid]);

  if (flipToLongLat)
    return udDouble3::create(newLatLong.y, newLatLong.x, newLatLong.z);
  else
    return newLatLong;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
udResult udGeoZone_FindSRID(int32_t *pSRIDCode, const udDouble3 &latLong, bool flipFromLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  double lat = !flipFromLongLat ? latLong.x : latLong.y;
  double lon = !flipFromLongLat ? latLong.y : latLong.x;

  if (datum != udGZGD_WGS84)
  {
    udDouble3 fixedLatLon = udGeoZone_ConvertDatum(udDouble3::create(lat, lon, 0.f), datum, udGZGD_WGS84);
    lat = fixedLatLon.x;
    lon = fixedLatLon.y;
  }

  int32_t zone = (uint32_t)(udFloor(lon + 186.0) / 6.0);
  if (zone < 1 || zone > 60)
    return udR_NotFound;

  int32_t sridCode = (lat >= 0) ? zone + 32600 : zone + 32700;
  if (pSRIDCode)
    *pSRIDCode = sridCode;
  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
static void udGeoZone_SetUTMZoneBounds(udGeoZone *pZone, bool northernHemisphere)
{
  pZone->latLongBoundMin.x = (northernHemisphere) ? 0 : -80;
  pZone->latLongBoundMax.x = (northernHemisphere) ? 84 : 0;
  pZone->latLongBoundMin.y = pZone->meridian - 3;
  pZone->latLongBoundMax.y = pZone->meridian + 3;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static void udGeoZone_SetSpheroid(udGeoZone *pZone)
{
  if (pZone->semiMajorAxis == 0.0 && pZone->flattening == 0.0 && pZone->datum < udGZGD_Count)
  {
    udGeoZoneEllipsoid ellipsoidID;
    if (pZone->datum < udGZGD_Count)
    {
      pZone->knownDatum = true;
      ellipsoidID = g_udGZ_GeodeticDatumDescriptors[pZone->datum].ellipsoid;
    }
    else
    {
      pZone->knownDatum = false;
      ellipsoidID = g_InternalDatumList[pZone->datum - udGZGD_Count - 1].ellipsoid;
    }

    if (ellipsoidID < udGZE_Count)
    {
      const udGeoZoneEllipsoidInfo &ellipsoidInfo = g_udGZ_StdEllipsoids[ellipsoidID];
      pZone->semiMajorAxis = ellipsoidInfo.semiMajorAxis / pZone->unitMetreScale;
      pZone->flattening = ellipsoidInfo.flattening;
    }
  }

  udGeoZoneGeodeticDatumDescriptor datumDescriptor;
  if (pZone->datum < udGZGD_Count)
  {
    datumDescriptor = g_udGZ_GeodeticDatumDescriptors[pZone->datum];
  }
  else
  {
    datumDescriptor = g_InternalDatumList[pZone->datum - udGZGD_Count - 1];
  }

  for (int i = 0; i < 7; ++i)
    pZone->paramsHelmert7[i] = datumDescriptor.paramsHelmert7[i];

  pZone->semiMinorAxis = pZone->semiMajorAxis * (1 - pZone->flattening);
  pZone->thirdFlattening = pZone->flattening / (2 - pZone->flattening);
  pZone->eccentricitySq = pZone->flattening * (2 - pZone->flattening);
  pZone->eccentricity = udSqrt(pZone->eccentricitySq);
  pZone->n[0] = 1.0;
  pZone->n[1] = pZone->thirdFlattening;
  pZone->n[2] = pZone->thirdFlattening * pZone->n[1];
  pZone->n[3] = pZone->thirdFlattening * pZone->n[2];
  pZone->n[4] = pZone->thirdFlattening * pZone->n[3];
  pZone->n[5] = pZone->thirdFlattening * pZone->n[4];
  pZone->n[6] = pZone->thirdFlattening * pZone->n[5];
  pZone->n[7] = pZone->thirdFlattening * pZone->n[6];
  pZone->n[8] = pZone->thirdFlattening * pZone->n[7];
  pZone->n[9] = pZone->thirdFlattening * pZone->n[8];

  // The alpha and beta constant below can be cross-referenced with https://geographiclib.sourceforge.io/html/transversemercator.html
  pZone->alpha[0] = 1.0 / 2.0 * pZone->n[1] - 2.0 / 3.0 * pZone->n[2] + 5.0 / 16.0 * pZone->n[3] + 41.0 / 180.0 * pZone->n[4] - 127.0 / 288.0 * pZone->n[5] + 7891.0 / 37800.0 * pZone->n[6] + 72161.0 / 387072.0 * pZone->n[7] - 18975107.0 / 50803200.0 * pZone->n[8] + 60193001.0 / 290304000.0 * pZone->n[9];
  pZone->alpha[1] = 13.0 / 48.0 * pZone->n[2] - 3.0 / 5.0 * pZone->n[3] + 557.0 / 1440.0 *pZone->n[4] + 281.0 / 630.0 * pZone->n[5] - 1983433.0 / 1935360.0 * pZone->n[6] + 13769.0 / 28800.0 * pZone->n[7] + 148003883.0 / 174182400.0 * pZone->n[8] - 705286231.0 / 465696000.0 * pZone->n[9];
  pZone->alpha[2] = 61.0 / 240.0 * pZone->n[3] - 103.0 / 140.0 * pZone->n[4] + 15061.0 / 26880.0 * pZone->n[5] + 167603.0 / 181440.0 * pZone->n[6] - 67102379.0 / 29030400.0 * pZone->n[7] + 79682431.0 / 79833600.0 * pZone->n[8] + 6304945039.0 / 2128896000.0 * pZone->n[9];
  pZone->alpha[3] = 49561.0 / 161280.0 * pZone->n[4] - 179.0 / 168.0 * pZone->n[5] + 6601661.0 / 7257600.0 * pZone->n[6] + 97445.0 / 49896.0 * pZone->n[7] - 40176129013.0 / 7664025600.0 * pZone->n[8] + 138471097.0 / 66528000.0 * pZone->n[9];
  pZone->alpha[4] = 34729.0 / 80640.0 * pZone->n[5] - 3418889.0 / 1995840.0 * pZone->n[6] + 14644087.0 / 9123840.0 * pZone->n[7] + 2605413599.0 / 622702080.0 * pZone->n[8] - 31015475399.0 / 2583060480.0 * pZone->n[9];
  pZone->alpha[5] = 212378941.0 / 319334400.0 * pZone->n[6] - 30705481.0 / 10378368.0 * pZone->n[7] + 175214326799.0 / 58118860800.0 * pZone->n[8] + 870492877.0 / 96096000.0 * pZone->n[9];
  pZone->alpha[6] = 1522256789.0 / 1383782400.0 * pZone->n[7] - 16759934899.0 / 3113510400.0 * pZone->n[8] + 1315149374443.0 / 221405184000.0 * pZone->n[9];
  pZone->alpha[7] = 1424729850961.0 / 743921418240.0 * pZone->n[8] - 256783708069.0 / 25204608000.0 * pZone->n[9];
  pZone->alpha[8] = 21091646195357.0 / 6080126976000.0 * pZone->n[9];

  pZone->beta[0] = -1.0 / 2.0 * pZone->n[1] + 2.0 / 3.0 * pZone->n[2] - 37.0 / 96.0 * pZone->n[3] + 1.0 / 360.0 * pZone->n[4] + 81.0 / 512.0 * pZone->n[5] - 96199.0 / 604800.0 * pZone->n[6] + 5406467.0 / 38707200.0 * pZone->n[7] - 7944359.0 / 67737600.0 * pZone->n[8] + 7378753979.0 / 97542144000.0 * pZone->n[9];
  pZone->beta[1] = -1.0 / 48.0 * pZone->n[2] - 1.0 / 15.0 * pZone->n[3] + 437.0 / 1440.0 * pZone->n[4] - 46.0 / 105.0 * pZone->n[5] + 1118711.0 / 3870720.0 * pZone->n[6] - 51841.0 / 1209600.0 * pZone->n[7] - 24749483.0 / 348364800.0 * pZone->n[8] + 115295683.0 / 1397088000.0 * pZone->n[9];
  pZone->beta[2] = -17.0 / 480.0 * pZone->n[3] + 37.0 / 840.0 * pZone->n[4] + 209.0 / 4480.0 * pZone->n[5] - 5569.0 / 90720.0 * pZone->n[6] - 9261899.0 / 58060800.0 * pZone->n[7] + 6457463.0 / 17740800.0 * pZone->n[8] - 2473691167.0 / 9289728000.0 * pZone->n[9];
  pZone->beta[3] = -4397.0 / 161280.0 * pZone->n[4] + 11.0 / 504.0 * pZone->n[5] + 830251.0 / 7257600.0 * pZone->n[6] - 466511.0 / 2494800.0 * pZone->n[7] - 324154477.0 / 7664025600.0 * pZone->n[8] + 937932223.0 / 3891888000.0 * pZone->n[9];
  pZone->beta[4] = -4583.0 / 161280.0 * pZone->n[5] + 108847.0 / 3991680.0 * pZone->n[6] + 8005831.0 / 63866880.0 * pZone->n[7] - 22894433.0 / 124540416.0 * pZone->n[8] - 112731569449.0 / 557941063680.0 * pZone->n[9];
  pZone->beta[5] = -20648693.0 / 638668800.0 * pZone->n[6] + 16363163.0 / 518918400.0 * pZone->n[7] + 2204645983.0 / 12915302400.0 * pZone->n[8] - 4543317553.0 / 18162144000.0 * pZone->n[9];
  pZone->beta[6] = -219941297.0 / 5535129600.0 * pZone->n[7] + 497323811.0 / 12454041600.0 * pZone->n[8] + 79431132943.0 / 332107776000.0 * pZone->n[9];
  pZone->beta[7] = -191773887257.0 / 3719607091200.0 * pZone->n[8] + 17822319343.0 / 336825216000.0 * pZone->n[9];
  pZone->beta[8] = -11025641854267.0 / 158083301376000.0 * pZone->n[9];

  pZone->radius = pZone->semiMajorAxis / (1 + pZone->n[1]) * (1.0 + 1.0 / 4.0 * pZone->n[2] + 1.0 / 64.0 *pZone->n[4] + 1.0 / 256.0 * pZone->n[6] + 25.0 / 16384.0 * pZone->n[8]);

  if (pZone->firstParallel == 0.0 && pZone->secondParallel == 0.0 && pZone->parallel != 0) // Latitude of origin for Transverse Mercator
  {
    double l0 = UD_DEG2RAD(pZone->parallel);
    double q0 = udASinh(udTan(l0)) - (pZone->eccentricity * udATanh(pZone->eccentricity * udSin(l0)));
    double b0 = udATan(udSinh(q0));

    double u = b0;
    for (size_t i = 0; i < UDARRAYSIZE(pZone->alpha); i++)
    {
      double j = (i + 1) * 2.0;
      u += pZone->alpha[i] * udSin(j * b0);
    }

    pZone->firstParallel = u * pZone->radius;
  }
}

// ----------------------------------------------------------------------------
// Author: Paul Fox, April 2020
udResult udGeoZone_UpdateDisplayName(udGeoZone *pZone)
{
  if (pZone == nullptr)
    return udR_InvalidParameter;

  const udGeoZoneGeodeticDatumDescriptor *pDesc;
  if (pZone->datum < udGZGD_Count)
    pDesc = &g_udGZ_GeodeticDatumDescriptors[pZone->datum];
  else
    pDesc = &g_InternalDatumList[pZone->datum - udGZGD_Count - 1];

  // Some zones include the datum in the zoneName- if it does thats the displayName as well
  if (udStrBeginsWith(pZone->zoneName, pDesc->pShortName))
    udStrcpy(pZone->displayName, pZone->zoneName);
  else if (pZone->projection == udGZPT_ECEF)
    udSprintf(pZone->displayName, "%s / ECEF", pDesc->pShortName);
  else if (pZone->projection == udGZPT_LatLong)
    udSprintf(pZone->displayName, "%s / LatLong", pDesc->pShortName);
  else if (pZone->projection == udGZPT_LongLat)
    udSprintf(pZone->displayName, "%s / LongLat", pDesc->pShortName);
  else
    udSprintf(pZone->displayName, "%s / %s", pDesc->pShortName, pZone->zoneName);

  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode)
{
  if (pZone == nullptr)
    return udR_InvalidParameter;

  if (sridCode == -1) // Special case to help with unit tests
    udGeoZone_SetSpheroid(pZone);
  else
    memset(pZone, 0, sizeof(udGeoZone));

  pZone->unitMetreScale = 1.0; // Default to metres as there's only a few in feet
  if (sridCode == 0)
  {
    udSprintf(pZone->displayName, "Not Geolocated");
  }
  else if (sridCode >= 32601 && sridCode <= 32660)
  {
    // WGS84 Northern Hemisphere
    pZone->datum = udGZGD_WGS84;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 32600;
    udSprintf(pZone->zoneName, "UTM zone %dN", pZone->zone);
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode >= 32701 && sridCode <= 32760)
  {
    // WGS84 Southern Hemisphere
    pZone->datum = udGZGD_WGS84;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 32700;
    udSprintf(pZone->zoneName, "UTM zone %dS", pZone->zone);
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode >= 31284 && sridCode <= 31287)
  {
    pZone->datum = udGZGD_MGI;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = 1618;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->scaleFactor = 1;

    const char *pSuffix = nullptr;

    switch (sridCode)
    {
    case 31284:
      pSuffix = "M28";
      pZone->meridian = 10.33333333333333;
      pZone->falseEasting = 150000;
      break;

    case 31285:
      pSuffix = "M31";
      pZone->meridian = 13.33333333333333;
      pZone->falseEasting = 450000;
      break;

    case 31286:
      pSuffix = "M34";
      pZone->meridian = 16.33333333333333;
      pZone->falseEasting = 750000;
      break;

    case 31287:
      pSuffix = "Lambert";
      pZone->meridian = 13.33333333333333;
      pZone->firstParallel = 49;
      pZone->secondParallel = 46;
      pZone->falseEasting = 400000;
      pZone->falseNorthing = 400000;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->parallel = 47.5;
      break;
    }

    udSprintf(pZone->zoneName, "Austria %s", pSuffix);
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode >= 31254 && sridCode <= 31259)
  {
    pZone->datum = udGZGD_MGI;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = 1618;
    pZone->parallel = 0.0;
    pZone->scaleFactor = 1;
    pZone->falseEasting = 0;
    pZone->falseNorthing = -5000000;
    pZone->meridian = 10.33333333333333;

    const char *pSuffix = nullptr;

    switch (sridCode)
    {
    case 31254:
      pSuffix = "GK West";
      break;

    case 31255:
      pZone->meridian = 13.33333333333333;
      pSuffix = "GK Central";
      break;

    case 31256:
      pZone->meridian = 16.33333333333333;
      pSuffix = "GK East";
      break;

    case 31257:
      pZone->falseEasting = 150000;
      pSuffix = "GK M28";
      break;

    case 31258:
      pZone->falseEasting = 450000;
      pZone->meridian = 13.33333333333333;
      pSuffix = "GK M31";
      break;

    case 31259:
      pZone->falseEasting = 750000;
      pZone->meridian = 16.33333333333333;
      pSuffix = "GK M34";
      break;

    }

    udSprintf(pZone->zoneName, "Austria %s", pSuffix);
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode >= 4534 && sridCode <= 4554)
  {
    const udDouble4 cgcsRegions[] = {
      { 73.62, 35.81, 76.5, 40.65 },
      { 76.5, 31.03, 79.5, 41.83 },
      { 79.5, 29.95, 82.51, 45.88 },
      { 82.5, 28.26, 85.5, 47.23 },
      { 85.5, 27.8, 88.5, 49.18 },
      { 88.49, 27.32, 91.51, 48.42 },
      { 91.5, 27.71, 94.5, 45.13 },
      { 94.5, 28.23, 97.51, 44.5 },
      { 97.5, 21.43, 100.5, 42.76 },
      { 100.5, 21.13, 103.5, 42.69 },
      { 103.5, 22.5, 106.5, 42.21 },
      { 106.5, 18.19, 109.5, 42.47 },
      { 109.5, 18.11, 112.5, 45.11 },
      { 112.5, 21.52, 115.5, 45.45 },
      { 115.5, 22.6, 118.5, 49.88 },
      { 118.5, 24.43, 121.5, 53.33 },
      { 121.5, 28.22, 124.5, 53.56 },
      { 124.5, 40.19, 127.5, 53.2 },
      { 127.5, 41.37, 130.5, 50.25 },
      { 130.5, 42.42, 133.5, 48.88 },
      { 133.5, 45.85, 134.77, 48.4 }
    };

    // China_2000
    pZone->datum = udGZGD_CGCS2000;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 4534;
    udSprintf(pZone->zoneName, "3-degree Gauss-Kruger CM %dE", 75 + (pZone->zone * 3));
    pZone->meridian = 75 + (pZone->zone * 3);
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 1;
    udGeoZone_SetSpheroid(pZone);
    pZone->latLongBoundMin = udDouble2::create(cgcsRegions[pZone->zone].x, cgcsRegions[pZone->zone].y);
    pZone->latLongBoundMax = udDouble2::create(cgcsRegions[pZone->zone].z, cgcsRegions[pZone->zone].w);
  }
  else if (sridCode >= 26901 && sridCode <= 26923)
  {
    // NAD83 Northern Hemisphere
    pZone->datum = udGZGD_NAD83;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 26900;
    udSprintf(pZone->zoneName, "UTM zone %dN", pZone->zone);
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode >= 25828 && sridCode <= 25838)
  {
    // ETRS89 / UTM zones
    pZone->datum = udGZGD_ETRS89;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 25800;
    udSprintf(pZone->zoneName, "UTM zone %dN%s", pZone->zone, ((sridCode == 25838) ? " (deprecated)" : ""));
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if (sridCode >= 28348 && sridCode <= 28356)
  {
    // GDA94 Southern Hemisphere (for MGA)
    pZone->datum = udGZGD_GDA94;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 28300;
    udSprintf(pZone->zoneName, "MGA zone %d", pZone->zone);
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if (sridCode >= 7846 && sridCode <= 7859)
  {
    // GDA2020 Southern Hemisphere (for MGA)
    pZone->datum = udGZGD_GDA2020;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 7800;
    udSprintf(pZone->zoneName, "MGA zone %d", pZone->zone);
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->parallel = 0.0;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, false);
  }
  else if ((sridCode >= 2443 && sridCode <= 2461) || (sridCode >= 6669 && sridCode <= 6687))
  {
    // {2443-2461} JGD2000 / Japan Plane Rectangular CS I through XIX
    // {6669-6687} JGD2011 / Japan Plane Rectangular CS I through XIX

    const udDouble2 jprcsRegions[] = { // Meridian, Latitude Of Origin
      { 129.5, 33.0 },
      { 131.0, 33.0 },
      { 132.0 + 1.0 / 6.0, 36.0 },
      { 133.5, 33.0 },
      { 134.0 + 1.0 / 3.0, 36.0 },
      { 136.0, 36.0 },
      { 137.0 + 1.0 / 6.0, 36.0 },
      { 138.5, 36.0 },
      { 139.0 + 5.0 / 6.0, 36.0 },
      { 140.0 + 5.0 / 6.0, 40.0 },
      { 140.25, 44.0 },
      { 142.25, 44.0 },
      { 144.25, 44.0 },
      { 142.0, 26.0 },
      { 127.5, 26.0 },
      { 124.0, 26.0 },
      { 131.0, 26.0 },
      { 136.0, 20.0 },
      { 154.0, 26.0 }
    };
    const char *pRomanNumerals[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX" };

    if (sridCode >= 2443 && sridCode <= 2461) // JDG2000
    {
      pZone->datum = udGZGD_JGD2000;
      pZone->zone = sridCode - 2443;
    }
    else if (sridCode >= 6669 && sridCode <= 6687) // JDG2011
    {
      pZone->datum = udGZGD_JGD2011;
      pZone->zone = sridCode - 6669;
    }

    pZone->projection = udGZPT_TransverseMercator;
    if (pZone->zone > (int)udLengthOf(pRomanNumerals))
      return udR_InternalError;
    udSprintf(pZone->zoneName, "Japan Plane Rectangular CS %s", pRomanNumerals[pZone->zone]);
    pZone->meridian = jprcsRegions[pZone->zone].x;
    pZone->parallel = jprcsRegions[pZone->zone].y;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 0;
    pZone->scaleFactor = 0.9999;
    udGeoZone_SetSpheroid(pZone);
    udGeoZone_SetUTMZoneBounds(pZone, true);
  }
  else if ((sridCode >= 3942 && sridCode <= 3950) || (sridCode >= 9842 && sridCode <= 9850))
  {
    // France Conic Conformal zones
    if (sridCode >= 9842)
    {
      pZone->datum = udGZGD_RGF93v2b;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->zone = sridCode - 9842;
      udSprintf(pZone->zoneName, "CC%d", sridCode - 9800);
    }
    else
    {
      pZone->datum = udGZGD_RGF93;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->zone = sridCode - 3942;
      udSprintf(pZone->zoneName, "CC%d", sridCode - 3900);
    }

    pZone->meridian = 3.0;
    pZone->parallel = 42.0 + pZone->zone;
    pZone->firstParallel = pZone->parallel - 0.75;
    pZone->secondParallel = pZone->parallel + 0.75;
    pZone->falseNorthing = 1200000 + 1000000 * pZone->zone;
    pZone->falseEasting = 1700000;
    pZone->scaleFactor = 1.0;
    udGeoZone_SetSpheroid(pZone);
    pZone->latLongBoundMin = udDouble2::create(pZone->parallel - 1.0, -2.0); // Longitude here not perfect, differs greatly in different zones
    pZone->latLongBoundMax = udDouble2::create(pZone->parallel + 1.0, 10.00);
  }
  else if (sridCode >= 5682 && sridCode <= 5685)
  {
    // DB_REF
    pZone->datum = udGZGD_DBREF;
    pZone->projection = udGZPT_TransverseMercator;
    pZone->zone = sridCode - 5680; // Starts at zone 2
    udSprintf(pZone->zoneName, "3-degree Gauss-Kruger zone %d (E-N)", pZone->zone);
    pZone->meridian = 3.0 * pZone->zone;
    pZone->parallel = 0;
    pZone->falseEasting = 500000 + 1000000 * pZone->zone;
    pZone->falseNorthing = 0;
    pZone->scaleFactor = 1.0;
    udGeoZone_SetSpheroid(pZone);
  }
  else //unordered codes
  {
    switch (sridCode)
    {
    case 84: // LongLat (Not EPSG - CRS:84. There is no EPSG LongLat code)
      pZone->datum = udGZGD_WGS84;
      pZone->projection = udGZPT_LongLat;
      pZone->zone = 0;
      pZone->scaleFactor = 0.0174532925199433;
      pZone->unitMetreScale = 1.0;
      udStrcpy(pZone->zoneName, "");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-90, -180);
      pZone->latLongBoundMax = udDouble2::create(90, 180);
      break;
    case 2154: // RGF93 / Lambert-93
      pZone->datum = udGZGD_RGF93;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      udStrcpy(pZone->zoneName, "Lambert-93");
      pZone->meridian = 3;
      pZone->parallel = 46.5;
      pZone->firstParallel = 49;
      pZone->secondParallel = 44;
      pZone->falseNorthing = 6600000;
      pZone->falseEasting = 700000;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(41.1800, -9.6200);
      pZone->latLongBoundMax = udDouble2::create(51.5400, 10.3000);
      break;
    case 9794: // RGF93 / Lambert-93 v2b
      pZone->datum = udGZGD_RGF93v2b;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      udStrcpy(pZone->zoneName, "Lambert-93");
      pZone->meridian = 3;
      pZone->parallel = 46.5;
      pZone->firstParallel = 49;
      pZone->secondParallel = 44;
      pZone->falseNorthing = 6600000;
      pZone->falseEasting = 700000;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(41.1800 , -9.6200);
      pZone->latLongBoundMax = udDouble2::create(51.5400, 10.3000);
      break;
    case 2193: // NZGD2000
      pZone->datum = udGZGD_NZGD2000;
      pZone->projection = udGZPT_TransverseMercator;
      udStrcpy(pZone->zoneName, "New Zealand Transverse Mercator 2000");
      pZone->meridian = 173;
      pZone->parallel = 0;
      pZone->falseNorthing = 10000000;
      pZone->falseEasting = 1600000;
      pZone->scaleFactor = 0.9996;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-47.4, 166.33);
      pZone->latLongBoundMax = udDouble2::create(-34, 178.6);
      break;
    case 2230: // NAD83 / California zone 6 (ftUS)
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "California zone 6 (ftUS)");
      pZone->meridian = -116.25;
      pZone->parallel = 32.0 + 1.0 / 6.0;
      pZone->firstParallel = 33.0 + 53.0 / 60.0;
      pZone->secondParallel = 32.0 + 47.0 / 60.0;
      pZone->falseNorthing = 1640416.667;
      pZone->falseEasting = 6561666.667;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(23.81, -172.54);
      pZone->latLongBoundMax = udDouble2::create(86.46, -47.74);
      break;
    case 2238: // NAD83 / Florida North (ftUS)
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Florida North (ftUS)");
      pZone->meridian = -84.5;
      pZone->parallel = 29.0;
      pZone->firstParallel = 30.75;
      pZone->secondParallel = 29.0 + 175.0 / 300.0;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1968500.0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(29.28, -87.64);
      pZone->latLongBoundMax = udDouble2::create(31.0, -82.05);
      break;
    case 2248: // NAD83 / Maryland (ftUS)
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Maryland (ftUS)");
      pZone->meridian = -77.0;
      pZone->parallel = 37.0 + 2.0 / 3.0;
      pZone->firstParallel = 39.45;
      pZone->secondParallel = 38.3;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1312333.333;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(37.88, -79.49);
      pZone->latLongBoundMax = udDouble2::create(39.72, -74.98);
      break;
    case 2250: // NAD83 / Massachusetts Island (ftUS)
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Massachusetts Island (ftUS)");
      pZone->meridian = -70.5;
      pZone->parallel = 41.0;
      pZone->firstParallel = 41.0 + 145.0 / 300.0;
      pZone->secondParallel = 41.0 + 85.0 / 300.0;;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1640416.667;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(41.2, -70.87);
      pZone->latLongBoundMax = udDouble2::create(41.51, -69.9);
      break;
    case 2285: // NAD83 / Washington North (ftUS)
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Washington North (ftUS)");
      pZone->meridian = -120 - 25.0 / 30.0;
      pZone->parallel = 47.0;
      pZone->firstParallel = 48.0 + 22.0 / 30.0;;
      pZone->secondParallel = 47.5;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1640416.667;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(47.08, -124.75);
      pZone->latLongBoundMax = udDouble2::create(49.0, -117.03);
      break;
    case 2326: // Hong Kong 1980 Grid System
      pZone->datum = udGZGD_HK1980;
      pZone->projection = udGZPT_TransverseMercator;
      udStrcpy(pZone->zoneName, "Hong Kong 1980 Grid System");
      pZone->meridian = 114.1785555555556;
      pZone->parallel = 22.31213333333334;
      pZone->falseNorthing = 819069.8;
      pZone->falseEasting = 836694.05;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(22.13, 113.76);
      pZone->latLongBoundMax = udDouble2::create(22.58, 114.51);
      break;
    case 2771: // NAD83(HARN) / California zone 6
      pZone->datum = udGZGD_NAD83_HARN;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      udStrcpy(pZone->zoneName, "California zone 6");
      pZone->meridian = -116.25;
      pZone->parallel = 32.0 + 1.0 / 6.0;
      pZone->firstParallel = 33.0 + 265.0 / 300.0;
      pZone->secondParallel = 32.0 + 235.0 / 300.0;
      pZone->falseNorthing = 500000.0;
      pZone->falseEasting = 2000000.0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(32.51, -118.14);
      pZone->latLongBoundMax = udDouble2::create(34.08, -114.43);
      break;
    case 3032: // WGS 84 / Australian Antartic Polar Sterographic
      pZone->datum = udGZGD_WGS84;//udGZGD_AUST_ANT;
      pZone->projection = udGZPT_SterographicPolar_vB;
      udStrcpy(pZone->zoneName, "Australian Antarctic Polar Stereographic");
      pZone->meridian = 70.0;
      pZone->parallel = -71.0;
      pZone->falseEasting = 6000000.0;
      pZone->falseNorthing = 6000000.0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(45.0, -90.0);
      pZone->latLongBoundMax = udDouble2::create(160.0, -60.0);
      break;
    case 3112: // GDA94 / Geoscience Australia Lambert
      pZone->datum = udGZGD_GDA94;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      udStrcpy(pZone->zoneName, "Geoscience Australia Lambert");
      pZone->meridian = 134;
      pZone->parallel = 0;
      pZone->firstParallel = -18;
      pZone->secondParallel = -36;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-60.56, 93.41);
      pZone->latLongBoundMax = udDouble2::create(-8.47, 173.35);
      break;
    case 3113: // GDA94 / BCSG02
      pZone->datum = udGZGD_GDA94;
      pZone->projection = udGZPT_TransverseMercator;
      udStrcpy(pZone->zoneName, "BCSG02");
      pZone->meridian = 153;
      pZone->parallel = -28;
      pZone->falseNorthing = 100000;
      pZone->falseEasting = 50000;
      pZone->scaleFactor = 0.99999;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-60.56, 93.41);
      pZone->latLongBoundMax = udDouble2::create(-8.47, 173.35);
      break;
    case 3139: // Vanua Levu 1915 / Cassini Soldner Hyperbolic Test
      pZone->datum = udGZGD_VANUA1915;
      pZone->projection = udGZPT_CassiniSoldnerHyperbolic;
      udStrcpy(pZone->zoneName, "Vanua Levu 1915");
      pZone->meridian = 179 + 1.0 / 3.0;
      pZone->parallel = -16.25;
      pZone->falseNorthing = 1662888.5;
      pZone->falseEasting = 1251331.8;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 0.201168; // Link Unit
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-179.5, -17.05);
      pZone->latLongBoundMax = udDouble2::create(178.25, -16.0);
      break;
    case 3174: // Great Lakes Albers / ALbers Conis Equal Area Projection Test
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_AlbersEqualArea;
      udStrcpy(pZone->zoneName, "Great Lakes Albers");
      pZone->meridian = -84.455955;
      pZone->latProjCentre = 45.568977;
      pZone->falseNorthing = 1662888.5;
      pZone->firstParallel = 42.122774;
      pZone->secondParallel = 49.01518;
      pZone->falseEasting = 1000000;
      pZone->falseNorthing = 1000000;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(40.4, -93.21);
      pZone->latLongBoundMax = udDouble2::create(50.74, -74.50);
      break;
    case 3414: // Singapore TM
      pZone->datum = udGZGD_SVY21;
      pZone->projection = udGZPT_TransverseMercator;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Singapore TM");
      pZone->meridian = 103 + (5.0 / 6.0);
      pZone->parallel = 1 + (11.0 / 30.0);
      pZone->falseNorthing = 38744.572;
      pZone->falseEasting = 28001.642;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(1.1200, 103.6200);
      pZone->latLongBoundMax = udDouble2::create(1.4600, 104.1600);
      break;
    case 3433: // Arkansas North
      pZone->datum = udGZGD_NAD83;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->zone = 0;
      pZone->parallel = 34.33333333333334; // Can't be the fraction as rounds to '4'
      pZone->firstParallel = 36.2 + (1.0/30);
      pZone->secondParallel = 34.9 + (1.0/30);
      pZone->meridian = -92;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1312333.3333;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Arkansas North (ftUS)");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(34.67,-94.62);
      pZone->latLongBoundMax = udDouble2::create(36.5,-89.64);
      break;
    case 3857: // Web Mercator
      pZone->datum = udGZGD_WGS84;
      pZone->projection = udGZPT_WebMercator;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Pseudo-Mercator");
      pZone->meridian = 0;
      pZone->parallel = 0;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-85, -180);
      pZone->latLongBoundMax = udDouble2::create(95, 180);
      break;
    case 4087: // WGS84 / World Equidistant Cylindrical
        pZone->datum = udGZGD_WGS84;
        pZone->projection = udGZPT_EquidistantCylindrical;
        pZone->zone = 0;
        udStrcpy(pZone->zoneName, "World Equidistant Cylindrical");
        pZone->meridian = 0;
        pZone->parallel = 0;
        pZone->falseNorthing = 0;
        pZone->falseEasting = 0;
        pZone->scaleFactor = 1.0;
        udGeoZone_SetSpheroid(pZone);
        pZone->latLongBoundMin = udDouble2::create(-85, -180);
        pZone->latLongBoundMax = udDouble2::create(95, 180);
        break;
    case 4326: // LatLong
      pZone->datum = udGZGD_WGS84;
      pZone->projection = udGZPT_LatLong;
      pZone->zone = 0;
      pZone->scaleFactor = 0.0174532925199433;
      pZone->unitMetreScale = 1.0;
      udStrcpy(pZone->zoneName, "");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-90, -180);
      pZone->latLongBoundMax = udDouble2::create(90, 180);
      break;
    case 4328: // ECEF (Deprecated)
      pZone->datum = udGZGD_WGS84;
      pZone->projection = udGZPT_ECEF;
      pZone->zone = 0;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 1.0;
      udStrcpy(pZone->zoneName, "");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-90, -180);
      pZone->latLongBoundMax = udDouble2::create(90, 180);
      break;
    case 4936: // ETRS89
      pZone->datum = udGZGD_ETRS89;
      pZone->projection = udGZPT_ECEF;
      pZone->zone = 0;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 1.0;
      udStrcpy(pZone->zoneName, "");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(34.5, -10.67);
      pZone->latLongBoundMax = udDouble2::create(71.05, 31.55);
      break;
    case 4978: // ECEF
      pZone->datum = udGZGD_WGS84;
      pZone->projection = udGZPT_ECEF;
      pZone->zone = 0;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 1.0;
      udStrcpy(pZone->zoneName, "");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-90, -180);
      pZone->latLongBoundMax = udDouble2::create(90, 180);
      break;
    case 6411: // Arkansas North
      pZone->datum = udGZGD_NAD83_2011;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      pZone->zone = 0;
      pZone->parallel = 34.33333333333334; // Can't be the fraction because it rounds to '4'
      pZone->firstParallel = 36.2 + (1.0 / 30.0);
      pZone->secondParallel = 34.9 + (1.0 / 30.0);
      pZone->meridian = -92;
      pZone->falseNorthing = 0.0;
      pZone->falseEasting = 1312333.3333;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 0.3048006096012192;
      udStrcpy(pZone->zoneName, "Arkansas North (ftUS)");
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(34.67,-94.62);
      pZone->latLongBoundMax = udDouble2::create(36.5,-89.64);
      break;
    case 7845: // GDA2020 / Geoscience Australia Lambert
      pZone->datum = udGZGD_GDA2020;
      pZone->projection = udGZPT_LambertConformalConic2SP;
      udStrcpy(pZone->zoneName, "GA LCC");
      pZone->meridian = 134;
      pZone->parallel = 0;
      pZone->firstParallel = -18;
      pZone->secondParallel = -36;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-43.7, 112.85);
      pZone->latLongBoundMax = udDouble2::create(-9.86, 153.69);
      break;
    case 8353: // Slovakia Krovak
      pZone->datum = udGZGD_SJTSK03;
      pZone->projection = udGZPT_KrovakNorthOrientated;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "JTSK03");
      pZone->latProjCentre = 49.5000000000003;
      pZone->coLatConeAxis = 30.2881397527781;
      pZone->meridian = 24.8333333333336;
      pZone->parallel = 78.5; //78.5000000000003
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->scaleFactor = 0.9999;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(1.13, 103.59);
      pZone->latLongBoundMax = udDouble2::create(1.47, 104.07);
      break;
    case 8705: // Mars PF
      pZone->datum = udGZGD_MARS_PCPF;
      pZone->projection = udGZPT_ECEF;
      pZone->zone = 0;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 1.0;
      udGeoZone_SetSpheroid(pZone);
      break;
    case 19920: // Singapore Grid
      pZone->datum = udGZGD_SINGGRID;
      pZone->projection = udGZPT_CassiniSoldner;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Singapore Grid");
      pZone->meridian = 103.853002222;
      pZone->parallel = 1.287646667;
      pZone->falseNorthing = 30000;
      pZone->falseEasting = 30000;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(1.13, 103.59);
      pZone->latLongBoundMax = udDouble2::create(1.47, 104.07);
      break;
    case 27700: // OSGB 1936 / British National Grid
      pZone->datum = udGZGD_OSGB36;
      pZone->projection = udGZPT_TransverseMercator;
      udStrcpy(pZone->zoneName, "British National Grid");
      pZone->meridian = -2;
      pZone->parallel = 49.0;
      pZone->falseNorthing = -100000;
      pZone->falseEasting = 400000;
      pZone->scaleFactor = 0.9996012717;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-7.5600, 49.9600);
      pZone->latLongBoundMax = udDouble2::create(1.7800, 60.8400);
      break;
    case 28992: // Amersfoort / Stereographic oblique and Equatorial Projection Test
      pZone->datum = udGZGD_AMERSFOORT;
      pZone->projection = udGZPT_SterographicObliqueNEquatorial;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Amersfoort");
      pZone->meridian = 5.3876388888888888;
      pZone->parallel = 52.156160555555556;
      pZone->falseNorthing = 463000;
      pZone->falseEasting = 155000;
      pZone->scaleFactor = 0.9999079;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(3.37,50.75);
      pZone->latLongBoundMax = udDouble2::create(7.21, 53.47);
      break;
    case 29873: // Timbalai 1948 / Hotine Oblique Mercator Variant B Projection Test
      pZone->datum = udGZGD_TIMB1948;
      pZone->projection = udGZPT_HotineObliqueMercatorvB;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Timbalai 1948");
      pZone->meridian = 115.0;
      pZone->parallel = 53.13010236111111;
      pZone->coLatConeAxis = 53.31582047222222;
      pZone->latProjCentre = 4.0;
      pZone->falseNorthing = 442857.65;
      pZone->falseEasting = 590476.87;
      pZone->scaleFactor = 0.99984;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(0.85, 109.55);
      pZone->latLongBoundMax = udDouble2::create(7.35, 119.26);
      break;
    //case 30100:
    case 30101: //Moon PF
      pZone->datum = udGZGD_MOON_PCPF;
      pZone->projection = udGZPT_ECEF;
      pZone->zone = 30101;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->meridian = 0;
      pZone->scaleFactor = 1;
      pZone->parallel = 0;
      udGeoZone_SetSpheroid(pZone);
      break;
    //case 30174:
    case 30175: //Moon M
      pZone->datum = udGZGD_MOON_MERC;
      pZone->projection = udGZPT_Mercator;
      udStrcpy(pZone->zoneName, "Moon 2000 Mercator");
      pZone->zone = 30175;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->meridian = 0;
      pZone->firstParallel = 0;
      pZone->scaleFactor = 1.0;
      udGeoZone_SetSpheroid(pZone);
      break;
    case 30200: // Trinidad 1903 / Cassini Soldner Projection Test
      pZone->datum = udGZGD_TRI1903;
      pZone->projection = udGZPT_CassiniSoldner;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Trinidad 1903");
      pZone->meridian = -61.0 - (1.0 / 3.0);
      pZone->parallel = 10.441 + (2.0 / (3.0 * 1000.0));  
      pZone->falseNorthing = 325000;
      pZone->falseEasting = 430000;
      pZone->scaleFactor = 1.0;
      pZone->unitMetreScale = 0.201166195164; // Clarke's link Unit
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(-62.08, 9.82);
      pZone->latLongBoundMax = udDouble2::create(-58.53, 11.68);
      break;
    case 31700: // Dealul Piscului 1970/ Stereo 70
      pZone->datum = udGZGD_DEALUL1970;
      pZone->projection = udGZPT_SterographicObliqueNEquatorial;
      pZone->zone = 0;
      udStrcpy(pZone->zoneName, "Dealul Piscului 1970");
      pZone->meridian = 25.0;
      pZone->parallel = 46.0;
      pZone->falseNorthing = 500000;
      pZone->falseEasting = 500000;
      pZone->scaleFactor = 0.99975;
      udGeoZone_SetSpheroid(pZone);
      pZone->latLongBoundMin = udDouble2::create(43.62, 20.26);
      pZone->latLongBoundMax = udDouble2::create(48.26, 31.5);
      break;
    //case 49974:
    case 49975: //Mars M
      pZone->datum = udGZGD_MARS_MERC;
      pZone->projection = udGZPT_Mercator;
      udStrcpy(pZone->zoneName, "Mars 2000 Mercator");
      pZone->zone = 49975;
      pZone->falseNorthing = 0;
      pZone->falseEasting = 0;
      pZone->meridian = 0;
      pZone->scaleFactor = 1.0;
      pZone->firstParallel = 0;
      udGeoZone_SetSpheroid(pZone);
      break;

    default:
    {
      udScopeLock scopeLock(g_pMutex);

      int index = udGeoZone_InternalIndexOf(sridCode);
      if (index >= 0)
        *pZone = g_InternalGeoZoneList[index];
      else
        return udR_NotFound;
    }
    }
  }

  pZone->srid = sridCode;
  pZone->knownDatum = true;

  if (sridCode != 0)
  {
    udStrcpy(pZone->datumName, g_udGZ_GeodeticDatumDescriptors[pZone->datum].pDatumName);
    udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[pZone->datum].pShortName);
    udGeoZone_UpdateDisplayName(pZone);
  }

  return udR_Success;
}

udResult udGeoZone_UpdateSphereoidInfo(udGeoZone *pZone)
{
  if (pZone == nullptr)
    return udR_InvalidParameter;

  if (pZone->datum >= 0 && pZone->datum < udGZGD_Count)
  {
    udGeoZone_SetSpheroid(pZone);

    udStrcpy(pZone->datumName, g_udGZ_GeodeticDatumDescriptors[pZone->datum].pDatumName);
    udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[pZone->datum].pShortName);
    udGeoZone_UpdateDisplayName(pZone);

    return udR_Success;
  }

  return udR_Failure;
}

// ----------------------------------------------------------------------------
// Author: Jon Kable, February 2019
// Once unitMetreScale, semiMajorAxis and flattening have all been identified, this helper function can be called to perform all remaining calculations
static void udGeoZone_MetreScaleSpheroidMaths(udGeoZone *pZone)
{
  pZone->semiMajorAxis /= pZone->unitMetreScale;
  double a = pZone->semiMajorAxis;
  double f = pZone->flattening;
  double b = a * (1 - f); // b=a*(1-f)
  pZone->semiMinorAxis = b; // in feet or metres
  pZone->eccentricity = udSqrt(a*a - b*b) / a; // e=sqrt(a^2-b^2)/a
  pZone->eccentricitySq = udPow(pZone->eccentricity, 2);
  pZone->thirdFlattening = (a - b) / (a + b); // tf=(a-b)/(a+b)
  udGeoZone_SetSpheroid(pZone);
}

// ----------------------------------------------------------------------------
// Author: Jon Kable, February 2019
// Helper function for parsing through WellKnownText data
static void udGeoZone_JSONTreeSearch(udGeoZone *pZone, udJSON *wkt, const char *pStr)
{
  size_t n = wkt->Get("%s", pStr).ArrayLength();
  for (size_t i = 0; i < n; ++i)
  {
    // Data Extraction
    const char *pElem = nullptr;
    const char *pVal = nullptr;
    udSprintf(&pElem, "%s[%d]", pStr, (int)i);
    const char *pType = wkt->Get("%s.type", pElem).AsString();
    const char *pName = wkt->Get("%s.name", pElem).AsString();

    if (pType == nullptr)
    {
      // nothing to check
    }
    else if (udStrEqual(pType, "PARAMETER"))
    {
      if (udStrEqual(pName, "false_easting"))
        pZone->falseEasting = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "false_northing"))
        pZone->falseNorthing = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "scale_factor"))
        pZone->scaleFactor = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "central_meridian"))
        pZone->meridian = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "latitude_of_origin")) // aka parallel of origin
        pZone->parallel = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "standard_parallel_1"))
        pZone->firstParallel = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "standard_parallel_2"))
        pZone->secondParallel = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "colatitude_cone_axis"))
        pZone->coLatConeAxis = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "latitude_projection_centre"))
        pZone->latProjCentre = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "latitude_of_center"))
        pZone->latProjCentre = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "longitude_of_center"))
        pZone->meridian = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "azimuth"))
        pZone->coLatConeAxis = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if (udStrEqual(pName, "rectified_grid_angle"))
        pZone->parallel = wkt->Get("%s.values[0]", pElem).AsDouble();
      else if constexpr (UD_DEBUG)
        udDebugPrintf("Unknown PARAMETER: %s\n", pName);
    }
    else if (udStrEqual(pType, "UNIT"))
    {
      if (pZone->unitMetreScale == 0 && (udStrstr(pName, 0, "foot") || udStrstr(pName, 0, "feet") || udStrstr(pName, 0, "ft") || udStrstr(pName, 0, "metre") || udStrstr(pName, 0, "link") || udStrstr(pName, 0, "Clarke's link")))
      {
        pZone->unitMetreScale = wkt->Get("%s.values[0]", pElem).AsDouble();
        if (pZone->semiMajorAxis != 0)
        {
          udGeoZone_MetreScaleSpheroidMaths(pZone);
        }
      }
    }
    else if (udStrEqual(pType, "PROJCS"))
    {
      if (pZone->projection == udGZPT_Unknown)
        pZone->projection = udGZPT_TransverseMercator; // Most likely- This will be overriden later

      size_t pIndex = 0;
      const char *pNameStr = udStrchr(pName, "/", &pIndex); // sometimes the PROJCS name is listed in WKT as: 'shortname / longname'...
      if (pNameStr != nullptr)
      {
        pNameStr += 2; // ...if so, ignore the '/' and the space following it, focus only on 'longname'
        udStrcpy(pZone->zoneName, pNameStr);
      }
      else
      {
        udStrcpy(pZone->zoneName, pName);
      }

      udSprintf(&pVal, "%s.values", pElem);
      size_t numValues = wkt->Get("%s", pVal).ArrayLength();
      for (size_t j = 0; j < numValues; ++j)
      {
        if (udStrEqual(wkt->Get("%s[%d].type", pVal, (int)j).AsString(), "AUTHORITY"))
        {
          pZone->srid = wkt->Get("%s[%d].values[0]", pVal, (int)j).AsInt(); // the authority tag directly under PROJCS is the srid
          break;
        }
      }
    }
    else if (udStrEqual(pType, "GEOGCS"))
    {
      if (pZone->projection == udGZPT_Unknown)
      {
        pZone->projection = udGZPT_LatLong; // Most likely situation
        pZone->unitMetreScale = 1.0;

        udSprintf(&pVal, "%s.values", pElem);
        size_t numValues = wkt->Get("%s", pVal).ArrayLength();
        for (size_t j = 0; j < numValues; ++j)
        {
          const char *pItemName = wkt->Get("%s[%d].type", pVal, (int)j).AsString();

          if (udStrEqual(pItemName, "UNIT"))
            pZone->scaleFactor = wkt->Get("%s[%d].values[0]", pVal, (int)j).AsDouble();
          else if (udStrEqual(pItemName, "AUTHORITY"))
            pZone->srid = wkt->Get("%s[%d].values[0]", pVal, (int)j).AsInt();
          else if (udStrEqual(pItemName, "AXIS") && udStrEqual(wkt->Get("%s[%d].name", pVal, (int)j).AsString(), "Lat") && udStrEqual(wkt->Get("%s[%d].values[0]", pVal, (int)j).AsString(), "Y"))
            pZone->projection = udGZPT_LongLat;
        }
      }

      int j = 0;
      for (j = 0; j < udGZGD_Count; ++j)
      {
        if (udStrEqual(g_udGZ_GeodeticDatumDescriptors[j].pFullName, pName))
        {
          pZone->knownDatum = true;
          pZone->datum = (udGeoZoneGeodeticDatum)j; // enum ordering corresponds to GDD dataset ordering
          udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[j].pShortName);
          break;
        }
      }

      if (j == udGZGD_Count)
      {
        for (size_t aliasIndex = 0; aliasIndex < udLengthOf(g_udGZ_DatumAlias); ++aliasIndex)
        {
          if (udStrEqual(g_udGZ_DatumAlias[aliasIndex].pAlias, pName))
          {
            pZone->knownDatum = true;
            pZone->datum = (udGeoZoneGeodeticDatum)g_udGZ_DatumAlias[aliasIndex].datumIndex; // enum ordering corresponds to GDD dataset ordering
            udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[g_udGZ_DatumAlias[aliasIndex].datumIndex].pShortName);
            j = g_udGZ_DatumAlias[aliasIndex].datumIndex;
            break;
          }
        }
      }

      if constexpr (UD_DEBUG)
      {
        if (j == udGZGD_Count)
        {
          udStrcpy(pZone->datumShortName, pName);
          udDebugPrintf("Unknown Datum: %s\n", pName);
        }
      }
    }
    else if (udStrEqual(pType, "GEOCCS"))
    {
      if (pZone->projection == udGZPT_Unknown)
      {
        pZone->projection = udGZPT_ECEF;
        pZone->scaleFactor = 1.0;

        udSprintf(&pVal, "%s.values", pElem);
        size_t numValues = wkt->Get("%s", pVal).ArrayLength();
        for (size_t j = 0; j < numValues; ++j)
        {
          const char *pItemName = wkt->Get("%s[%d].type", pVal, (int)j).AsString();

          if (udStrEqual(pItemName, "UNIT"))
            pZone->unitMetreScale = wkt->Get("%s[%d].values[0]", pVal, (int)j).AsDouble();
          else if (udStrEqual(pItemName, "AUTHORITY"))
            pZone->srid = wkt->Get("%s[%d].values[0]", pVal, (int)j).AsInt();
        }
      }

      int j = 0;
      for (j = 0; j < udGZGD_Count; ++j)
      {
        if (udStrEqual(g_udGZ_GeodeticDatumDescriptors[j].pFullName, pName))
        {
          pZone->knownDatum = true;
          pZone->datum = (udGeoZoneGeodeticDatum)j; // enum ordering corresponds to GDD dataset ordering
          udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[j].pShortName);
          break;
        }
      }

      if (j == udGZGD_Count)
      {
        for (size_t aliasIndex = 0; aliasIndex < udLengthOf(g_udGZ_DatumAlias); ++aliasIndex)
        {
          if (udStrEqual(g_udGZ_DatumAlias[aliasIndex].pAlias, pName))
          {
            pZone->knownDatum = true;
            pZone->datum = (udGeoZoneGeodeticDatum)g_udGZ_DatumAlias[aliasIndex].datumIndex; // enum ordering corresponds to GDD dataset ordering
            udStrcpy(pZone->datumShortName, g_udGZ_GeodeticDatumDescriptors[g_udGZ_DatumAlias[aliasIndex].datumIndex].pShortName);
            j = g_udGZ_DatumAlias[aliasIndex].datumIndex;
            break;
          }
        }
      }
      if (j == udGZGD_Count)
      {
        udStrcpy(pZone->datumShortName, pName);
        if constexpr (UD_DEBUG)
          udDebugPrintf("Unknown Datum: %s\n", pName);
      }
    }
    else if (udStrEqual(pType, "DATUM"))
    {
      udStrcpy(pZone->datumName, pName);
    }
    else if (udStrEqual(pType, "PROJECTION"))
    {
      if (udStrstr(pName, 0, "Mercator_1SP"))
      {
        pZone->projection = udGZPT_WebMercator;
        if (pZone->scaleFactor == 0) // default for WebMerc is 1.0
          pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Transverse_Mercator"))
      {
        pZone->projection = udGZPT_TransverseMercator;
        if (pZone->scaleFactor == 0) // default for TM is 0.9996
          pZone->scaleFactor = 0.9996;
      }
      else if (udStrstr(pName, 0, "Lambert"))
      {
        pZone->projection = udGZPT_LambertConformalConic2SP;
        if (pZone->scaleFactor == 0) // default for lambert is 1.0
          pZone->scaleFactor = 1;
      }
      else if (udStrstr(pName, 0, "Hyperbolic_Cassini_Soldner"))
      {
        pZone->projection = udGZPT_CassiniSoldnerHyperbolic;
        if (pZone->scaleFactor == 0) // default for lambert is 1.0
          pZone->scaleFactor = 1;
      }
      else if (udStrstr(pName, 0, "Cassini_Soldner"))
      {
        pZone->projection = udGZPT_CassiniSoldner;
        if (pZone->scaleFactor == 0) // default for lambert is 1.0
          pZone->scaleFactor = 1;
      }
      else if (udStrstr(pName, 0, "Oblique_Stereographic"))
      {
        pZone->projection = udGZPT_SterographicObliqueNEquatorial;
        if (pZone->scaleFactor == 0) // default for lambert is 1.0
          pZone->scaleFactor = 1;
      }
      else if (udStrstr(pName, 0, "Polar_Stereographic"))
      {
        pZone->projection = udGZPT_SterographicPolar_vB;
          if (pZone->scaleFactor == 0) // default for Polar Stereo is 1.0
            pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Krovak (North Orientated)"))
      {
        pZone->projection = udGZPT_KrovakNorthOrientated;
        if (pZone->scaleFactor == 0) // default for Krovak
          pZone->scaleFactor = 0.9999;
      }
      else if (udStrstr(pName, 0, "Krovak"))
      {
        pZone->projection = udGZPT_Krovak;
        if (pZone->scaleFactor == 0) // default for Krovak
          pZone->scaleFactor = 0.999;
      }
      else if (udStrstr(pName, 0, "Hotine_Oblique_Mercator_Azimuth_Center"))
      {
        pZone->projection = udGZPT_HotineObliqueMercatorvB;
        if (pZone->scaleFactor == 0) // default for Hotine Oblique Mercator
          pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Hotine_Oblique_Mercator"))
      {
        pZone->projection = udGZPT_HotineObliqueMercatorvA;
        if (pZone->scaleFactor == 0) // default for Hotine Oblique Mercator
          pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Mercator"))
      {
        pZone->projection = udGZPT_Mercator;
        if (pZone->scaleFactor == 0) // default for Mercator is 1.0
          pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Albers_Conic_Equal_Area"))
      {
        pZone->projection = udGZPT_AlbersEqualArea;
        if (pZone->scaleFactor == 0)
          pZone->scaleFactor = 1.0;
      }
      else if (udStrstr(pName, 0, "Equirectangular"))
      {
        pZone->projection = udGZPT_EquidistantCylindrical;
        if (pZone->scaleFactor == 0) // default for EquiCylindrical is 1.0
          pZone->scaleFactor = 1.0;
      }
      else if constexpr (UD_DEBUG)
      {
        udDebugPrintf("Unsupported Projection: %s\n", pName);
      }
    }
    else if (udStrEqual(pType, "SPHEROID"))
    {
      // is this even used ?
      //-----
      pZone->semiMajorAxis = wkt->Get("%s.values[0]", pElem).AsDouble(); // in feet or metres
      double f = wkt->Get("%s.values[1]", pElem).AsDouble();
      pZone->flattening = f == 0.0 ? 0.0 : 1.0 / wkt->Get("%s.values[1]", pElem).AsDouble(); // inverse flattening
      if (pZone->unitMetreScale != 0)
        udGeoZone_MetreScaleSpheroidMaths(pZone);
      //-----

      int32_t epsg = wkt->Get("%s.values[2].values[0]", pElem).AsInt();
      pZone->datumSrid = epsg;
      for (int epsgIndex = 0; epsgIndex < udGZE_Count; ++epsgIndex)
      {
        if (epsg == g_udGZ_StdEllipsoids[epsgIndex].authorityEpsg)
          pZone->zoneSpheroid = (udGeoZoneEllipsoid)epsgIndex;
      }
    }
    else if (udStrEqual(pType, "TOWGS84") && !pZone->knownDatum)
    {
      pZone->paramsHelmert7[0] = wkt->Get("%s.values[0]", pElem).AsDouble();
      pZone->paramsHelmert7[1] = wkt->Get("%s.values[1]", pElem).AsDouble();
      pZone->paramsHelmert7[2] = wkt->Get("%s.values[2]", pElem).AsDouble();
      pZone->paramsHelmert7[3] = wkt->Get("%s.values[3]", pElem).AsDouble();
      pZone->paramsHelmert7[4] = wkt->Get("%s.values[4]", pElem).AsDouble();
      pZone->paramsHelmert7[5] = wkt->Get("%s.values[5]", pElem).AsDouble();
      pZone->paramsHelmert7[6] = wkt->Get("%s.values[6]", pElem).AsDouble();

      pZone->toWGS84 = true;
    }
    else if (udStrEqual(pType, "AXIS"))
    {
      pZone->axisInfo = true;
    }

    // Recursive Iteration (or Iterative Recursion)
    udSprintf(&pVal, "%s.values", pElem);
    udFree(pElem);
    if (wkt->Get("%s", pVal).ArrayLength() > 0)
      udGeoZone_JSONTreeSearch(pZone, wkt, pVal);
    udFree(pVal);
  }
}

// ----------------------------------------------------------------------------
// Author: Jon Kable, February 2019
udResult udGeoZone_SetFromWKT(udGeoZone *pZone, const char *pWKT)
{
  if (pZone == nullptr || pWKT == nullptr)
    return udR_InvalidParameter;
  else
    memset(pZone, 0, sizeof(udGeoZone));

  udJSON wkt;
  udParseWKT(&wkt, pWKT);

  pZone->zoneSpheroid = udGZE_Count;
  // recursive helper function
  udGeoZone_JSONTreeSearch(pZone, &wkt, "values");

  // if unknown Datum and known spheroid -> fill datumDescriptor table with parameters read from the JSON
  if (!pZone->knownDatum && pZone->zoneSpheroid != udGZE_Count)
  {
    //                                                             Full Name,        Short  name,           Datum name,       Ellipsoid index,     ToWGS84 parameters,                                                                                                                                                                      epsg,        auth,             AxisInfo,        ToWGS84  
    g_InternalDatumList.PushBack(udGeoZoneGeodeticDatumDescriptor{ pZone->datumName, pZone->datumShortName, pZone->datumName, pZone->zoneSpheroid, {pZone->paramsHelmert7[0], pZone->paramsHelmert7[1], pZone->paramsHelmert7[2], pZone->paramsHelmert7[3], pZone->paramsHelmert7[4], pZone->paramsHelmert7[5], pZone->paramsHelmert7[6]}, pZone->srid, pZone->datumSrid, pZone->axisInfo, pZone->toWGS84 });
    pZone->datum = (udGeoZoneGeodeticDatum) ((int)udGZGD_Count + g_InternalDatumList.length);
  }

  //reset zone params used above
  bool supportedDatum = pZone->zoneSpheroid != udGZE_Count;
  pZone->zoneSpheroid = (udGeoZoneEllipsoid)0;
  pZone->datumSrid = 0;
  pZone->axisInfo = 0;

  udGeoZone_UpdateDisplayName(pZone);

  if (pZone->scaleFactor != 0 && !udStrEqual(pZone->datumShortName, "") && pZone->semiMajorAxis != 0 && pZone->srid != 0 && supportedDatum) // ensure some key variables are not null
    return udR_Success;
  return udR_Failure;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, September 2018
udResult udGeoZone_GetWellKnownText(const char **ppWKT, const udGeoZone &zone)
{
  udResult result;
  const udGeoZoneGeodeticDatumDescriptor *pDesc = nullptr;
  const udGeoZoneEllipsoidInfo *pEllipsoid = nullptr;
  const char *pWKTSpheroid = nullptr;
  const char *pWKTToWGS84 = nullptr;
  const char *pWKTDatum = nullptr;
  const char *pWKTGeoGCS = nullptr;
  const char *pWKTUnit = nullptr;
  const char *pWKTProjection = nullptr;
  const char *pWKT = nullptr;

  // Different datums require different precision (if more zones are found to require these precision changes, these should be moved to the datum table)
  int falseOriginPrecision = ((zone.datum == udGZGD_NAD83 || zone.datum == udGZGD_NAD83_2011) ? 4 : 3);
  int parallelPrecision = ((zone.datum == udGZGD_SVY21) ? 15 : 14);
  int meridianPrecision = ((zone.datum == udGZGD_MGI) ? 14 : 13);
  int scalePrecision = 10;

  UD_ERROR_NULL(ppWKT, udR_InvalidParameter);
  UD_ERROR_IF(zone.srid == 0, udR_InvalidParameter);

  if (zone.datum < udGZGD_Count)
    pDesc = &g_udGZ_GeodeticDatumDescriptors[zone.datum];
  else
    pDesc = &g_InternalDatumList[zone.datum - udGZGD_Count - 1];

  pEllipsoid = &g_udGZ_StdEllipsoids[pDesc->ellipsoid];
  // If the ellipsoid isn't WGS84, then provide parameters to get to WGS84
  if (pDesc->exportToWGS84)
  {
    int decimalPlaces = zone.datum == udGZGD_HK1980 ? 7 : zone.datum == udGZGD_MGI ? 4 : 3;
    udSprintf(&pWKTToWGS84, ",\nTOWGS84[%s,%s,%s,%s,%s,%s,%s]",
      udTempStr_TrimDouble(pDesc->paramsHelmert7[0], 3),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[1], 3),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[2], 3),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[3], decimalPlaces),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[4], decimalPlaces),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[5], decimalPlaces),
      udTempStr_TrimDouble(pDesc->paramsHelmert7[6], decimalPlaces));
  }

  udSprintf(&pWKTSpheroid, "SPHEROID[\"%s\",%s,%s,\nAUTHORITY[\"EPSG\",\"%d\"]]", pEllipsoid->pName, udTempStr_TrimDouble(pEllipsoid->semiMajorAxis, 8), pEllipsoid->flattening == 0.0 ? "0.0" : udTempStr_TrimDouble(1.0 / pEllipsoid->flattening, 9), pEllipsoid->authorityEpsg);
  udSprintf(&pWKTDatum, "DATUM[\"%s\",\n%s%s,\nAUTHORITY[\"EPSG\",\"%d\"]", pDesc->pDatumName, pWKTSpheroid, pWKTToWGS84 ? pWKTToWGS84 : "", pDesc->authority);

  if (zone.projection == udGZPT_ECEF && zone.datum == udGZGD_MARS_PCPF) // Mars
    udSprintf(&pWKTGeoGCS, "GEOCCS[\"%s\",\n%s],\nPRIMEM[\"AIRY-0\",0],\nUNIT[\"metre\",1,\nAUTHORITY[\"EPSG\",\"9001\"]]", pDesc->pFullName, pWKTDatum);
  else if (zone.projection == udGZPT_ECEF)
    udSprintf(&pWKTGeoGCS, "GEOCCS[\"%s\",\n%s],\nPRIMEM[\"Greenwich\",0,\nAUTHORITY[\"EPSG\",\"8901\"]],\nUNIT[\"metre\",1,\nAUTHORITY[\"EPSG\",\"9001\"]]", pDesc->pFullName, pWKTDatum);
  else if (zone.projection == udGZPT_LongLat) // This isn't an official option- ISO6709 doesn't allow it so we handle it specially
    udSprintf(&pWKTGeoGCS, "GEOGCS[\"%s\",\n%s],\nPRIMEM[\"Greenwich\",0,\nAUTHORITY[\"EPSG\",\"8901\"]],\nUNIT[\"degree\",0.0174532925199433,\nAUTHORITY[\"EPSG\",\"9122\"]],\nAXIS[\"Lon\",X],\nAXIS[\"Lat\",Y],\nAUTHORITY[\"CRS\",\"%d\"]]", pDesc->pFullName, pWKTDatum, zone.srid);
  else
    udSprintf(&pWKTGeoGCS, "GEOGCS[\"%s\",\n%s],\nPRIMEM[\"Greenwich\",0,\nAUTHORITY[\"EPSG\",\"8901\"]],\nUNIT[\"degree\",0.0174532925199433,\nAUTHORITY[\"EPSG\",\"9122\"]],\nAUTHORITY[\"EPSG\",\"%d\"]]", pDesc->pFullName, pWKTDatum, pDesc->epsg);

  // We only handle degree, metres, us feet, Clarke's link and link , each of which have their own fixed authority code
  if (zone.scaleFactor == 0.0174532925199433)
    udSprintf(&pWKTUnit, "UNIT[\"degree\",0.0174532925199433,\nAUTHORITY[\"EPSG\",\"9122\"]]");
  else if (zone.unitMetreScale == 1.0)
    udSprintf(&pWKTUnit, "UNIT[\"metre\",1,\nAUTHORITY[\"EPSG\",\"9001\"]]");
  else if (zone.unitMetreScale == 0.3048006096012192)
    udSprintf(&pWKTUnit, "UNIT[\"US survey foot\",0.3048006096012192,\nAUTHORITY[\"EPSG\",\"9003\"]]");
  else if (zone.unitMetreScale == 0.201166195164)
    udSprintf(&pWKTUnit, "UNIT[\"Clarke's link\",0.201166195164,\nAUTHORITY[\"EPSG\",\"9039\"]]");
  else if (zone.unitMetreScale == 0.201168)
    udSprintf(&pWKTUnit, "UNIT[\"link\",0.201168,\nAUTHORITY[\"EPSG\",\"9098\"]]");
  else
    udSprintf(&pWKTUnit, "UNIT[\"unknown\",%s]", udTempStr_TrimDouble(zone.unitMetreScale, 16)); // Can't provide authority for unknown unit

  if (zone.projection == udGZPT_TransverseMercator)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Transverse_Mercator\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],"
                               "\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
                               udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
                               udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_LambertConformalConic2SP)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Lambert_Conformal_Conic_2SP\"],\nPARAMETER[\"standard_parallel_1\",%s],\nPARAMETER[\"standard_parallel_2\",%s],"
                               "\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
                                udTempStr_TrimDouble(zone.firstParallel, parallelPrecision, 0, true), udTempStr_TrimDouble(zone.secondParallel, parallelPrecision), udTempStr_TrimDouble(zone.parallel, parallelPrecision, 0, true), udTempStr_TrimDouble(zone.meridian, meridianPrecision),
                                udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_WebMercator)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Mercator_1SP\"],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
                                udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
                                udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_CassiniSoldner)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Cassini_Soldner\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_CassiniSoldnerHyperbolic)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Hyperbolic_Cassini_Soldner\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_SterographicObliqueNEquatorial)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Oblique_Stereographic\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_Mercator)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Mercator\"],\nPARAMETER[\"False_Easting\",%s],\nPARAMETER[\"False_Northing\",%s],\nPARAMETER[\"Central_Meridian\",%s],\nPARAMETER[\"Standard_Parallel_1\",%s],\n%s",
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision),
      udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.firstParallel, parallelPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_SterographicPolar_vB)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Polar_Stereographic\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_Krovak)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Krovak\"],\nPARAMETER[\"latitude_projection_centre\",%s],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"colatitude_cone_axis\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.latProjCentre, parallelPrecision), udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.coLatConeAxis, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_KrovakNorthOrientated)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Krovak (North Orientated)\"],\nPARAMETER[\"latitude_projection_centre\",%s],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"colatitude_cone_axis\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.latProjCentre, parallelPrecision), udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.coLatConeAxis, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_HotineObliqueMercatorvA)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Hotine_Oblique_Mercator\"],\nPARAMETER[\"latitude_of_center\",%s],\nPARAMETER[\"longitude_of_center\",%s],\nPARAMETER[\"azimuth\",%s],\nPARAMETER[\"rectified_grid_angle\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.latProjCentre, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.coLatConeAxis, parallelPrecision), udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_HotineObliqueMercatorvB)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Hotine_Oblique_Mercator_Azimuth_Center\"],\nPARAMETER[\"latitude_of_center\",%s],\nPARAMETER[\"longitude_of_center\",%s],\nPARAMETER[\"azimuth\",%s],\nPARAMETER[\"rectified_grid_angle\",%s],\nPARAMETER[\"scale_factor\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.latProjCentre, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.coLatConeAxis, parallelPrecision), udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.scaleFactor, scalePrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_AlbersEqualArea)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Albers_Conic_Equal_Area\"],\nPARAMETER[\"latitude_of_center\",%s],\nPARAMETER[\"longitude_of_center\",%s],\nPARAMETER[\"standard_parallel_1\",%s],\nPARAMETER[\"standard_parallel_2\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.latProjCentre, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision), udTempStr_TrimDouble(zone.firstParallel, parallelPrecision), udTempStr_TrimDouble(zone.secondParallel, parallelPrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }
  else if (zone.projection == udGZPT_EquidistantCylindrical)
  {
    udSprintf(&pWKTProjection, "PROJECTION[\"Equirectangular\"],\nPARAMETER[\"latitude_of_origin\",%s],\nPARAMETER[\"central_meridian\",%s],\nPARAMETER[\"false_easting\",%s],\nPARAMETER[\"false_northing\",%s],\n%s",
      udTempStr_TrimDouble(zone.parallel, parallelPrecision), udTempStr_TrimDouble(zone.meridian, meridianPrecision),
      udTempStr_TrimDouble(zone.falseEasting, falseOriginPrecision), udTempStr_TrimDouble(zone.falseNorthing, falseOriginPrecision), pWKTUnit);
  }

  // JGD2000, JGD2011 and CGCS2000 doesn't provide axis information
  if (pDesc->exportAxisInfo)
  {
    // Generally transverse mercator projections have one style, lambert another, except for GDA LCC (3112 and 7845)
    if (zone.projection == udGZPT_TransverseMercator || zone.srid == 3112 || zone.srid == 7845)
      udSprintf(&pWKTProjection, "%s,\nAXIS[\"Easting\",EAST],\nAXIS[\"Northing\",NORTH]", pWKTProjection);
    else if (zone.projection == udGZPT_ECEF)
      udSprintf(&pWKTProjection, "AXIS[\"Geocentric X\",OTHER],\nAXIS[\"Geocentric Y\",OTHER],\nAXIS[\"Geocentric Z\",NORTH]");
    else if (zone.projection == udGZPT_LambertConformalConic2SP || zone.projection == udGZPT_WebMercator || zone.projection == udGZPT_CassiniSoldner || zone.projection == udGZPT_SterographicObliqueNEquatorial)
      udSprintf(&pWKTProjection, "%s,\nAXIS[\"X\",EAST],\nAXIS[\"Y\",NORTH]", pWKTProjection);
    else if (zone.projection == udGZPT_Krovak)
      udSprintf(&pWKTProjection, "%s,\nAXIS[\"latitude(Lat)\",north],AXIS[\"longitude(Lon)\",east]", pWKTProjection);
    else if (zone.projection == udGZPT_KrovakNorthOrientated)
      udSprintf(&pWKTProjection, "%s,\nAXIS[\"Easting(X)\",east],AXIS[\"Northing(Y)\",north]", pWKTProjection);
  }

  // Hong Kong doesn't seem to have actual zones, so we detect by testing if short name and zone name are the same
  if (zone.projection == udGZPT_ECEF)
    udSprintf(&pWKT, "%s,\n%s,\nAUTHORITY[\"EPSG\",\"%d\"]]", pWKTGeoGCS, pWKTProjection, zone.srid);
  else if (zone.projection == udGZPT_LatLong || zone.projection == udGZPT_LongLat)
    udSprintf(&pWKT, "%s", pWKTGeoGCS);
  else if (udStrBeginsWith(zone.zoneName, pDesc->pShortName))
    udSprintf(&pWKT, "PROJCS[\"%s\",\n%s,\n%s,\nAUTHORITY[\"EPSG\",\"%d\"]]", zone.zoneName, pWKTGeoGCS, pWKTProjection, zone.srid);
  else
    udSprintf(&pWKT, "PROJCS[\"%s / %s\",\n%s,\n%s,\nAUTHORITY[\"EPSG\",\"%d\"]]", pDesc->pShortName, zone.zoneName, pWKTGeoGCS, pWKTProjection, zone.srid);

  *ppWKT = pWKT;
  pWKT = nullptr;
  result = udR_Success;

epilogue:
  udFree(pWKTSpheroid);
  udFree(pWKTToWGS84);
  udFree(pWKTDatum);
  udFree(pWKTGeoGCS);
  udFree(pWKTUnit);
  udFree(pWKTProjection);
  udFree(pWKT);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
// Root finding with Newton's method to calculate conformal latitude.
static double udGeoZone_LCCLatConverge(double t, double td, double e)
{
  double s = udSinh(e * udATanh(e * t / udSqrt(1 + t * t)));
  double fn = t * udSqrt(1 + s * s) - s * udSqrt(1 + t * t) - td;
  double fd = (udSqrt(1 + s * s) * udSqrt(1 + t * t) - s * t) * ((1 - e * e) * udSqrt(1 + t * t)) / (1 + (1 - e * e) * t * t);
  return fn / fd;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double udGeoZone_LCCMeridonial(double phi, double e)
 {
   double d = e * udSin(phi);
   return udCos(phi) / udSqrt(1 - d * d);
 }

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static double udGeoZone_LCCConformal(double phi, double e)
 {
   double d = e * udSin(phi);
   return udTan(UD_PI / 4.0 - phi / 2.0) / udPow((1 - d) / (1 + d), e / 2.0);
 }

// ----------------------------------------------------------------------------
// Author: Jules Proust, September 2020
static double udGeoZone_MeridianArcDistance(const double phi, const double* n)
{
  // Helmert's series
  // Maxima:
  // trigreduce( defint(taylor((1-n^2)^2/(2 * n * cos(2*t) + n^2 + 1)^(3/2), n, 0,9),t,0,phi) );
  return (phi * (82575360.0 + 185794560.0 * n[2] + 290304000.0 * n[4] + 395136000.0 * n[6] + 500094000.0 * n[8])
    + udSin(2.0 * phi) * (-123863040.0 * n[1] - 232243200.0 * n[3] - 338688000.0 * n[5] - 444528000.0 * n[7] - 550103400.0 * n[9])
    + udSin(4.0 * phi) * (77414400.0 * n[2] + 135475200.0 * n[4] + 190512000.0 * n[6] + 244490400.0 * n[8])
    + udSin(6.0 * phi) * (-60211200.0 * n[3] - 101606400.0 * n[5] - 139708800.0 * n[7] - 176576400.0 * n[9])
    + udSin(8.0 * phi) * (50803200.0 * n[4] + 83825280.0 * n[6] + 113513400.0 * n[8])
    + udSin(10.0 * phi) * (-44706816.0 * n[5] - 72648576.0 * n[7] - 97297200.0 * n[9])
    + udSin(12.0 * phi) * (40360320.0 * n[6] + 64864800.0 * n[8])
    + udSin(14.0 * phi) * (-37065600.0 * n[7] - 59073300.0 * n[9])
    + udSin(16.0 * phi) * (34459425.0 * n[8])
    + udSin(18.0 * phi) * (-32332300.0 * n[9])
    ) / 82575360.0;
}

// ----------------------------------------------------------------------------
// Author: Jules Proust, September 2020
static double udGeoZone_DelambreCoefficients(const double e)
{
  // First Delambre coeficients Ao
  // http://www.mygeodesy.id.au/documents/Meridian%20Distance.pdf p.7
  return 1.0 - 1.0 / 4.0 * udPow(e, 2) - 3.0 / 64.0 * udPow(e, 4) - 5.0 / 256.0 * udPow(e, 6) - 175.0 / 16384.0 * udPow(e, 8) - 441.0 / 65536.0 * udPow(e, 10) - 4851.0 / 1048576.0 * udPow(e, 12) - 14157.0 / 4194304.0 * udPow(e, 14);
}

// ----------------------------------------------------------------------------
// Author: Jules Proust, September 2020
static double udGeoZone_LatMeridianSameNorthing(const double mu, const double e)
{
  // Reverted Helmert's series .... Lack of precision need investigation on Maxima
  // http://www.mygeodesy.id.au/documents/Meridian%20Distance.pdf p.20 eq.61
  return mu
      + (3.0 / 2.0 * e - 20.0 / 32.0 * udPow(e,3)) * udSin(2 * mu)
      + (21.0 / 16.0 * udPow(e,2) - 55.0 / 32.0 * udPow(e,4)) * udSin(4 * mu)
      + 121.0 / 96.0 * udPow(e,3) * udSin(6 * mu)
      + 1097.0 / 512.0 * udPow(e,4) * udSin(8 * mu);;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_LatLongToCartesian(const udGeoZone &zone, const udDouble3 &latLong, bool flipFromLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  double e = zone.eccentricity;
  double phi = ((!flipFromLongLat) ? latLong.x : latLong.y);
  double omega = ((!flipFromLongLat) ? latLong.y : latLong.x);
  double ellipsoidHeight = latLong.z;
  double X, Y;

  if (datum != zone.datum)
  {
    udDouble3 convertedLatLong = udGeoZone_ConvertDatum(udDouble3::create(phi, omega, latLong.z), datum, zone.datum);
    phi = convertedLatLong.x;
    omega = convertedLatLong.y;
    ellipsoidHeight = convertedLatLong.z;
  }

  if (zone.projection == udGZPT_ECEF)
  {
    udGeoZoneEllipsoidInfo ellipsoidInfo = zone.datum > udGZGD_Count ? g_udGZ_StdEllipsoids[g_InternalDatumList[zone.datum - udGZGD_Count].ellipsoid] : g_udGZ_StdEllipsoids[g_udGZ_GeodeticDatumDescriptors[zone.datum].ellipsoid];
    return udGeoZone_LatLongToGeocentric(udDouble3::create(phi, omega, ellipsoidHeight), ellipsoidInfo);
  }
  else if (zone.projection == udGZPT_LatLong)
  {
    return udDouble3::create(phi, omega, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_LongLat)
  {
    return udDouble3::create(omega, phi, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_TransverseMercator)
  {
    // UTM rather than Lambert CC which requires two parallels for calculation
    phi = UD_DEG2RAD(phi);
    omega = UD_DEG2RAD(omega - zone.meridian);

    double sigma = udSinh(e * udATanh(e * udTan(phi) / udSqrt(1 + udPow(udTan(phi), 2))));
    double tanConformalPhi = udTan(phi) * udSqrt(1 + udPow(sigma, 2)) - sigma * udSqrt(1 + udPow(udTan(phi), 2));

    double v = udASinh(udSin(omega) / udSqrt(udPow(tanConformalPhi, 2) + udPow(udCos(omega), 2)));
    double u = udATan2(tanConformalPhi, udCos(omega));

    double eta0 = v;
    double xi0 = u;
    for (size_t i = 0; i < UDARRAYSIZE(zone.alpha); i++)
    {
      double j = (i + 1) * 2.0;
      eta0 += zone.alpha[i] * udCos(j * u) * udSinh(j * v);
      xi0 += zone.alpha[i] * udSin(j * u) * udCosh(j * v);
    }
    eta0 = eta0 * zone.radius;
    xi0 = xi0 * zone.radius;

    return udDouble3::create(zone.scaleFactor * eta0 + zone.falseEasting, zone.scaleFactor * (xi0 - zone.firstParallel) + zone.falseNorthing, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_LambertConformalConic2SP)
  {
    // If two standard parallels, project onto Lambert Conformal Conic
    phi = UD_DEG2RAD(phi);
    omega = UD_DEG2RAD(omega - zone.meridian);

    double phi0 = UD_DEG2RAD(zone.parallel);
    double phi1 = UD_DEG2RAD(zone.firstParallel);
    double phi2 = UD_DEG2RAD(zone.secondParallel);
    double m1 = udGeoZone_LCCMeridonial(phi1, e);
    double m2 = udGeoZone_LCCMeridonial(phi2, e);
    double t = udGeoZone_LCCConformal(phi, e);
    double tOrigin = udGeoZone_LCCConformal(phi0, e);
    double t1 = udGeoZone_LCCConformal(phi1, e);
    double t2 = udGeoZone_LCCConformal(phi2, e);
    double n = (udLogN(m1) - udLogN(m2)) / (udLogN(t1) - udLogN(t2));
    double F = m1 / (n * udPow(t1, n));
    double p0 = zone.semiMajorAxis * F *  udPow(tOrigin, n);
    double p = zone.semiMajorAxis * F *  udPow(t, n);
    X = p * udSin(n * omega);
    Y = p0 - p * udCos(n * omega);

    return udDouble3::create(X + zone.falseEasting, Y + zone.falseNorthing, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_WebMercator)
  {
    phi = UD_DEG2RAD(phi);
    omega = UD_DEG2RAD(omega - zone.meridian);

    X = zone.semiMajorAxis * omega;
    Y = zone.semiMajorAxis * udLogN(udTan(UD_PI/4.0 + phi/2.0));

    return udDouble3::create(X + zone.falseEasting, Y + zone.falseNorthing, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_CassiniSoldner)
  {
    phi = UD_DEG2RAD(phi);
    double A = UD_DEG2RAD(omega - zone.meridian) * udCos(phi);
    double T = udPow(udTan(phi), 2);
    double C = zone.eccentricitySq * udPow(udCos(phi), 2) / (1 - zone.eccentricitySq);
    double nu = zone.semiMajorAxis / udSqrt(1 - zone.eccentricitySq * udPow(udSin(phi), 2));

    double m = zone.semiMajorAxis * udGeoZone_MeridianArcDistance(phi, zone.n);
    double m0 = zone.semiMajorAxis * udGeoZone_MeridianArcDistance(UD_DEG2RAD(zone.parallel), zone.n);

    double x = m - m0 + nu * udTan(phi) * (udPow(A, 2) / 2.0 + (5.0 - T + 6.0 * C) * udPow(A, 4) / 24.0);

    double E = zone.falseEasting + nu * (A - T * udPow(A, 3) / 6.0 - (8.0 - T + 8.0 * C) * T * udPow(A, 5) / 120.0);
    double N = zone.falseNorthing + x;

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_CassiniSoldnerHyperbolic)
  {
    phi = UD_DEG2RAD(phi);
    double A = UD_DEG2RAD(omega - zone.meridian) * udCos(phi);
    double T = udPow(udTan(phi), 2);
    double C = zone.eccentricitySq * udPow(udCos(phi), 2) / (1 - zone.eccentricitySq);
    double nu = zone.semiMajorAxis / udSqrt(1 - zone.eccentricitySq * udPow(udSin(phi), 2));
    double rho = zone.semiMajorAxis * (1 - zone.eccentricitySq) / udPow(1 - zone.eccentricitySq * udPow(udSin(phi), 2), 1.5);

    double m = zone.semiMajorAxis * udGeoZone_MeridianArcDistance(phi, zone.n);
    double m0 = zone.semiMajorAxis * udGeoZone_MeridianArcDistance(UD_DEG2RAD(zone.parallel), zone.n);

    double x = m - m0 + nu * udTan(phi) * (udPow(A, 2) / 2 + (5 - T + 6 * C) * udPow(A, 4) / 24);

    double E = zone.falseEasting + nu * (A - T * udPow(A, 3) / 6 - (8 - T + 8 * C) * T * udPow(A, 5) / 120);
    double N = zone.falseNorthing + x - (udPow(x, 3) / (6 * rho * nu));

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_SterographicObliqueNEquatorial)
  {
    double eSq = zone.eccentricitySq;
    double a = zone.semiMajorAxis;
    phi = UD_DEG2RAD(phi);

    double phi0 = UD_DEG2RAD(zone.parallel);
    double rho0 = a * (1.0 - eSq) / udPow(1.0 - eSq * udPow(udSin(phi0), 2), 3.0 / 2.0);; // radius of curvature at the meridian
    double nu0 = a / udSqrt(1.0 - eSq * udPow(udSin(phi0), 2));; // radius of curvature at the Transverse

    double s1 = (1.0 + udSin(phi0)) / (1.0 - udSin(phi0));
    double s2 = (1.0 - e * udSin(phi0)) / (1.0 + e * udSin(phi0));

    // Sphere constants
    double R = udSqrt(rho0 * nu0);
    double n = udSqrt(1.0 + (eSq * udPow(udCos(phi0), 4) / (1.0 - eSq)));

    double w1 = udPow(s1 * udPow(s2, e), n);
    double sin_chi0 = (w1 - 1.0) / (w1 + 1.0);

    double c = (n + udSin(phi0)) * (1.0 - sin_chi0) / ((n - udSin(phi0)) * (1.0 + sin_chi0));

    double w2 = c * w1;
    double chi0 = udASin((w2 - 1.0) / (w2 + 1.0));
    double lambda0 = UD_DEG2RAD(zone.meridian);

    double lambda = n * (UD_DEG2RAD(omega) - lambda0) + lambda0;
    double sA = (1.0 + udSin(phi)) / (1.0 - udSin(phi));
    double sB = (1.0 - e * udSin(phi)) / (1.0 + e * udSin(phi));
    double w = c * udPow(sA * udPow(sB,e),n);
    double chi = udASin((w - 1.0) / (w + 1.0));

    double B = (1.0 + udSin(chi) * udSin(chi0) + udCos(chi) * udCos(chi0) * cos(lambda - lambda0));

    double E = zone.falseEasting + 2 * R * zone.scaleFactor * udCos(chi) * udSin(lambda - lambda0) / B;
    double N = zone.falseNorthing + 2 * R * zone.scaleFactor * (udSin(chi) * udCos(chi0) - udCos(chi) * udSin(chi0) * udCos(lambda - lambda0)) / B;

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_Mercator)
  {
    // let's try variant B

    phi = UD_DEG2RAD(phi);

    double k0 = udCos(UD_DEG2RAD(zone.firstParallel)) / udSqrt(1 - zone.eccentricitySq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2));
    double E = zone.falseEasting + zone.semiMajorAxis * k0 * (UD_DEG2RAD(omega) - UD_DEG2RAD(zone.meridian));
    double N = zone.falseNorthing + zone.semiMajorAxis * k0 * udLogN(udTan(UD_PI / 4.0 + phi / 2.0) * udPow((1 - zone.eccentricity * udSin(phi)) / (1 + zone.eccentricity * udSin(phi)), zone.eccentricity / 2.0));

    return udDouble3::create(E, N, ellipsoidHeight);

  }
  else if (zone.projection == udGZPT_SterographicPolar_vB)
  {
    bool isNorthPole = zone.parallel > 0.0;
    double eSq = zone.eccentricitySq;
    phi = UD_DEG2RAD(phi);
    omega = UD_DEG2RAD(omega);
    double theta = omega - UD_DEG2RAD(zone.meridian);
    double tF = udTan(UD_PI / 4.0 + UD_DEG2RAD(zone.parallel) / 2.0) / udPow((1 + e * udSin(UD_DEG2RAD(zone.parallel))) / (1 - e - udSin(UD_DEG2RAD(zone.parallel))), e / 2.0);
    double mF = udCos(UD_DEG2RAD(zone.parallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.parallel)), 2));
    double k0 = mF * udSqrt(udPow(1 + e, 1 + e) * udPow(1 - e, 1 - e)) / (2.0 * tF);

    double t = udTan(UD_PI / 4.0 + phi / 2.0) / udPow((1 + e * udSin(phi)) / (1 - e * udSin(phi)), e / 2.0);
    if (isNorthPole)
      t = udTan(UD_PI / 4.0 - phi / 2.0) / udPow((1 + e * udSin(phi)) / (1 - e * udSin(phi)), e / 2.0);

    double rho = 2.0 *zone.semiMajorAxis * k0 * t / udSqrt(udPow(1 + e, 1 + e) * udPow(1 - e, 1 - e));
    double dE = rho * udSin(theta);
    double dN = rho * udCos(theta);
    double E = dE + zone.falseEasting;
    double N = zone.falseNorthing + dN; // north pole N = fN - dN;
    if (isNorthPole)
      N = zone.falseNorthing - dN;

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_Krovak || zone.projection == udGZPT_KrovakNorthOrientated)
  {
    double phiC = UD_DEG2RAD(zone.latProjCentre); // latitude of projection centre
    double alphaC = UD_DEG2RAD(zone.coLatConeAxis);// rotation in plane of meridian of origin || co latitude of the cone axis
    double phiP = UD_DEG2RAD(zone.parallel); // latitude of pseudo standard parallel
    double kP = zone.scaleFactor; // scale factor on pseudo standard parallel
    double lambda0 = UD_DEG2RAD(zone.meridian);
    phi = UD_DEG2RAD(phi);

    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double A = a * udSqrt(1 - eSq) / (1 - eSq * udPow(udSin(phiC), 2));
    double B = udSqrt(1 + (eSq * udPow(udCos(phiC), 4) / (1 - eSq)));
    double gamma0 = udASin(udSin(phiC) / B);
    double t0 = udTan(UD_PI / 4.0 + gamma0 / 2.0) * udPow((1 + e * udSin(phiC)) / (1 - e * udSin(phiC)), e * B / 2.0) / udPow(udTan(UD_PI / 4.0 + phiC / 2.0), B);
    double n = udSin(phiP);
    double r0 = kP * A / udTan(phiP);

    double U = 2.0 * (udATan(t0 * udPow(udTan(phi / 2 + UD_PI / 4.0), B) / udPow((1 + e * udSin(phi)) / (1 - e * udSin(phi)), e * B / 2.0)) - UD_PI / 4.0);
    double V = B * (lambda0 - UD_DEG2RAD(omega));
    double T = udASin(udCos(alphaC) * udSin(U) + udSin(alphaC) * udCos(U) * udCos(V));
    double D = udASin(udCos(U) * udSin(V) / udCos(T));
    double theta = n * D;
    double r = r0 * udPow(udTan(UD_PI / 4.0 + phiP / 2.0), n) / udPow(udTan(T / 2.0 + UD_PI / 4.0), n);
    double Xp = r * udCos(theta);
    double Yp = r * udSin(theta);

    double W = Yp + zone.falseEasting;
    double S = Xp + zone.falseNorthing;

    if (zone.projection == udGZPT_KrovakNorthOrientated)
      return udDouble3::create(-S, -W, ellipsoidHeight);

    return udDouble3::create(S, W, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_HotineObliqueMercatorvA || zone.projection == udGZPT_HotineObliqueMercatorvB)
  {
    phi = UD_DEG2RAD(phi);

    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double alphaC = UD_DEG2RAD(zone.coLatConeAxis);
    double phiC = UD_DEG2RAD(zone.latProjCentre);
    double lambdaC = UD_DEG2RAD(zone.meridian);
    double gammaC = UD_DEG2RAD(zone.parallel);

    double B = udSqrt(1 + (eSq * udPow(udCos(phiC), 4) / (1 - eSq)));
    double A = a * B * zone.scaleFactor * udSqrt(1 - eSq) / (1 - eSq * udPow(udSin(phiC), 2));
    double t0 = udTan(UD_PI / 4.0 - phiC / 2.0) / udPow((1 - e * udSin(phiC)) / (1 + e * udSin(phiC)), e / 2.0);
    double D = B * udSqrt(1 - eSq) / (udCos(phiC) * udSqrt(1 - eSq * udPow(udSin(phiC), 2)));
    double DSq = D < 1.0 ? 1.0 : D * D;
    double sign = phiC < 0 ? -1.0 : 1.0;
    double F = D + udSqrt(DSq - 1) * sign;
    double H = F * udPow(t0, B);
    double G = (F - 1.0 / F) / 2.0;
    double gamma0 = udASin(udSin(alphaC) / D);
    double lambda0 = UD_DEG2RAD(zone.meridian) - (udASin(G * udTan(gamma0))) / B;

    double uC = 0.0;
    if (alphaC == 90)
      uC = A * (lambdaC - lambda0);
    else 
      uC = (A / B) * udATan2(udSqrt(D * D - 1), udCos(alphaC)) * sign;

    double t = udTan(UD_PI / 4.0 - phi / 2.0) / udPow((1 - e * udSin(phi)) / (1 + e * udSin(phi)), e / 2.0);
    double Q = H / udPow(t, B);
    double S = (Q - 1 / Q) / 2;
    double T = (Q + 1 / Q) / 2.0;
    double V = udSin(B * (UD_DEG2RAD(omega) - lambda0));
    double U = (-V * udCos(gamma0) + S * udSin(gamma0)) / T;
    double v = A * udLogN((1 - U) / (1 + U)) / (2 * B);

    //Variant A
    double u = A * udATan2((S * udCos(gamma0) + V * udSin(gamma0)), udCos(B * (UD_DEG2RAD(omega) - lambda0))) / B;

    if (zone.projection == udGZPT_HotineObliqueMercatorvB)
    {
      //Variant B
      u -= (udAbs(uC) * sign);
      double signLambda = (lambdaC - UD_DEG2RAD(omega)) < 0 ? -1.0 : 1.0;
      if (alphaC == 90.0)
      {
        if (UD_DEG2RAD(omega) == lambdaC)
          u = 0;
        else
          u = (A * udATan2(S * udCos(gamma0) + V * udSin(gamma0), udCos(B * (UD_DEG2RAD(omega) - lambda0))) / B) - (udAbs(uC) * sign * signLambda);
      }
    }
    double E = v * udCos(gammaC) + u * udSin(gammaC) + zone.falseEasting;
    double N = u * udCos(gammaC) - v * udSin(gammaC) + zone.falseNorthing;

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_AlbersEqualArea)
  {
    phi = UD_DEG2RAD(phi);
    omega = -UD_DEG2RAD(omega); // West Axis
    //double phi0 = UD_DEG2RAD(zone.latProjCentre - zone.falseEasting);
    //double lambda0 = UD_DEG2RAD(zone.meridian - zone.falseNorthing);
    double phi0 = UD_DEG2RAD(zone.latProjCentre);
    double lambda0 = UD_DEG2RAD(zone.meridian);
    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double OneMeSq = (1 - eSq);
    double alpha = OneMeSq * ((udSin(phi) / (1 - eSq * udPow(udSin(phi), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(phi)) / (1 + e * udSin(phi))));
    double alpha0 = OneMeSq * ((udSin(phi0) / (1 - eSq * udPow(udSin(phi0), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(phi0)) / (1 + e * udSin(phi0))));
    double alpha1 = OneMeSq * ((udSin(UD_DEG2RAD(zone.firstParallel)) / (1 - eSq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(UD_DEG2RAD(zone.firstParallel))) / (1 + e * udSin(UD_DEG2RAD(zone.firstParallel)))));
    double alpha2 = OneMeSq * ((udSin(UD_DEG2RAD(zone.secondParallel)) / (1 - eSq * udPow(udSin(UD_DEG2RAD(zone.secondParallel)), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(UD_DEG2RAD(zone.secondParallel))) / (1 + e * udSin(UD_DEG2RAD(zone.secondParallel)))));

    double m1 = udCos(UD_DEG2RAD(zone.firstParallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2));
    double m2 = udCos(UD_DEG2RAD(zone.secondParallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.secondParallel)), 2));
    double n = (udPow(m1, 2) - udPow(m2, 2)) / (alpha2 - alpha1);
    double C = udPow(m1, 2) + n * alpha1;

    double rho = (a * udSqrt(C - n * alpha)) / n;
    double rho0 = (a * udSqrt(C - n * alpha0)) / n;
    double theta = n * (omega - lambda0);

    double E = zone.falseEasting + rho * udSin(theta);;
    double N = zone.falseNorthing + rho0 - rho * udCos(theta);

    return udDouble3::create(E, N, ellipsoidHeight);
  }
  else if (zone.projection == udGZPT_EquidistantCylindrical)
  {
    double a = zone.semiMajorAxis;

    phi = UD_DEG2RAD(phi);
  
    double eSq = zone.eccentricitySq;
    double M = a * ((1 - (1.0 / 4.0) * eSq - (3.0 / 64.0) * udPow(eSq, 2) - (5.0 / 256.0) * udPow(eSq,3) - (175.0 / 16384.0) * udPow(eSq,4) - (441.0 / 65536.0) * udPow(eSq, 5) - (4851.0 / 1048576.0) * udPow(eSq, 6) - (14157.0 / 4194304.0) * udPow(eSq, 7)) * phi
      + (-(3.0 / 8.0) * eSq - (3.0 / 32.0) * udPow(eSq,2) - (45.0 / 1024.0) * udPow(eSq,3) - (105.0 / 4096.0) * udPow(eSq,4) - (2205.0 / 131072.0) * udPow(eSq,5) - (6237.0 / 524288.0) * udPow(eSq,6) - (297297.0 / 33554432.0) * udPow(eSq,7)) * udSin(2*phi)
      + ((15.0 / 256.0) * udPow(eSq,2) + (45.0 / 1024.0) * udPow(eSq,3) + (525.0 / 16384.0) * udPow(eSq,4) + (1575.0 / 65536.0) * udPow(eSq,5) + (155925.0 / 8388608.0) * udPow(eSq,6) + (495495/33554432) * udPow(eSq,7)) * udSin(4*phi)
      + (-(35.0 / 3072.0) * udPow(eSq,3) - (175.0 / 12288.0) * udPow(eSq,4) - (3675.0 / 262144.0) * udPow(eSq,5) - (13475.0 / 1048576.0) * udPow(eSq,6) - (385385.0 / 33554432.0) * udPow(eSq,7)) * udSin(6*phi)
      + ((315.0 / 131072.0) * udPow(eSq,4) + (2205.0 / 524288.0) * udPow(eSq,5) + (43659.0 / 8388608.0) * udPow(eSq,6) + (189189.0 / 33554432.0) * udPow(eSq,7)) * udSin(8*phi)
      + (-(693.0 / 1310720.0) * udPow(eSq,5) - (6237.0 / 5242880.0) * udPow(eSq,6) - (297297.0 / 167772160.0) * udPow(eSq,7)) * udSin(10*phi)
      + ((1001.0 / 8388608.0) * udPow(eSq,6) + (11011.0 / 33554432.0) * udPow(eSq,7)) * udSin(12*phi)
      + (-(6435.0 / 234881024.0) * udPow(eSq,7)) * udSin(14* phi));

    double v1 = a / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.parallel)), 2));

    double E = zone.falseEasting + v1 * udCos(UD_DEG2RAD(zone.parallel)) * (UD_DEG2RAD(omega) - UD_DEG2RAD(zone.meridian));
    double N = zone.falseNorthing + M;

    return udDouble3::create(E, N, ellipsoidHeight);
  }

  return udDouble3::zero(); // Unsupported projection
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_CartesianToLatLong(const udGeoZone &zone, const udDouble3 &position, bool flipToLongLat /*= false*/, udGeoZoneGeodeticDatum datum /*= udGZGD_WGS84*/)
{
  udDouble3 latLong = udDouble3::zero();
  double e = zone.eccentricity;

  if (zone.projection == udGZPT_ECEF)
  {
    udGeoZoneEllipsoidInfo ellipsoidInfo = zone.datum > udGZGD_Count ? g_udGZ_StdEllipsoids[g_InternalDatumList[zone.datum - udGZGD_Count].ellipsoid] : g_udGZ_StdEllipsoids[g_udGZ_GeodeticDatumDescriptors[zone.datum].ellipsoid];
    latLong = udGeoZone_GeocentricToLatLong(position, ellipsoidInfo);
  }
  else if (zone.projection == udGZPT_LatLong)
  {
    latLong = position;
  }
  else if (zone.projection == udGZPT_LongLat)
  {
    latLong = udDouble3::create(position.y, position.x, position.z);
  }
  else if (zone.projection == udGZPT_TransverseMercator)
  {
    double eta = (position.x - zone.falseEasting) / (zone.radius * zone.scaleFactor);
    double xi = (zone.firstParallel * zone.scaleFactor + position.y - zone.falseNorthing) / (zone.radius * zone.scaleFactor);

    double eta0 = eta;
    double xi0 = xi;
    for (size_t i = 0; i < UDARRAYSIZE(zone.beta); i++)
    {
      double j = (i + 1) * 2.0;
      xi0  += zone.beta[i] * udSin(j * xi) * udCosh(j * eta);
      eta0 += zone.beta[i] * udCos(j * xi) * udSinh(j * eta);
    }

    double tanConformalPhi = udSin(xi0) / (udSqrt(udPow(udSinh(eta0), 2) + udPow(udCos(xi0), 2)));
    double omega = udATan2(udSinh(eta0), udCos(xi0));
    double t = tanConformalPhi;

    for (int i = 0; i < 5; ++i)
      t = t - udGeoZone_LCCLatConverge(t, tanConformalPhi, e);

    latLong.x = UD_RAD2DEG(udATan(t));
    latLong.y = (zone.meridian + UD_RAD2DEG(omega));
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_LambertConformalConic2SP)
  {
    double y = position.y - zone.falseNorthing;
    double x = position.x - zone.falseEasting;
    double phi0 = UD_DEG2RAD(zone.parallel);
    double phi1 = UD_DEG2RAD(zone.firstParallel);
    double phi2 = UD_DEG2RAD(zone.secondParallel);
    double m1 = udGeoZone_LCCMeridonial(phi1, e);
    double m2 = udGeoZone_LCCMeridonial(phi2, e);
    double tOrigin = udGeoZone_LCCConformal(phi0, e);
    double t1 = udGeoZone_LCCConformal(phi1, e);
    double t2 = udGeoZone_LCCConformal(phi2, e);
    double n = (udLogN(m1) - udLogN(m2)) / (udLogN(t1) - udLogN(t2));
    double F = m1 / (n * udPow(t1, n));
    double p0 = zone.semiMajorAxis * F *  udPow(tOrigin, n);
    double p = udSqrt(x * x + (p0 - y) * (p0 - y)); // This is r' in the EPSG specs- it must have the same sign as n
    if (n < 0)
      p = -p;

    double theta = udATan(x / (p0 - y));
    double t = udPow(p / (zone.semiMajorAxis * F), 1 / n);
    double phi = UD_HALF_PI - 2.0 * udATan(t);
    for (int i = 0; i < 5; ++i)
      phi = UD_HALF_PI - 2 * udATan(t * udPow((1 - e * udSin(phi)) / (1 + e * udSin(phi)), e / 2.0));

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(theta / n) + zone.meridian;
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_WebMercator)
  {
    double y = position.y - zone.falseNorthing;
    double x = position.x - zone.falseEasting;

    double phi = UD_HALF_PI - 2 * udATan(udExp(-y / zone.semiMajorAxis));
    double omega = (x / zone.semiMajorAxis);

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(omega) + zone.meridian;
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_CassiniSoldner)
  {
    // Reading variables
    double a = zone.semiMajorAxis;
    double lmESq = 1 - zone.eccentricitySq;

    double m0 = a * udGeoZone_MeridianArcDistance(UD_DEG2RAD(zone.parallel), zone.n);
    double m1 = m0 + (position.y - zone.falseNorthing);
    double mu1 = m1 / (a * udGeoZone_DelambreCoefficients(zone.eccentricity));
    double e1 = (1 - udSqrt(lmESq)) / (1 + udSqrt(lmESq));
    double phi1 = udGeoZone_LatMeridianSameNorthing(mu1, e1);

    double nu1 = a / udSqrt(1.0 - zone.eccentricitySq * udPow(udSin(phi1), 2));
    double rho1 = a * (lmESq) / udPow(1.0 - zone.eccentricitySq * udPow(udSin(phi1), 2), 1.5);

    double t1 = udPow(udTan(phi1), 2);
    double d = (position.x - zone.falseEasting) / nu1;

    latLong.x = UD_RAD2DEG(phi1 - (nu1 * udTan(phi1) / rho1) * (udPow(d, 2) / 2.0 - (1.0 + 3.0 * t1) * udPow(d, 4) / 24.0));
    latLong.y = UD_RAD2DEG(UD_DEG2RAD(zone.meridian) + (d - t1 * udPow(d, 3) / 3.0 + (1.0 + 3.0 * t1) * t1 * udPow(d, 5) / 15.0) / udCos(phi1));
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_CassiniSoldnerHyperbolic)
  {
    double phi0 = UD_DEG2RAD(zone.parallel);
    double lambda0 = UD_DEG2RAD(zone.meridian);

    // Reading variables
    double a = zone.semiMajorAxis;
    double NmFN = position.y - zone.falseNorthing;
    double lmESq = 1 - zone.eccentricitySq;

    double m0 = a * udGeoZone_MeridianArcDistance(phi0, zone.n);
    double phi1p = phi0 + NmFN / 315320.0;
    double rho1p = a * lmESq / udPow(lmESq * udPow(udSin(phi1p), 2), 1.5);
    double nu1p = a / udSqrt(lmESq * udPow(udSin(phi1p), 2));
    double qp = udPow(NmFN, 3) / (6 * rho1p * nu1p);
    double q = udPow(NmFN + qp, 3) / (6 * rho1p * nu1p);
    double m1 = m0 + NmFN + q;
    double mu1 = m1 / (a * udGeoZone_DelambreCoefficients(zone.eccentricity));
    double e1 = (1 - udSqrt(lmESq)) / (1 + udSqrt(lmESq));
    double phi1 = udGeoZone_LatMeridianSameNorthing(mu1, e1);

    double nu1 = a / udSqrt(lmESq * udPow(udSin(phi1), 2));
    double rho1 = a * lmESq / udPow(lmESq * udPow(udSin(phi1), 2), 1.5);

    double t1 = udPow(udTan(phi1), 2);
    double d = (position.x - zone.falseEasting) / nu1;

    latLong.x = UD_RAD2DEG(phi1 - (nu1 * udTan(phi1) / rho1) * (udPow(d, 2) / 2 - (1 + 3 * t1) * udPow(d, 4) / 24));
    latLong.y = UD_RAD2DEG(lambda0 + (d - t1 * udPow(d, 3) / 3 + (1 + 3 * t1) * t1 * udPow(d, 5) / 15) / udCos(phi1));
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_SterographicObliqueNEquatorial)
  {
    double eSq = zone.eccentricitySq;
    double a = zone.semiMajorAxis;

    double phi0 = UD_DEG2RAD(zone.parallel);
    double rho0 = a * (1.0 - eSq) / udPow(1.0 - eSq * udPow(udSin(phi0), 2), 3.0 / 2.0);; // radius of curvature at the meridian
    double nu0 = a / udSqrt(1.0 - eSq * udPow(udSin(phi0), 2));; // radius of curvature at the Transverse

    double s1 = (1.0 + udSin(phi0)) / (1.0 - udSin(phi0));
    double s2 = (1.0 - e * udSin(phi0)) / (1.0 + e * udSin(phi0));

    // Sphere constants
    double R = udSqrt(rho0 * nu0);
    double n = udSqrt(1.0 + (eSq * udPow(udCos(phi0), 4) / (1.0 - eSq)));

    double w1 = udPow(s1 * udPow(s2, e), n);
    double sin_chi0 = (w1 - 1.0) / (w1 + 1.0);

    double c = (n + udSin(phi0)) * (1.0 - sin_chi0) / ((n - udSin(phi0)) * (1.0 + sin_chi0));

    double w2 = c * w1;
    double chi0 = udASin((w2 - 1.0) / (w2 + 1.0));

    double g = 2 * R * zone.scaleFactor * udTan(UD_PI / 4.0 - chi0 / 2.0);
    double h = 4 * R * zone.scaleFactor * udTan(chi0) + g;
    double i = udATan2((position.x - zone.falseEasting), (h + (position.y - zone.falseNorthing)));
    double j = udATan2((position.x - zone.falseEasting), (g - (position.y - zone.falseNorthing))) - i;

    double chi = chi0 + 2 * udATan(((position.y - zone.falseNorthing) - (position.x - zone.falseEasting)*udTan(j/2.0)) / (2.0 * R * zone.scaleFactor));
    double lambda = j + 2 * i + UD_DEG2RAD(zone.meridian);

    double psi = 0.5 * udLogN((1 + udSin(chi)) / (c * (1 - udSin(chi)))) / n;

    double phi = 0;
    double phiTmp = 2 * udATan(udExp(psi)) - UD_HALF_PI;
    double psiI = 0;
    double epsilon = udPow(10.0, -14.0);
    while (fabs(phi- phiTmp) > epsilon && !isnan(phi))
    {
      phi = phiTmp;
      psiI = udLogN(udTan(phiTmp / 2.0 + UD_PI / 4.0) * udPow((1 - e * udSin(phiTmp)) / (1 + e * udSin(phiTmp)), (e / 2.0)));
      phiTmp = phi - (psiI - psi) * udCos(phi) * (1 - eSq * udPow(udSin(phi), 2)) / (1 - eSq);
    }

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(((lambda - UD_DEG2RAD(zone.meridian)) / n) + UD_DEG2RAD(zone.meridian));
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_Mercator)
  {
    double eSq = zone.eccentricitySq;
    double k0 = udCos(UD_DEG2RAD(zone.firstParallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2));
    double B = udExp(1.0);
    double t = udPow(B, (zone.falseNorthing - position.y)/(zone.semiMajorAxis * k0));
    double chi = UD_HALF_PI - 2.0 * udATan(t);
    double phi = chi + (eSq / 2.0 + 5.0 * udPow(eSq, 2) / 24.0 + udPow(eSq, 3) / 12.0 + 13.0 * udPow(eSq, 4) / 360.0) * udSin(2.0 * chi)
      + (7.0 * udPow(eSq, 2) / 48.0 + 29.0 * udPow(eSq, 3) / 240.0 + 811.0 * udPow(eSq, 4) / 11520.0) * udSin(4.0 * chi)
      + (7.0 * udPow(eSq, 3) / 120.0 + 81.0 * udPow(eSq, 4) / 1120.0) * udSin(6.0 * chi)
      + (4279.0 * udPow(eSq, 4) / 161280.0) * udSin(8.0 * chi);

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG((position.x - zone.falseEasting) / (zone.semiMajorAxis * k0) + zone.meridian);
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_SterographicPolar_vB)
  {
    bool isNorthPole = zone.parallel > 0.0;
    double eSq = zone.eccentricitySq;

    // South Pole
    double tF = udTan(UD_PI / 4.0 + UD_DEG2RAD(zone.parallel) / 2.0) / udPow((1 + e * udSin(UD_DEG2RAD(zone.parallel))) / (1 - e - udSin(UD_DEG2RAD(zone.parallel))), e / 2.0);
    double mF = udCos(UD_DEG2RAD(zone.parallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.parallel)), 2));
    double k0 = mF * udSqrt(udPow(1 + e, 1 + e) * udPow(1 - e, 1 - e)) / (2.0 * tF);
    double rhoP = udSqrt(udPow(position.x - zone.falseEasting, 2) + udPow(position.y - zone.falseNorthing, 2));
    double tP = rhoP * udSqrt(udPow(1 + e, 1 + e) * udPow(1 - e, 1 - e)) / (2.0 * zone.semiMajorAxis * k0);

    double chi = 2.0 * udATan(tP) - UD_HALF_PI;
    if (isNorthPole)// north pole
      chi = -chi;

    double phi = chi + (eSq / 2.0 + 5.0 * udPow(eSq, 2) / 24.0 + udPow(eSq, 3) / 12.0 + 13.0 * udPow(eSq, 4) / 360.0) * udSin(2.0 * chi)
      + (7.0 * udPow(eSq, 2) / 48.0 + 29.0 * udPow(eSq, 3) / 240.0 + 811.0 * udPow(eSq, 4) / 11520.0) * udSin(4.0 * chi)
      + (7.0 * udPow(eSq, 3) / 120.0 + 81.0 * udPow(eSq, 4) / 1120.0) * udSin(6.0 * chi)
      + (4279.0 * udPow(eSq, 4) / 161280.0) * udSin(8.0 * chi);

    latLong.x = UD_RAD2DEG(phi);
    if (position.x == zone.falseEasting)
    {
      latLong.y = zone.falseEasting;
    }
    else
    {
      latLong.y = zone.meridian + UD_RAD2DEG(udATan2(position.x - zone.falseEasting, position.y - zone.falseNorthing));
      if (isNorthPole) // north pole
        latLong.y = zone.meridian + UD_RAD2DEG(udATan2(zone.falseEasting - position.x, zone.falseNorthing - position.y));
    }
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_Krovak || zone.projection == udGZPT_KrovakNorthOrientated)
  {
    double southing = position.x;
    double westing = position.y;
    if (zone.projection == udGZPT_KrovakNorthOrientated)
    {
      southing *= -1.0;
      westing *= -1.0;
    }

    double phiC = UD_DEG2RAD(zone.latProjCentre); // latitude of projection centre
    double alphaC = UD_DEG2RAD(zone.coLatConeAxis);// rotation in plane of meridian of origin || co latitude of the cone axis
    double phiP = UD_DEG2RAD(zone.parallel); // latitude of pseudo standard parallel
    double kP = zone.scaleFactor; // scale factor on pseudo standard parallel

    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double A = a * udSqrt(1 - eSq) / (1 - eSq * udPow(udSin(phiC), 2));
    double B = udSqrt(1 + (eSq * udPow(udCos(phiC), 4) / (1 - eSq)));
    double gamma0 = udASin(udSin(phiC) / B);
    double t0 = udTan(UD_PI / 4.0 + gamma0 / 2.0) * udPow((1 + e * udSin(phiC)) / (1 - e * udSin(phiC)), e * B / 2.0) / udPow(udTan(UD_PI / 4.0 + phiC / 2.0), B);
    double n = udSin(phiP);
    double r0 = kP * A / udTan(phiP);

    double Xpp = southing - zone.falseNorthing;
    double Ypp = westing - zone.falseEasting;

    double rP = udSqrt(udPow(Xpp, 2) + udPow(Ypp, 2));
    double thetaP = udATan2(Ypp, Xpp);
    double Dp = thetaP / udSin(phiP);
    double Tp = 2 * (udATan(udPow(r0 / rP, 1.0 / n) * udTan(UD_PI / 4.0 + phiP / 2.0)) - UD_PI / 4.0);
    double Up = udASin(udCos(alphaC) * udSin(Tp) - udSin(alphaC) * udCos(Tp) * udCos(Dp));
    double Vp = udASin(udCos(Tp) * udSin(Dp) / udCos(Up));

    double phi = Up;
    double phiTmp = 2 * (udATan(udPow(t0, -1.0 / B) * udPow(udTan(Up / 2.0 + UD_PI / 4.0), 1.0 / B) * udPow((1.0 + e * udSin(phi)), e / 2.0)) - UD_PI / 4.0);
    double epsilon = udPow(10.0, -14.0);
    while (fabs(phi - phiTmp) > epsilon && !isnan(phi))
    {
      phi = phiTmp;
      phiTmp = 2 * (udATan(udPow(t0, -1.0 / B) * udPow(udTan(Up / 2.0 + UD_PI / 4.0), 1.0 / B) * udPow((1.0 + e * udSin(phi)), e / 2.0)) - UD_PI / 4.0);
    }

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(UD_DEG2RAD(zone.meridian) - Vp / B);
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_HotineObliqueMercatorvA || zone.projection == udGZPT_HotineObliqueMercatorvB)
  {
    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double alphaC = UD_DEG2RAD(zone.coLatConeAxis);
    double phiC = UD_DEG2RAD(zone.latProjCentre);
    double lambdaC = UD_DEG2RAD(zone.meridian);
    double gammaC = UD_DEG2RAD(zone.parallel);

    double B = udSqrt(1 + (eSq * udPow(udCos(phiC), 4) / (1 - eSq)));
    double A = a * B * zone.scaleFactor * udSqrt(1 - eSq) / (1 - eSq * udPow(udSin(phiC), 2));
    double t0 = udTan(UD_PI / 4.0 - phiC / 2.0) / udPow((1 - e * udSin(phiC)) / (1 + e * udSin(phiC)), e / 2.0);
    double D = B * udSqrt(1 - eSq) / (udCos(phiC) * udSqrt(1 - eSq * udPow(udSin(phiC), 2)));
    double DSq = D < 1.0 ? 1.0 : D * D;
    double sign = phiC < 0 ? -1.0 : 1.0;
    double F = D + udSqrt(DSq - 1) * sign;
    double H = F * udPow(t0, B);
    double G = (F - 1.0 / F) / 2.0;
    double gamma0 = udASin(udSin(alphaC) / D);
    double lambda0 = UD_DEG2RAD(zone.meridian) - (udASin(G * udTan(gamma0))) / B;

    double uC = 0.0;
    if (alphaC == 90)
      uC = A * (lambdaC - lambda0);
    else
      uC = (A / B) * udATan2(udSqrt(D * D - 1), udCos(alphaC)) * sign;

    //Variant A
    double vP = (position.x - zone.falseEasting) * udCos(gammaC) - (position.y - zone.falseNorthing) * udSin(gammaC);
    double uP = (position.y - zone.falseNorthing) * udCos(gammaC) + (position.x - zone.falseEasting) * udSin(gammaC);

    if (zone.projection == udGZPT_HotineObliqueMercatorvB)
    {
      //Variant B
      uP += udAbs(uC) * sign;
    }

    double Qp = udExp(-(B * vP / A));
    double Sp = (Qp - 1 / Qp) / 2.0;
    double Tp = (Qp + 1 / Qp) / 2.0;
    double Vp = udSin(B * uP / A);
    double Up = (Vp * udCos(gamma0) + Sp * udSin(gamma0)) / Tp;
    double tP = udPow(H / udSqrt((1 + Up) / (1 - Up)), 1 / B);
    double chi = UD_HALF_PI - 2.0 * udATan(tP);

    double phi = chi + (eSq / 2.0 + 5.0 * udPow(eSq, 2) / 24.0 + udPow(eSq, 3) / 12.0 + 13.0 * udPow(eSq, 4) / 360.0) * udSin(2.0 * chi)
      + (7.0 * udPow(eSq, 2) / 48.0 + 29.0 * udPow(eSq, 3) / 240.0 + 811.0 * udPow(eSq, 4) / 11520.0) * udSin(4.0 * chi)
      + (7.0 * udPow(eSq, 3) / 120.0 + 81.0 * udPow(eSq, 4) / 1120.0) * udSin(6.0 * chi)
      + (4279.0 * udPow(eSq, 4) / 161280.0) * udSin(8.0 * chi);

    double lambda = lambda0 - udATan2(Sp * udCos(gamma0) - Vp * udSin(gamma0), udCos(B * uP / A)) / B;

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(lambda);
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_AlbersEqualArea)
  {
    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;

    double phi0 = UD_DEG2RAD(zone.latProjCentre);
    //double lambda0 = UD_DEG2RAD(zone.meridian);

    double OneMeSq = (1 - eSq);
    double alpha0 = OneMeSq * ((udSin(phi0) / (1 - eSq * udPow(udSin(phi0), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(phi0)) / (1 + e * udSin(phi0))));
    double alpha1 = OneMeSq * ((udSin(UD_DEG2RAD(zone.firstParallel)) / (1 - eSq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(UD_DEG2RAD(zone.firstParallel))) / (1 + e * udSin(UD_DEG2RAD(zone.firstParallel)))));
    double alpha2 = OneMeSq * ((udSin(UD_DEG2RAD(zone.secondParallel)) / (1 - eSq * udPow(udSin(UD_DEG2RAD(zone.secondParallel)), 2))) - (1 / (2 * e)) * udLogN((1 - e * udSin(UD_DEG2RAD(zone.secondParallel))) / (1 + e * udSin(UD_DEG2RAD(zone.secondParallel)))));

    double m1 = udCos(UD_DEG2RAD(zone.firstParallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.firstParallel)), 2));
    double m2 = udCos(UD_DEG2RAD(zone.secondParallel)) / udSqrt(1 - eSq * udPow(udSin(UD_DEG2RAD(zone.secondParallel)), 2));
    double n = (udPow(m1, 2) - udPow(m2, 2)) / (alpha2 - alpha1);
    double C = udPow(m1, 2) + n * alpha1;

    double rho0 = (a * udSqrt(C - n * alpha0)) / n;
    double theta = 0.0;
    if (n > 0.0)
      theta = udATan2((position.x - zone.falseEasting), (rho0 - (position.y - zone.falseNorthing)));
    else
      theta = udATan2(-(position.x - zone.falseEasting), -(rho0 - (position.y - zone.falseNorthing)));

    double rhoP = udSqrt(udPow(position.x - zone.falseEasting,2) + udPow(rho0 - (position.y - zone.falseNorthing), 2));
    double alphaP = (C - (udPow(rhoP, 2) * udPow(n,2) / udPow(a,2))) / n;
    double betaP = udASin(alphaP / (1 - ((1 - eSq) / (2 * e)) * udLogN ((1 - e) / (1 + e))));

    double phi = betaP
      + ((eSq / 3.0 + 31 * udPow(e, 4) / 180 + 517 * udPow(e, 6) / 5040) * udSin(2 * betaP))
      + ((23 * udPow(e, 4) / 360 + 251 * udPow(e, 6) / 3780) * udSin(4 * betaP))
      + ((761 * udPow(e, 6) / 45360) * udSin(6 * betaP));
    double lambda = UD_DEG2RAD(zone.meridian) + (theta / n);

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(-lambda);// West Axis
    latLong.z = position.z;
  }
  else if (zone.projection == udGZPT_EquidistantCylindrical)
  {
    double a = zone.semiMajorAxis;
    double eSq = zone.eccentricitySq;
    double X = position.x - zone.falseEasting;
    double Y = position.y - zone.falseNorthing;

    double mu = Y / (a * udGeoZone_DelambreCoefficients(zone.eccentricity));
    double eta = (1 - udSqrt(1 - eSq)) / (1 + udSqrt(1 - eSq));

    double lambda = UD_DEG2RAD(zone.meridian) + X * udSqrt(1.0 - eSq * udPow(udSin(zone.parallel), 2)) / (a * udCos(zone.parallel));
    double phi = mu + ((3.0 / 2.0) * eta - (27.0 / 32.0) * udPow(eta, 3) + (269.0 / 512.0) * udPow(eta, 5) - (6607.0 / 24576.0) * udPow(eta, 7)) * udSin(2 * mu)
      + ((21.0 / 16.0) * udPow(eta, 2) - (55.0 / 32.0) * udPow(eta, 4) + (6759.0 / 4096.0) * udPow(eta, 6)) * udSin(4 * mu)
      + ((151.0 / 96.0) * udPow(eta, 3) - (417.0 / 128.0) * udPow(eta, 5) + (87963.0 / 20480.0) * udPow(eta, 7)) * udSin(6 * mu)
      + ((1097.0 / 512.0) * udPow(eta, 4) - (15543.0 / 2560.0) * udPow(eta, 6)) * udSin(8 * mu)
      + ((8011.0 / 2560.0) * udPow(eta, 5) - (69119.0 / 6144.0) * udPow(eta, 7)) * udSin(10 * mu)
      + ((293393.0 / 61440.0) * udPow(eta, 6)) * udSin(12 * mu)
      + ((6845701.0 / 860160.0) * udPow(eta, 7)) * udSin(14 * mu);

    latLong.x = UD_RAD2DEG(phi);
    latLong.y = UD_RAD2DEG(lambda);
    latLong.z = position.z;
  } 

  if (datum != zone.datum)
    latLong = udGeoZone_ConvertDatum(latLong, zone.datum, datum);

  return (!flipToLongLat) ? latLong : udDouble3::create(latLong.y, latLong.x, latLong.z);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
udDouble3 udGeoZone_TransformPoint(const udDouble3 &point, const udGeoZone &sourceZone, const udGeoZone &destZone)
{
  if (sourceZone.srid == destZone.srid)
    return point;

  udDouble3 latlon = udGeoZone_CartesianToLatLong(sourceZone, point);
  return udGeoZone_LatLongToCartesian(destZone, latlon);
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, August 2018
udDouble4x4 udGeoZone_TransformMatrix(const udDouble4x4 &matrix, const udGeoZone &sourceZone, const udGeoZone &destZone)
{
  if (sourceZone.srid == destZone.srid)
    return matrix;

  udDouble3 position, scale;
  udDoubleQuat orientation;
  matrix.extractTransforms(position, scale, orientation);

  udDouble3 llO = udGeoZone_CartesianToLatLong(sourceZone, matrix.axis.t.toVector3());
  udDouble3 llX = udGeoZone_CartesianToLatLong(sourceZone, matrix.axis.t.toVector3() + udNormalize3(matrix.axis.x.toVector3()));
  udDouble3 llY = udGeoZone_CartesianToLatLong(sourceZone, matrix.axis.t.toVector3() + udNormalize3(matrix.axis.y.toVector3()));
  udDouble3 llZ = udGeoZone_CartesianToLatLong(sourceZone, matrix.axis.t.toVector3() + udNormalize3(matrix.axis.z.toVector3()));

  udDouble3 czO = udGeoZone_LatLongToCartesian(destZone, llO);
  udDouble3 czX = (udGeoZone_LatLongToCartesian(destZone, llX) - czO);
  udDouble3 czY = (udGeoZone_LatLongToCartesian(destZone, llY) - czO);
  udDouble3 czZ = (udGeoZone_LatLongToCartesian(destZone, llZ) - czO);

  //Orthonormalise using czZ as the reference seems to give the best results.
  czY = udCross3(czZ, czX);
  czX = udCross3(czY, czZ);

  udDouble4x4 m;
  m.axis.x = udDouble4::create(udNormalize3(czX) * scale.x, 0);
  m.axis.y = udDouble4::create(udNormalize3(czY) * scale.y, 0);
  m.axis.z = udDouble4::create(udNormalize3(czZ) * scale.z, 0);
  m.axis.t = udDouble4::create(czO, 1);

  return m;
}
