/*
 * orbital.h - Keplerian orbital elements for the eight planets.
 *
 * An orbit is an ellipse in space. Describing one completely takes
 * exactly six numbers:
 *
 *   SHAPE OF THE ELLIPSE
 *     a       semi-major axis - the size, in astronomical units
 *     e       eccentricity - 0 is a circle, near 1 is very elongated
 *
 *   ORIENTATION IN SPACE
 *     i       inclination - tilt relative to Earth's orbital plane
 *     node    longitude of ascending node - where the orbit crosses
 *             Earth's plane going north
 *     w_bar   longitude of perihelion - which way the closest point
 *             of the orbit points
 *
 *   POSITION ALONG THE ORBIT
 *     L       mean longitude - where the planet is at a given moment
 *
 * None of these are constants. Gravitational tugging between planets
 * changes them slowly, so each is published as a value at the J2000
 * epoch plus a rate of change per Julian century:
 *
 *     value_now = value_at_J2000 + rate * centuries_since_j2000(jd)
 *
 * The first five drift by fractions of a degree per century. The
 * sixth, mean longitude, is the planet actually moving: Mercury
 * covers 149472 degrees per century, one lap every 88 days.
 *
 * SOURCE: JPL "Approximate Positions of the Planets" (Standish),
 * valid 1800-2050. Accurate to a few arcminutes, far better than
 * needed for pointing binoculars or a small telescope.
 */

#ifndef ORBITAL_H
#define ORBITAL_H

/*
 * Index into the planet table.
 *
 * An enum is a named integer. PLANET_MERCURY is 0, PLANET_VENUS is 1,
 * and so on - the compiler counts automatically. Writing
 * planets[PLANET_MARS] instead of planets[3] means a reader knows what
 * is being looked up, and the compiler catches typos that a bare
 * number never would.
 *
 * PLANET_COUNT sits at the end and takes the value 8. It is not a
 * planet - it is the size of the table. Adding a planet above it
 * updates the count automatically, so loops never go out of date.
 */
typedef enum {
    PLANET_MERCURY = 0,
    PLANET_VENUS,
    PLANET_EARTH,
    PLANET_MARS,
    PLANET_JUPITER,
    PLANET_SATURN,
    PLANET_URANUS,
    PLANET_NEPTUNE,
    PLANET_COUNT
} planet_id_t;

/*
 * One planet's orbital elements, each paired with its rate of change.
 *
 * A struct groups related values into a single named type. Without
 * one we would need twelve separate arrays and would have to keep
 * their indices in step by hand.
 *
 * Units: a in AU, e dimensionless, all angles in degrees, all rates
 * per Julian century.
 */
typedef struct {
    const char *name;

    double a,     a_rate;       /* semi-major axis, AU              */
    double e,     e_rate;       /* eccentricity, dimensionless      */
    double i,     i_rate;       /* inclination, degrees             */
    double L,     L_rate;       /* mean longitude, degrees          */
    double w_bar, w_bar_rate;   /* longitude of perihelion, degrees */
    double node,  node_rate;    /* longitude of ascending node, deg */
} orbital_elements_t;

/*
 * Look up a planet's elements.
 *
 * Returns a pointer to a static const table entry, or NULL if the id
 * is out of range. The caller must not modify what it points to -
 * hence const.
 *
 * Returning a pointer rather than a copy avoids moving 100 bytes of
 * struct on every call. On the ESP32 the table lives in flash, so
 * nothing is copied into RAM at all.
 */
const orbital_elements_t *orbital_get(planet_id_t id);

#endif /* ORBITAL_H */