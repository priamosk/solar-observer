/*
 * horizon.c - conversion to altitude and azimuth for an observer.
 */

#include "horizon.h"

#include "angles.h"
#include "equatorial.h"
#include "julian.h"

#include <math.h>
#include <stddef.h>

double gmst(double jd)
{
    /* Days since J2000, including the fraction. */
    double d = jd - JD_J2000;

    /* Greenwich Mean Sidereal Time.
     *
     * The constant 360.98564736629 is the whole point of this
     * function. A solar day is 360 degrees of rotation relative to
     * the Sun, but Earth rotates slightly MORE than that relative to
     * the stars, because it also advances along its orbit.
     *
     * The excess, 0.98564736629 degrees per day, is Earth's orbital
     * motion: 360 degrees spread over 365.25 days. Over a year the
     * extra rotation accumulates to exactly one full turn, which is
     * why a year contains one more sidereal day than solar days.
     *
     * The 280.46061837 offset is the value at the J2000 epoch. */
    double angle = 280.46061837 + 360.98564736629 * d;

    return norm_360(angle);
}

double lst(double jd, double longitude)
{
    /* Longitude is positive east, so an observer east of Greenwich
     * sees any given star cross their meridian earlier - their local
     * sidereal time is ahead.
     *
     * Getting this sign backwards is a common error and produces a
     * position error of 15 degrees per hour of longitude. For Zurich
     * at 8.5 degrees east, that is a 17 degree error - obvious once
     * you look at the sky, invisible in the numbers. */
    return norm_360(gmst(jd) + longitude);
}
int horizon_of(planet_id_t id, double jd, observer_t observer,
               horizontal_t *result)
{
    if (result == NULL) {
        return -1;
    }

    /* Start from equatorial coordinates - same for every observer
     * on Earth. Everything below is what makes them local. */
    equatorial_t eq;
    int status = equatorial_of(id, jd, &eq);

    if (status != 0) {
        return status;
    }

    /* --- Hour angle ---
     *
     * How far the object is from the observer's meridian, the
     * north-south line directly overhead.
     *
     * Local sidereal time IS the right ascension currently crossing
     * that meridian, so subtracting the object's RA gives its
     * distance from it.
     *
     * norm_180 rather than norm_360, because the sign carries
     * meaning: negative means the object has not yet crossed and is
     * still rising in the east, positive means it has crossed and is
     * setting in the west. */
    double H = norm_180(lst(jd, observer.longitude) - eq.right_ascension);

    double H_rad   = deg2rad(H);
    double dec_rad = deg2rad(eq.declination);
    double lat_rad = deg2rad(observer.latitude);

    double sin_dec = sin(dec_rad);
    double cos_dec = cos(dec_rad);
    double sin_lat = sin(lat_rad);
    double cos_lat = cos(lat_rad);
    double cos_H   = cos(H_rad);

    /* --- Altitude ---
     *
     * Spherical trigonometry on the triangle formed by the celestial
     * pole, the zenith, and the object:
     *
     *     sin(alt) = sin(dec)*sin(lat) + cos(dec)*cos(lat)*cos(H)
     *
     * Two terms with clear meanings. The first is how the object's
     * declination relates to the observer's latitude - an object at
     * declination equal to your latitude passes directly overhead.
     * The second is the daily rotation: cos(H) is maximum at the
     * meridian and drops as the object moves away from it. */
    double sin_alt = sin_dec * sin_lat + cos_dec * cos_lat * cos_H;

    /* Clamp before asin. Rounding can push the value a hair beyond
     * the valid [-1, 1] domain, and asin(1.0000000001) returns NaN.
     * NaN then propagates silently through everything downstream and
     * defeats range checks, because any comparison with NaN is
     * false. */
    if (sin_alt > 1.0) {
        sin_alt = 1.0;
    }
    if (sin_alt < -1.0) {
        sin_alt = -1.0;
    }

    result->altitude = rad2deg(asin(sin_alt));

    /* --- Azimuth ---
     *
     * atan2 again, and for the same reason as in equatorial.c: a
     * single ratio cannot distinguish opposite directions.
     *
     * The argument arrangement produces an angle measured from
     * SOUTH, which is the traditional astronomical convention. The
     * +180 below converts it to the navigational convention used by
     * compasses and by this project: measured clockwise from north. */
    double y = sin(H_rad);
    double x = cos_H * sin_lat - (sin_dec / cos_dec) * cos_lat;

    result->azimuth = norm_360(rad2deg(atan2(y, x)) + 180.0);

    result->distance = eq.distance;

    return 0;
}