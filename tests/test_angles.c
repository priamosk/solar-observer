/*
 * test_angles.c - unit tests for the angle helpers.
 */

#include <stdio.h>   /* printf */
#include <math.h>    /* fabs   */

#include "angles.h"

/* How close two doubles must be to count as equal.
 * 1e-9 means 0.000000001 */
#define TOLERANCE 1e-9

/* Counters. Declared outside any function, so every function in this
 * file can see and modify them. */
static int checks_run = 0;
static int checks_failed = 0;
/*
 * Compare one result against its expected value.
 *
 * We never compare doubles with ==, because binary floating point
 * cannot represent most decimal values exactly. Instead we check that
 * the absolute difference is smaller than TOLERANCE.
 */
static void check(double actual, double expected, const char *name)
{
    checks_run++;

    /* fabs = floating-point absolute value. Turns -0.5 into 0.5,
     * so we measure distance regardless of direction. */
    double difference = fabs(actual - expected);

    if (difference > TOLERANCE) {
        printf("  FAIL  %s\n", name);
        printf("        expected %.10f, got %.10f\n", expected, actual);
        checks_failed++;
    }
}

int main(void)
{
    printf("deg2rad\n");
    check(deg2rad(0.0),     0.0,          "zero");
    check(deg2rad(180.0),   ANG_PI,       "180 degrees is pi");
    check(deg2rad(90.0),    ANG_PI / 2.0, "90 degrees is pi/2");

    printf("rad2deg\n");
    check(rad2deg(ANG_PI),  180.0, "pi is 180 degrees");
    check(rad2deg(deg2rad(37.5)), 37.5, "round trip");

    printf("norm_360\n");
    check(norm_360(0.0),    0.0,   "zero unchanged");
    check(norm_360(360.0),  0.0,   "360 wraps to 0");
    check(norm_360(730.0),  10.0,  "two turns plus 10");
    check(norm_360(-10.0),  350.0, "small negative");
    check(norm_360(-370.0), 350.0, "negative beyond one turn");

    printf("norm_180\n");
    check(norm_180(90.0),   90.0,   "below 180 unchanged");
    check(norm_180(180.0), -180.0,  "180 folds to -180");
    check(norm_180(190.0), -170.0,  "just above 180");
    check(norm_180(350.0),  -10.0,  "near a full turn");
    check(norm_180(-190.0), 170.0,  "large negative");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    /* Exit code: 0 means success, anything else means failure.
     * The shell and later the Makefile read this number. */
    return checks_failed;
}