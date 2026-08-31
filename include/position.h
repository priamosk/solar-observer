/*
 * position.h - heliocentric position of a planet.
 *
 * Turns the six orbital elements into a point in space, measured from
 * the Sun, in astronomical units.
 *
 * The pipeline, all of which is already built except the last two
 * steps:
 *
 *   elements at J2000 + rates
 *          |  centuries_since_j2000()
 *   elements for the requested instant
 *          |  M = L - w_bar
 *   mean anomaly
 *          |  kepler_solve()
 *   eccentric anomaly E
 *          |  ellipse geometry
 *   position in the orbital plane (x, y)
 *          |  three rotations
 *   heliocentric ecliptic coordinates (x, y, z)
 *
 * See docs/BACKGROUND.md for the derivation.
 */

#ifndef POSITION_H
#define POSITION_H

#include "orbital.h"
#include "vec3.h"

/*
 * Compute a planet's heliocentric ecliptic position.
 *
 *   id      which planet
 *   jd      Julian Date of the moment wanted
 *   result  output parameter; receives the position in AU
 *
 * Returns 0 on success, non-zero on failure.
 *
 * Failure modes: an out-of-range planet id, a NULL result pointer, or
 * a Kepler solver that did not converge. The status code
 * distinguishes them so a caller can tell a programming mistake from
 * a numerical one.
 */
int position_heliocentric(planet_id_t id, double jd, vec3_t *result);
/*
 * Compute a planet's geocentric ecliptic position.
 *
 * Same coordinate system as position_heliocentric, but measured from
 * Earth rather than from the Sun. This is what an observer actually
 * needs: the direction from where they are standing to the planet.
 *
 * The conversion is one vector subtraction:
 *
 *     geocentric = planet_from_sun - earth_from_sun
 *
 * It works because both positions are already in the same frame -
 * same axes, same units, same epoch. Subtracting simply moves the
 * origin from the Sun to the Earth.
 *
 *   id      which planet
 *   jd      Julian Date of the moment wanted
 *   result  output parameter; receives the position in AU
 *
 * Returns 0 on success, non-zero on failure.
 *
 * NOTE: passing PLANET_EARTH returns (0, 0, 0), which is correct but
 * useless - Earth is at no distance from itself. Callers displaying a
 * planet list should skip it.
 */
int position_geocentric(planet_id_t id, double jd, vec3_t *result);
#endif /* POSITION_H */