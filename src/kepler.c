/*
 * kepler.c - Newton-Raphson solver for Kepler's equation.
 *
 * See kepler.h for the mathematics and docs/BACKGROUND.md for the
 * full derivation.
 */

#include "kepler.h"

#include <math.h>     /* sin, cos, fabs */
#include <stddef.h>   /* NULL */

int kepler_solve(double mean_anomaly, double eccentricity, double *result)
{
    /* --- Validate inputs before doing any work ---
     *
     * A NULL output pointer would crash on the first write. Checking
     * costs one comparison and converts a segfault into a return
     * code the caller can handle.
     *
     * Eccentricity outside [0, 1) is not a closed orbit. Negative is
     * meaningless; 1 or above is a parabola or hyperbola - an object
     * leaving the solar system, for which this equation does not
     * apply at all. Feeding such a value in would either diverge or
     * silently return nonsense. */
    if (result == NULL) {
        return -1;
    }

    if (eccentricity < 0.0 || eccentricity >= 1.0) {
        return -2;
    }
        /* --- Initial guess ---
     *
     * Starting from E = M works well because for small eccentricity
     * the correction term e*sin(E) is small, so E is already close to
     * M. For Earth (e = 0.0167) the first guess is within 0.9 degrees
     * of the answer. Even for Mercury (e = 0.2056) it is close enough
     * that convergence takes four iterations instead of three. */
    double E = mean_anomaly;

    /* --- Iterate ---
     *
     * The loop counter is what makes this bounded. See ADR-009: an
     * unbounded loop on a microcontroller is a hang, not a slowdown. */
    for (int iteration = 0; iteration < KEPLER_MAX_ITERATIONS; iteration++) {

        /* f(E) = E - e*sin(E) - M
         *
         * This is Kepler's equation rearranged to equal zero. Its
         * value IS the current error: how far this guess is from
         * satisfying the equation. */
        double f = E - eccentricity * sin(E) - mean_anomaly;

        /* f'(E) = 1 - e*cos(E)
         *
         * The derivative gives the slope of f at the current guess,
         * which is the direction and steepness to follow.
         *
         * This can never reach zero for our planets: the largest
         * eccentricity in the table is Mercury's 0.2056, so the
         * derivative stays above 0.79. */
        double f_prime = 1.0 - eccentricity * cos(E);

        /* Follow the tangent line to where it crosses zero. That
         * intersection is the next, better guess. */
        double delta = f / f_prime;

        E = E - delta;

        /* Stop when the correction becomes negligible.
         *
         * We test the size of the STEP rather than the size of f.
         * A tiny step means we have stopped moving, which is a
         * direct statement about convergence. Testing f would tell
         * us the residual is small, which is related but less
         * direct.
         *
         * fabs is needed because delta can be negative - we care
         * about magnitude, not direction. */
        if (fabs(delta) < KEPLER_TOLERANCE) {
            *result = E;
            return 0;
        }
    }

    /* --- Did not converge ---
     *
     * Reaching here means 30 iterations passed without the step
     * falling below tolerance. For planetary eccentricities this
     * should be unreachable.
     *
     * We deliberately do NOT write to *result. The caller gets a
     * failure code and an untouched variable, rather than a
     * plausible-looking number that happens to be wrong. Silent bad
     * data is worse than an obvious failure. */
    return -3;
}