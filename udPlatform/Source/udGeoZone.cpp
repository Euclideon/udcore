#include "udGeoZone.h"

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udResult udGeoZone_FindSRID(int32_t * /*pSRIDCode*/, const udDouble2 & /*longLat*/)
{
  return udR_Unsupported;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static void SetWGS84Ellipsoid(udGeoZone *pZone)
{
  pZone->flattening = 1 / 298.257223563;
  pZone->semiMajorAxis = 6378137;
  pZone->semiMinorAxis = pZone->semiMajorAxis * (1 - pZone->flattening);
  pZone->thirdFlattening = (pZone->semiMajorAxis - pZone->semiMinorAxis) / (pZone->semiMajorAxis + pZone->semiMinorAxis);
  pZone->eccentricity = udSqrt(1 - udPow(pZone->semiMinorAxis / pZone->semiMajorAxis, 2));
  pZone->secEccentricitySq = pZone->eccentricity * pZone->eccentricity / (1 - pZone->eccentricity * pZone->eccentricity);
  pZone->meridonialParams[0] = 6367449.146;
  pZone->meridonialParams[1] = 16038.42955;
  pZone->meridonialParams[2] = 16.83261333;
  pZone->meridonialParams[3] = 0.021984404;
  pZone->meridonialParams[4] = 0.00031270;
  pZone->phiParams[0] = 3 * pZone->thirdFlattening / 2 - 27 * udPow(pZone->thirdFlattening, 3) / 32.0;
  pZone->phiParams[1] = 21 * udPow(pZone->thirdFlattening, 2) / 16 - 55 * udPow(pZone->thirdFlattening, 4) / 32;
  pZone->phiParams[2] = 151 * udPow(pZone->thirdFlattening, 3) / 96;
  pZone->phiParams[3] = 1097 * udPow(pZone->thirdFlattening, 4) / 512;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
static void SetNAD83Ellipsoid(udGeoZone *pZone)
{
  pZone->flattening = 1 / 298.257222101;
  pZone->semiMajorAxis = 6378137;
  pZone->semiMinorAxis = pZone->semiMajorAxis * (1 - pZone->flattening);
  pZone->thirdFlattening = (pZone->semiMajorAxis - pZone->semiMinorAxis) / (pZone->semiMajorAxis + pZone->semiMinorAxis);
  pZone->eccentricity = udSqrt(1 - udPow(pZone->semiMinorAxis / pZone->semiMajorAxis, 2));
  pZone->secEccentricitySq = pZone->eccentricity * pZone->eccentricity / (1 - pZone->eccentricity * pZone->eccentricity);
  pZone->meridonialParams[0] = 6367449.146;
  pZone->meridonialParams[1] = 16038.42955;
  pZone->meridonialParams[2] = 16.83261333;
  pZone->meridonialParams[3] = 0.021984404;
  pZone->meridonialParams[4] = 0.00031270;
  pZone->phiParams[0] = 3 * pZone->thirdFlattening / 2 - 27 * udPow(pZone->thirdFlattening, 3) / 32.0;
  pZone->phiParams[1] = 21 * udPow(pZone->thirdFlattening, 2) / 16 - 55 * udPow(pZone->thirdFlattening, 4) / 32;
  pZone->phiParams[2] = 151 * udPow(pZone->thirdFlattening, 3) / 96;
  pZone->phiParams[3] = 1097 * udPow(pZone->thirdFlattening, 4) / 512;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2018
static void SetUTMZoneBounds(udGeoZone *pZone)
{
  pZone->latLongBoundMin.x = (pZone->hemisphere == 'N') ? 0 : -80;
  pZone->latLongBoundMax.x = (pZone->hemisphere == 'N') ? 84 : 0;
  pZone->latLongBoundMin.y = (pZone->meridian >= 0) ? pZone->meridian - 3 : pZone->meridian + 3;
  pZone->latLongBoundMax.y = (pZone->meridian >= 0) ? pZone->meridian + 3 : pZone->meridian - 3;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udResult udGeoZone_SetFromSRID(udGeoZone *pZone, int32_t sridCode)
{
  if (pZone == nullptr)
    return udR_InvalidParameter_;
  memset(pZone->padding, 0, sizeof(pZone->padding));

  if (sridCode > 32600 && sridCode < 32661)
  {
    // WGS84 Northern Hemisphere
    pZone->zone = sridCode - 32600;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->hemisphere = 'N';
    SetWGS84Ellipsoid(pZone);
    SetUTMZoneBounds(pZone);
  }
  else if (sridCode > 32700 && sridCode < 32761)
  {
    // WGS84 Southern Hemisphere
    pZone->zone = sridCode - 32700;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->hemisphere = 'S';
    SetWGS84Ellipsoid(pZone);
    SetUTMZoneBounds(pZone);
  }
  else if (sridCode > 26900 && sridCode < 26924)
  {
    // NAD83 Northern Hemisphere
    pZone->zone = sridCode - 26900;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->falseNorthing = 0;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->hemisphere = 'N';
    SetNAD83Ellipsoid(pZone);
    SetUTMZoneBounds(pZone);
  }
  else if (sridCode > 28347 && sridCode < 28357)
  {
    // GDA94 Southern Hemisphere (for MGA)
    pZone->zone = sridCode - 28300;
    pZone->meridian = pZone->zone * 6 - 183;
    pZone->falseNorthing = 10000000;
    pZone->falseEasting = 500000;
    pZone->scaleFactor = 0.9996;
    pZone->hemisphere = 'S';
    SetNAD83Ellipsoid(pZone);
    SetUTMZoneBounds(pZone);
  }
  else
  {
    return udR_ObjectNotFound;
  }

  return udR_Success;
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_ToCartesian(const udGeoZone &zone, const udDouble3 &latLong)
{
  double latitude = latLong.x;
  double k0 = zone.scaleFactor;

  // eccentricity
  double e1sq = zone.secEccentricitySq;

  double nu = 6389236.914; // r curv 2 WGS84 only

  // Calculate Meridional Arc Length / Meridional Arc
  double S;

  // Calculation Constants
  // Delta Long
  double p = -0.483084;
  double sin1 = 4.84814E-06;

  // Coefficients for UTM Coordinates
  double K1 = 5101225.115;
  double K2 = 3750.291596;
  double K3 = 1.397608151;
  double K4 = 214839.3105;
  double K5 = -2.995382942;

  // Set Variables
  latitude = UD_DEG2RAD(latitude);
  nu = zone.semiMajorAxis / udPow(1 - udPow(zone.eccentricity * udSin(latitude), 2), (1 / 2.0));

  p = (latLong.y - zone.meridian) * 3600 / 10000;

  S = zone.meridonialParams[0] * latitude - zone.meridonialParams[1] * udSin(2 * latitude) + zone.meridonialParams[2] * udSin(4 * latitude) - zone.meridonialParams[3] * udSin(6 * latitude) + zone.meridonialParams[4] * udSin(8 * latitude);

  K1 = S * k0;
  K2 = nu * udSin(latitude) * udCos(latitude) * udPow(sin1, 2) * k0 * (100000000) / 2;
  K3 = ((udPow(sin1, 4) * nu * udSin(latitude) * udPow(udCos(latitude), 3)) / 24) * (5 - udPow(udTan(latitude), 2) + 9 * e1sq * udPow(udCos(latitude), 2) + 4 * udPow(e1sq, 2) * udPow(udCos(latitude), 4)) * k0 * (10000000000000000L);
  K4 = nu * udCos(latitude) * sin1 * k0 * 10000;
  K5 = udPow(sin1 * udCos(latitude), 3) * (nu / 6) * (1 - udPow(udTan(latitude), 2) + e1sq * udPow(udCos(latitude), 2)) * k0 * 1000000000000L;

  //NORTHING
  double northing = K1 + K2 * p * p + K3 * udPow(p, 4);
  if (latitude < 0.0)
    northing = zone.falseNorthing + northing;

  return udDouble3::create(zone.falseEasting + (K4 * p + K5 * udPow(p, 3)), northing, latLong.z);
}

// ----------------------------------------------------------------------------
// Author: Lauren Jones, June 2018
udDouble3 udGeoZone_ToLatLong(const udGeoZone & zone, const udDouble3 &position)
{
  double arc;
  double mu;
  double n0, r0, t0;
  double _a1, _a2;
  double dd0;
  double Q0;
  double lof1, lof2, lof3;
  double phi1;
  double fact1, fact2, fact3, fact4;

  double easting = position.x;
  double northing = position.y;

  double e = zone.eccentricity;
  double e1sq = zone.secEccentricitySq;

  if (zone.hemisphere == 'S')
    northing = zone.falseNorthing - northing;

  // Set Variables
  arc = northing / zone.scaleFactor;
  mu = arc / (zone.semiMajorAxis * (1 - udPow(e, 2) / 4.0 - 3 * udPow(e, 4) / 64.0 - 5 * udPow(e, 6) / 256.0));

  phi1 = mu + zone.phiParams[0] * udSin(2 * mu) + zone.phiParams[1] * udSin(4 * mu) + zone.phiParams[2] * udSin(6 * mu) + zone.phiParams[3] * udSin(8 * mu);
  n0 = zone.semiMajorAxis / udPow((1 - udPow((e * udSin(phi1)), 2)), (1 / 2.0));
  r0 = zone.semiMajorAxis * (1 - e * e) / udPow((1 - udPow((e * udSin(phi1)), 2)), (3 / 2.0));
  _a1 = zone.falseEasting - easting;
  dd0 = _a1 / (n0 * zone.scaleFactor);
  t0 = udPow(udTan(phi1), 2);
  Q0 = e1sq * udPow(udCos(phi1), 2);

  fact1 = n0 * udTan(phi1) / r0;
  fact2 = dd0 * dd0 / 2;
  fact3 = (5 + 3 * t0 + 10 * Q0 - 4 * Q0 * Q0 - 9 * e1sq) * udPow(dd0, 4) / 24;
  fact4 = (61 + 90 * t0 + 298 * Q0 + 45 * t0 * t0 - 252 * e1sq - 3 * Q0 * Q0) * udPow(dd0, 6) / 720;

  lof1 = _a1 / (n0 * zone.scaleFactor);
  lof2 = (1 + 2 * t0 + Q0) * udPow(dd0, 3) / 6.0;
  lof3 = (5 - 2 * Q0 + 28 * t0 - 3 * udPow(Q0, 2) + 8 * e1sq + 24 * udPow(t0, 2)) * udPow(dd0, 5) / 120;
  _a2 = (lof1 - lof2 + lof3) / udCos(phi1);

  udDouble3 latLong;
  // Latitude
  latLong.x = 180 * (phi1 - fact1 * (fact2 + fact3 + fact4)) / UD_PI;
  if (zone.hemisphere == 'S')
    latLong.x = -latLong.x;
  latLong.y = zone.meridian - UD_RAD2DEG(_a2);
  latLong.z = position.z;

  return latLong;
}

