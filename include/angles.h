/*
 * angles.h - angle conversion and normalisation.
 *
 * C's trig functions all take radians. Astronomical data is published in
 * degrees. Mixing the two silently is a guaranteed source of wrong answers,
 * so all conversion lives here in one place.
 */

/* Include guard - see explanation below. */
#ifndef ANGLES_H
#define ANGLES_H

/* Pi. We define our own rather than using M_PI from math.h, because M_PI is
 * not part of the C standard - it comes from POSIX. Under -pedantic it may
 * not be declared at all. Defining it ourselves is portable everywhere. */
#define ANG_PI 3.14159265358979323846

/* Convert degrees to radians. */
double deg2rad(double degrees);

/* Convert radians to degrees. */
double rad2deg(double radians);

/* Normalise an angle in degrees into the range [0, 360). */
double norm_360(double degrees);

/* Normalise an angle in degrees into the range [-180, 180). */
double norm_180(double degrees);

#endif /* ANGLES_H */