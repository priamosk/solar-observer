/*
 * angles.c - implementation of the helpers declared in angles.h.
 */

#include "angles.h"

#include <math.h>   /* fmod */

double deg2rad(double degrees)
{
    /* A full circle is 360 degrees or 2*pi radians,
     * so one degree is pi/180 radians. */
    return degrees * (ANG_PI / 180.0);
}

double rad2deg(double radians)
{
    /* The inverse. */
    return radians * (180.0 / ANG_PI);
}

double norm_360(double degrees)
{
    /* fmod returns the remainder of degrees / 360.
     *
     * The catch: C's fmod keeps the sign of the DIVIDEND, not the divisor.
     * So fmod(-10.0, 360.0) gives -10.0, not 350.0. Python's % operator
     * returns 350, so formulas copied from Python pseudocode break here. */
    double result = fmod(degrees, 360.0);

    /* Shift any negative result up by a full turn to land in [0, 360). */
    if (result < 0.0) {
        result += 360.0;
    }

    return result;
}

double norm_180(double degrees)
{
    /* First collapse into [0, 360). This handles arbitrarily large
     * inputs and negatives in one step. */
    double result = norm_360(degrees);

    /* Anything at or above 180 belongs in the negative half.
     * Subtracting a full turn gives the equivalent angle:
     * 190 becomes -170, 350 becomes -10, 180 becomes -180. */
    if (result >= 180.0) {
        result -= 360.0;
    }

    return result;
}