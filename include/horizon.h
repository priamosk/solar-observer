/*
 * horizon.h - horizontal coordinates: altitude and azimuth.
 *
 * The final step. Right ascension and declination are the same for
 * every observer on Earth - Mars has identical RA/Dec whether you are
 * in Zurich or Tokyo. What they do not tell you is where to look.
 *
 * Altitude and azimuth do. They are what turns "Mars is at RA 14h
 * 32m" into "look 38 degrees above the horizon, toward the
 * south-east".
 *
 * SIDEREAL TIME
 *
 * Your clock measures solar time: 24 hours from noon to noon. But
 * Earth does not take 24 hours to rotate once. It takes 23 hours 56
 * minutes 4 seconds.
 *
 * The difference exists because Earth is also moving along its orbit.
 * After one full rotation it must turn slightly further to face the
 * Sun again. For stars this does not apply - they are effectively
 * infinitely far away, so one rotation is enough.
 *
 * This is sidereal time, and it is what the sky actually follows. It
 * gains almost four minutes per day, which is why stars rise four
 * minutes earlier each night and why the night sky changes with the
 * seasons.
 */

#ifndef HORIZON_H
#define HORIZON_H

#include "orbital.h"

/*
 * An observer's position on Earth.
 *
 * Sign conventions follow the usual geographic ones:
 *   latitude   positive north, negative south, [-90, 90]
 *   longitude  positive east, negative west, [-180, 180]
 *
 * Zurich is approximately (47.37, 8.54).
 * New York is approximately (40.71, -74.01) - note the negative.
 */
typedef struct {
    double latitude;    /* degrees */
    double longitude;   /* degrees */
} observer_t;

/*
 * Where to look.
 *
 * altitude  degrees above the horizon. Negative means below it -
 *           the object has set and is not visible.
 * azimuth   degrees clockwise from north: 0 is north, 90 east,
 *           180 south, 270 west.
 */
typedef struct {
    double altitude;    /* degrees, [-90, 90]  */
    double azimuth;     /* degrees, [0, 360)   */
    double distance;    /* AU                  */
} horizontal_t;

/*
 * Greenwich Mean Sidereal Time, in degrees.
 *
 * The rotation angle of Earth relative to the stars, as measured at
 * the Greenwich meridian.
 */
double gmst(double jd);

/*
 * Local Sidereal Time, in degrees.
 *
 * GMST adjusted for the observer's longitude. Numerically this equals
 * the right ascension currently crossing the observer's meridian -
 * the line running due north-south overhead.
 */
double lst(double jd, double longitude);

/*
 * Altitude and azimuth of a planet for a specific observer at a
 * specific moment.
 *
 * Returns 0 on success, non-zero on failure.
 *
 * NOTE: this is the geometric position. It does not correct for
 * atmospheric refraction, which lifts objects near the horizon by up
 * to about half a degree. See docs/BACKGROUND.md.
 */
int horizon_of(planet_id_t id, double jd, observer_t observer,
               horizontal_t *result);

#endif /* HORIZON_H */