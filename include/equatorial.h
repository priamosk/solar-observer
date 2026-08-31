/*
 * equatorial.h - equatorial coordinates: right ascension and
 * declination.
 *
 * WHY A SECOND COORDINATE SYSTEM
 *
 * Everything so far has been in ecliptic coordinates, referenced to
 * Earth's orbital plane. Star charts and telescopes use equatorial
 * coordinates, referenced to Earth's EQUATOR projected onto the sky.
 *
 * The reason is rotation. Earth spins, so the sky appears to turn
 * about the celestial poles. An equatorially mounted telescope tracks
 * a star by rotating about one axis parallel to Earth's, and
 * equatorial coordinates match that motion directly.
 *
 * THE OBLIQUITY
 *
 * The two planes differ by about 23.44 degrees - the obliquity of the
 * ecliptic, the tilt of Earth's axis. The same tilt that causes the
 * seasons.
 *
 * It is not constant. It decreases by roughly 47 arcseconds per
 * century:
 *
 *     e = 23.439291 - 0.0130042 * T
 */

#ifndef EQUATORIAL_H
#define EQUATORIAL_H

#include "orbital.h"

/*
 * A direction on the celestial sphere.
 *
 * Distance is carried alongside because it costs nothing to compute
 * and Phase 4 needs it - apparent brightness depends on it.
 */
typedef struct {
    double right_ascension;   /* degrees, [0, 360)   */
    double declination;       /* degrees, [-90, 90]  */
    double distance;          /* AU                  */
} equatorial_t;

/*
 * Obliquity of the ecliptic at a given moment, in degrees.
 */
double obliquity(double jd);

/*
 * Right ascension, declination and distance of a planet as seen from
 * Earth.
 *
 * Returns 0 on success, non-zero on failure.
 */
int equatorial_of(planet_id_t id, double jd, equatorial_t *result);

#endif /* EQUATORIAL_H */