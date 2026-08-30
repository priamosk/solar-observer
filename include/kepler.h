/*
 * kepler.h - solver for Kepler's equation.
 *
 * THE PROBLEM
 *
 * Kepler's second law says a planet sweeps equal areas in equal
 * times, so it moves faster near the Sun and slower far from it.
 *
 * Mean anomaly M describes an imaginary planet moving at constant
 * speed around a circle, completing one lap in the same period as the
 * real one. It is time disguised as an angle, and it comes straight
 * out of the element table: M = L - w_bar.
 *
 * Eccentric anomaly E is the geometric bridge to the planet's real
 * position. The two are related by:
 *
 *     M = E - e * sin(E)
 *
 * We know M and e. We need E. And there is no formula.
 *
 * The equation is transcendental: E appears both linearly and inside
 * a sine, and no algebraic manipulation separates them. Kepler
 * struggled with this from 1609; in 1900 Bruns proved no closed-form
 * solution in elementary functions exists. It is not undiscovered -
 * it is impossible.
 *
 * THE SOLUTION
 *
 * Newton-Raphson iteration. Rewrite the equation so it equals zero:
 *
 *     f(E)  = E - e * sin(E) - M
 *     f'(E) = 1 - e * cos(E)
 *
 * Guess an E, measure how wrong it is, follow the tangent line to
 * where it crosses zero, repeat:
 *
 *     E_next = E - f(E) / f'(E)
 *
 * Convergence is quadratic - the number of correct digits doubles
 * each step - so three or four iterations exhaust double precision.
 */

#ifndef KEPLER_H
#define KEPLER_H

/*
 * Hard ceiling on iterations.
 *
 * Four are typically enough. Thirty is a safety net, not a working
 * value.
 *
 * In embedded code an unbounded loop does not merely run slowly - it
 * hangs the device. There is no Ctrl+C on a microcontroller. The
 * screen freezes, the watchdog reboots the chip, and the user sees a
 * device that restarts for no visible reason. Every loop in this
 * project has a provable upper bound.
 */
#define KEPLER_MAX_ITERATIONS 30

/*
 * Convergence tolerance, in radians.
 *
 * 1e-12 radians is about 0.0000002 arcseconds - far below anything
 * observable, and near the limit of what a double can represent for
 * values around 1.
 */
#define KEPLER_TOLERANCE 1e-12

/*
 * Solve Kepler's equation for eccentric anomaly.
 *
 *   mean_anomaly   M, in RADIANS (not degrees)
 *   eccentricity   e, must be in [0, 1)
 *   result         output parameter; receives E in radians
 *
 * Returns 0 on success, non-zero on failure.
 *
 * WHY A RETURN CODE AND AN OUTPUT PARAMETER
 *
 * The function has two things to report: the answer, and whether it
 * succeeded. C returns one value, so one of them goes through a
 * pointer.
 *
 * The status is the return value rather than the answer because that
 * makes it awkward to ignore. Writing `E = solve(M, e)` and silently
 * using a garbage E is easy; `solve(M, e, &E)` without checking the
 * result at least looks wrong.
 *
 * This pattern - status returned, data through pointers - is standard
 * in embedded C for exactly this reason.
 */
int kepler_solve(double mean_anomaly, double eccentricity, double *result);

#endif /* KEPLER_H */