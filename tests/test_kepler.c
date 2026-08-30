/*
 * test_kepler.c - tests for the Kepler equation solver.
 *
 * VERIFICATION STRATEGY
 *
 * We cannot compare E against a known-correct value, because there is
 * no formula that produces one - that is precisely why the solver
 * exists.
 *
 * Instead we substitute the answer back into the original equation:
 *
 *     residual = E - e * sin(E) - M
 *
 * If the residual is essentially zero, E satisfies Kepler's equation
 * and is therefore correct. This is inversion checking, and it is
 * stronger than a table of precomputed values: it works for every
 * input, not only the ones someone thought to tabulate.
 */

#include <stdio.h>
#include <math.h>

#include "kepler.h"
#include "angles.h"

static int checks_run = 0;
static int checks_failed = 0;

static void check_true(int condition, const char *name)
{
    checks_run++;

    if (!condition) {
        printf("  FAIL  %s\n", name);
        checks_failed++;
    }
}

/*
 * Solve for E, then verify it satisfies the original equation.
 *
 * Residual tolerance is 1e-10 rather than the solver's 1e-12: the
 * solver stops when the STEP is below 1e-12, and the residual after
 * that step is a slightly different quantity. 1e-10 radians is about
 * 0.00002 arcseconds - still far below anything observable.
 */
static void check_solves(double M, double e, const char *name)
{
    checks_run++;

    double E = 0.0;
    int status = kepler_solve(M, e, &E);

    if (status != 0) {
        printf("  FAIL  %s\n", name);
        printf("        solver returned status %d\n", status);
        checks_failed++;
        return;
    }

    double residual = E - e * sin(E) - M;

    if (fabs(residual) > 1e-10) {
        printf("  FAIL  %s\n", name);
        printf("        M=%.6f e=%.6f E=%.6f residual=%.3e\n",
               M, e, E, residual);
        checks_failed++;
    }
}
int main(void)
{
    printf("circular orbits\n");

    /* With e = 0 the equation collapses to M = E, so the solver
     * should return exactly the input. This is the one case where we
     * DO know the answer in advance. */
    double E = 0.0;
    kepler_solve(1.0, 0.0, &E);
    check_true(fabs(E - 1.0) < 1e-12, "e=0 returns E=M");

    kepler_solve(3.0, 0.0, &E);
    check_true(fabs(E - 3.0) < 1e-12, "e=0 returns E=M again");

    printf("real planetary eccentricities\n");

    /* Values taken from the element table. If the solver handles
     * these, it handles every planet in the project. */
    check_solves(1.0, 0.20563593, "Mercury eccentricity");
    check_solves(1.0, 0.00677672, "Venus eccentricity");
    check_solves(1.0, 0.01671123, "Earth eccentricity");
    check_solves(1.0, 0.09339410, "Mars eccentricity");
    check_solves(1.0, 0.04838624, "Jupiter eccentricity");

    printf("angles across the full orbit\n");

    /* Sweep M through a complete revolution. Perihelion (M=0) and
     * aphelion (M=pi) are where the correction term behaves most
     * differently, so both must be covered. */
    for (int degrees = 0; degrees < 360; degrees += 15) {
        check_solves(deg2rad((double)degrees), 0.2056,
                     "full sweep at Mercury eccentricity");
    }

    printf("negative and large mean anomalies\n");

    /* M is not normalised before being passed in. The solver must
     * cope with values outside [0, 2pi). */
    check_solves(-1.0,  0.1, "negative M");
    check_solves(-5.0,  0.1, "large negative M");
    check_solves(10.0,  0.1, "M beyond one revolution");
    check_solves(100.0, 0.1, "M far beyond one revolution");

    printf("high eccentricity\n");

    /* Well beyond any planet, into comet territory. Newton-Raphson
     * gets slower here but must still converge within the iteration
     * cap. */
    check_solves(1.0, 0.5,  "e=0.5");
    check_solves(1.0, 0.8,  "e=0.8");
    check_solves(1.0, 0.95, "e=0.95");

    printf("input validation\n");

    /* A NULL output pointer must be rejected, not dereferenced. */
    check_true(kepler_solve(1.0, 0.1, NULL) != 0,
               "NULL result pointer rejected");

    /* Eccentricity outside [0, 1) is not a closed orbit. */
    check_true(kepler_solve(1.0, -0.1, &E) != 0,
               "negative eccentricity rejected");
    check_true(kepler_solve(1.0, 1.0, &E) != 0,
               "eccentricity of exactly 1 rejected");
    check_true(kepler_solve(1.0, 1.5, &E) != 0,
               "eccentricity above 1 rejected");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}