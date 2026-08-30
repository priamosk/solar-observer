/*
 * vec3.c - three-dimensional vector operations.
 */

#include "vec3.h"

#include <math.h>   /* sqrt */

vec3_t vec3_add(vec3_t a, vec3_t b)
{
    /* A compound literal: (vec3_t){ ... } builds a struct inline
     * without needing a named variable first. C99 and later. */
    return (vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z };
}

vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    /* Order matters. This is a - b, not b - a. Getting it backwards
     * would put every planet exactly opposite where it belongs -
     * which looks like a plausible result, not an obvious error. */
    return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z };
}

vec3_t vec3_scale(vec3_t v, double factor)
{
    return (vec3_t){ v.x * factor, v.y * factor, v.z * factor };
}

double vec3_length(vec3_t v)
{
    /* Pythagoras in three dimensions.
     *
     * Note we do NOT guard against overflow here. A more defensive
     * implementation would scale by the largest component before
     * squaring, because for very large values x*x can overflow a
     * double. Our positions are at most ~30 AU, so squaring reaches
     * 900 - nowhere near the limit of about 1.8e308. */
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

double vec3_dot(vec3_t a, vec3_t b)
{
    /* The dot product equals |a| * |b| * cos(theta), where theta is
     * the angle between the two vectors. Dividing by the two lengths
     * and taking acos gives that angle - which is how the
     * moon-proximity check in Phase 4 will work. */
    return a.x * b.x + a.y * b.y + a.z * b.z;
}