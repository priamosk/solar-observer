/*
 * position.c - heliocentric position from orbital elements.
 *
 * See docs/BACKGROUND.md for the derivation of the ellipse geometry
 * and the three rotations.
 */

#include "position.h"

#include "angles.h"
#include "julian.h"
#include "kepler.h"

#include <math.h>
#include <stddef.h>

int position_heliocentric(planet_id_t id, double jd, vec3_t *result)
{
    if (result == NULL) {
        return -1;
    }

    const orbital_elements_t *el = orbital_get(id);

    if (el == NULL) {
        return -2;
    }

    /* --- Step 1: bring the elements forward to the requested time ---
     *
     * Every element is published as a value at J2000 plus a rate of
     * change per Julian century. This is where that pairing is used.
     *
     * T is negative for dates before 2000, which works without any
     * special handling - the rates simply run backwards. */
    double T = centuries_since_j2000(jd);

    double a     = el->a     + el->a_rate     * T;   /* AU      */
    double e     = el->e     + el->e_rate     * T;   /* -       */
    double i     = el->i     + el->i_rate     * T;   /* degrees */
    double L     = el->L     + el->L_rate     * T;   /* degrees */
    double w_bar = el->w_bar + el->w_bar_rate * T;   /* degrees */
    double node  = el->node  + el->node_rate  * T;   /* degrees */

        /* --- Step 2: mean anomaly ---
     *
     * M is the angle of the fictitious uniformly-moving planet,
     * measured from perihelion. L is measured from the vernal
     * equinox, and w_bar points at perihelion, so the difference is
     * the angle from perihelion.
     *
     * Normalising to [-180, 180) matters here. L can be tens of
     * thousands of degrees - Mercury has completed 111 orbits since
     * J2000 - and while sin and cos handle large arguments, they lose
     * precision doing so. Bringing M into a small range first keeps
     * the trigonometry accurate.
     *
     * We use norm_180 rather than norm_360 because the Kepler solver
     * starts its guess at E = M, and starting near zero converges
     * marginally faster than starting near 360. */
    double M = norm_180(L - w_bar);

    /* --- Step 3: solve Kepler's equation ---
     *
     * The solver works in radians. Everything above is in degrees, so
     * this is the boundary where units change.
     *
     * Note the naming convention from angles.h: variables holding
     * radians carry a _rad suffix. Mixing units silently is the
     * easiest way to get a plausible-looking wrong answer. */
    double M_rad = deg2rad(M);
    double E_rad = 0.0;

    int status = kepler_solve(M_rad, e, &E_rad);

    if (status != 0) {
        /* Propagate the failure rather than masking it. The caller
         * gets -3 and an untouched result, not a plausible number
         * that happens to be wrong. */
        return -3;
    }
        /* --- Step 4: position in the orbital plane ---
     *
     * The orbit as a flat ellipse, Sun at the focus, perihelion on
     * the positive x axis.
     *
     * The -e term shifts the origin from the centre of the ellipse
     * (where E is measured from) to the focus (where the Sun is).
     *
     * The sqrt(1 - e*e) factor is the ratio of semi-minor to
     * semi-major axis. It squashes the circumscribed circle of
     * radius a into the actual ellipse. */
    double x_orbital = a * (cos(E_rad) - e);
    double y_orbital = a * sqrt(1.0 - e * e) * sin(E_rad);

    /* --- Step 5: rotate into the ecliptic frame ---
     *
     * The element table gives w_bar, longitude of perihelion. The
     * rotation needs w, argument of perihelion, measured from the
     * ascending node rather than from the vernal equinox. */
    double w = w_bar - node;

    double w_rad    = deg2rad(w);
    double i_rad    = deg2rad(i);
    double node_rad = deg2rad(node);

    /* Compute each sine and cosine once. Each appears two or three
     * times in the formulas below, and trigonometric functions are
     * among the slowest operations available - especially on the
     * ESP32, where double-precision maths is emulated in software. */
    double cos_w    = cos(w_rad);
    double sin_w    = sin(w_rad);
    double cos_i    = cos(i_rad);
    double sin_i    = sin(i_rad);
    double cos_node = cos(node_rad);
    double sin_node = sin(node_rad);

    /* Three rotations - by w about z, by i about x, by node about z -
     * multiplied out into their combined form.
     *
     * The order is not interchangeable. Each angle is defined
     * relative to the frame the previous rotations produce, so they
     * apply innermost first. See docs/BACKGROUND.md. */
    result->x = x_orbital * (cos_w * cos_node - sin_w * sin_node * cos_i)
              - y_orbital * (sin_w * cos_node + cos_w * sin_node * cos_i);

    result->y = x_orbital * (cos_w * sin_node + sin_w * cos_node * cos_i)
              - y_orbital * (sin_w * sin_node - cos_w * cos_node * cos_i);

    result->z = x_orbital * (sin_w * sin_i)
              + y_orbital * (cos_w * sin_i);

    return 0;
    
}

int position_geocentric(planet_id_t id, double jd, vec3_t *result)
{
    if (result == NULL) {
        return -1;
    }

    vec3_t planet;
    vec3_t earth;

    /* Both lookups can fail - an invalid id for the first, a
     * non-converging solver for either. Check each rather than
     * assuming success. */
    int status = position_heliocentric(id, jd, &planet);

    if (status != 0) {
        return status;
    }

    status = position_heliocentric(PLANET_EARTH, jd, &earth);

    if (status != 0) {
        return status;
    }

    /* Order matters: planet minus Earth, not Earth minus planet.
     * Reversing it would place every object exactly opposite where it
     * belongs - a result that looks entirely plausible. */
    *result = vec3_sub(planet, earth);

    return 0;
}