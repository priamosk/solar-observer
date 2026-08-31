/*
 * equatorial.c - conversion from ecliptic to equatorial coordinates.
 */

#include "equatorial.h"

#include "angles.h"
#include "julian.h"
#include "position.h"
#include "vec3.h"

#include <math.h>
#include <stddef.h>

double obliquity(double jd)
{
    double T = centuries_since_j2000(jd);

    /* Mean obliquity, linear term only.
     *
     * The full expression from the IAU has cubic terms, but they
     * contribute less than an arcsecond over 1800-2050 - far below
     * the few-arcminute accuracy of the orbital element table itself.
     * Adding them would be false precision.
     *
     * The value decreases with time: Earth's tilt is slowly
     * straightening, on a 41000 year cycle. */
    return 23.439291 - 0.0130042 * T;
}

int equatorial_of(planet_id_t id, double jd, equatorial_t *result)
{
    if (result == NULL) {
        return -1;
    }

    /* Start from the geocentric ecliptic position - where the planet
     * is relative to Earth, in Earth's orbital plane. */
    vec3_t ecliptic;
    int status = position_geocentric(id, jd, &ecliptic);

    if (status != 0) {
        return status;
    }

    /* --- Rotate about the x axis by the obliquity ---
     *
     * x is unchanged because it IS the axis of rotation. It points at
     * the vernal equinox, where the two planes intersect, so it is
     * common to both coordinate systems. */
    double eps_rad = deg2rad(obliquity(jd));
    double cos_eps = cos(eps_rad);
    double sin_eps = sin(eps_rad);

    double x = ecliptic.x;
    double y = ecliptic.y * cos_eps - ecliptic.z * sin_eps;
    double z = ecliptic.y * sin_eps + ecliptic.z * cos_eps;

    /* --- Convert the vector into two angles and a distance --- */
    double distance = sqrt(x * x + y * y + z * z);

    /* Guard against division by zero. This happens only for Earth,
     * whose geocentric position is exactly the origin. Without the
     * check, asin(0.0/0.0) produces NaN, which then propagates
     * silently through every subsequent calculation - NaN compared
     * with anything is false, so even range checks stop working. */
    if (distance < 1e-12) {
        return -4;
    }

    /* Declination: the angle above or below the celestial equator.
     * asin returns [-90, 90] degrees, which is exactly the valid
     * range for declination, so no normalisation is needed. */
    result->declination = rad2deg(asin(z / distance));

    /* Right ascension: the angle eastward along the celestial
     * equator.
     *
     * atan2 rather than atan. atan takes a single ratio y/x and
     * cannot distinguish (1,1) from (-1,-1) - both give a ratio of 1,
     * both return 45 degrees, but the second should be 225. atan2
     * takes the two components separately and therefore knows the
     * quadrant.
     *
     * Using atan here would put half the planets on the opposite side
     * of the sky. */
    result->right_ascension = norm_360(rad2deg(atan2(y, x)));

    result->distance = distance;

    return 0;
}