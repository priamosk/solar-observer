/*
 * vec3.h - three-dimensional vectors.
 *
 * WHY THIS EXISTS
 *
 * Block C moves from angles to positions in space. The pipeline is:
 *
 *   orbital elements -> position in the orbital plane   (x, y)
 *                    -> heliocentric ecliptic           (x, y, z)
 *                    -> minus Earth's position          (x, y, z)
 *                    -> rotated by obliquity            (x, y, z)
 *
 * Every stage handles three numbers that belong together. Carrying
 * them as separate doubles means writing three lines for every
 * operation and being able to mix up y and z without the compiler
 * noticing.
 *
 * UNITS
 *
 * Positions in this project are in astronomical units. The type does
 * not enforce that - it is a convention documented here and followed
 * everywhere.
 *
 * DESIGN NOTE
 *
 * These functions take and return structs BY VALUE rather than
 * through pointers. A vec3_t is 24 bytes, which fits in registers on
 * both x86-64 and the ESP32's Xtensa core, so copying costs nothing
 * and the code reads as mathematics:
 *
 *     vec3_t geo = vec3_sub(mars, earth);
 *
 * instead of:
 *
 *     vec3_t geo;
 *     vec3_sub(&mars, &earth, &geo);
 *
 * This is the one place in the project where returning by value beats
 * output parameters. The rule from ADR-005 - status returned, data
 * through pointers - exists for functions that can FAIL. None of
 * these can.
 */

#ifndef VEC3_H
#define VEC3_H

/*
 * A point or direction in three-dimensional space.
 *
 * No constructor, no methods - this is C. A plain struct holding
 * three doubles, 24 bytes total.
 */
typedef struct {
    double x;
    double y;
    double z;
} vec3_t;

/* Component-wise addition. */
vec3_t vec3_add(vec3_t a, vec3_t b);

/*
 * Component-wise subtraction: a - b.
 *
 * This is the workhorse of Block C. Converting a heliocentric
 * position (measured from the Sun) to a geocentric one (measured from
 * Earth) is exactly one subtraction:
 *
 *     geocentric = planet_from_sun - earth_from_sun
 */
vec3_t vec3_sub(vec3_t a, vec3_t b);

/* Multiply every component by a number. */
vec3_t vec3_scale(vec3_t v, double factor);

/*
 * Length of the vector.
 *
 * Pythagoras in three dimensions: sqrt(x^2 + y^2 + z^2).
 *
 * Used to get the distance to a planet once its position vector is
 * known.
 */
double vec3_length(vec3_t v);

/*
 * Dot product.
 *
 * Not needed until later phases, but it belongs with the rest of the
 * vector operations. It gives the angle between two directions, which
 * is how the moon-proximity check in Phase 4 will work.
 */
double vec3_dot(vec3_t a, vec3_t b);

#endif /* VEC3_H */