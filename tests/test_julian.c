/*
 * test_julian.c - unit tests for Julian Date conversion.
 *
 * The reference values below are published Julian Dates, not values
 * this code produced. Testing code against its own output proves
 * nothing. Each value here can be checked independently against any
 * astronomical almanac.
 */

#include <stdio.h>
#include <math.h>

#include "julian.h"

/* Tolerance in days. 1e-6 days is about 0.09 seconds - far tighter
 * than this project needs, so a failure means a real bug rather than
 * accumulated rounding. */
#define TOLERANCE 1e-6

static int checks_run = 0;
static int checks_failed = 0;

static void check(double actual, double expected, const char *name)
{
    checks_run++;

    double difference = fabs(actual - expected);

    if (difference > TOLERANCE) {
        printf("  FAIL  %s\n", name);
        printf("        expected %.10f, got %.10f\n", expected, actual);
        checks_failed++;
    }
}
/*
 * This file has its own main, separate from src/main.c and from
 * tests/test_angles.c. Each becomes a distinct program. They never
 * link together, so there is never more than one main per binary.
 */
int main(void)
{
    printf("J2000.0 epoch\n");

    /* THE critical test. J2000 is defined as exactly this instant and
     * exactly this Julian Date. Every orbital element in the project
     * is referenced to it. If this one value is wrong, every planet
     * position will be wrong, and no correct code downstream can
     * save it.
     *
     * Note this is 12:00 UTC, not midnight - the Julian day starts
     * at noon. */
    check(jd_from_utc(2000, 1, 1, 12, 0, 0.0),
          2451545.0, "J2000.0 is exactly JD 2451545.0");

    /* Midnight on the same day is half a day earlier, landing on a
     * .5 boundary. Every midnight UTC does this. */
    check(jd_from_utc(2000, 1, 1, 0, 0, 0.0),
          2451544.5, "midnight UTC falls on a .5 boundary");

    printf("published reference dates\n");

    check(jd_from_utc(1900, 1, 1, 0, 0, 0.0),
          2415020.5, "start of 1900");

    check(jd_from_utc(2026, 8, 29, 0, 0, 0.0),
          2461281.5, "29 August 2026");

    printf("time-of-day fraction\n");

    /* This group exists to catch one specific bug: integer division
     * in Step 4. If hour / 24 were computed with ints, every one of
     * these would collapse to the same midnight value. */
    double base = jd_from_utc(2026, 1, 1, 0, 0, 0.0);

    check(jd_from_utc(2026, 1, 1, 6, 0, 0.0) - base,
          0.25, "06:00 is a quarter day");

    check(jd_from_utc(2026, 1, 1, 12, 0, 0.0) - base,
          0.5, "12:00 is half a day");

    check(jd_from_utc(2026, 1, 1, 18, 0, 0.0) - base,
          0.75, "18:00 is three quarters");

    check(jd_from_utc(2026, 1, 1, 0, 1, 0.0) - base,
          1.0 / 1440.0, "one minute");

    check(jd_from_utc(2026, 1, 1, 0, 0, 1.0) - base,
          1.0 / 86400.0, "one second");

    printf("leap year handling\n");

    /* 2000 was a leap year (divisible by 400), so it spans 366 days. */
    double y2000 = jd_from_utc(2000, 1, 1, 0, 0, 0.0);
    double y2001 = jd_from_utc(2001, 1, 1, 0, 0, 0.0);
    check(y2001 - y2000, 366.0, "2000 was a leap year");

    /* 1900 was NOT a leap year (divisible by 100 but not 400), so it
     * spans 365 days. A naive "divisible by 4" implementation gets
     * this wrong, which is exactly why this test exists. */
    double y1900 = jd_from_utc(1900, 1, 1, 0, 0, 0.0);
    double y1901 = jd_from_utc(1901, 1, 1, 0, 0, 0.0);
    check(y1901 - y1900, 365.0, "1900 was not a leap year");

    /* Crossing the February/March boundary in a leap year exercises
     * the month-shift logic from Step 1. */
    double feb28 = jd_from_utc(2024, 2, 28, 0, 0, 0.0);
    double mar01 = jd_from_utc(2024, 3, 1, 0, 0, 0.0);
    check(mar01 - feb28, 2.0, "2024 had a 29 February");

    printf("centuries_since_j2000\n");

    check(centuries_since_j2000(JD_J2000), 0.0,
          "epoch is zero centuries");

    check(centuries_since_j2000(JD_J2000 + 36525.0), 1.0,
          "36525 days is one Julian century");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}