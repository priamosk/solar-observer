/*
 * test_orbital.c - checks on the planet element table.
 *
 * This suite does not verify the numbers are correct - they come from
 * JPL and are correct by definition. It verifies the TABLE is right:
 * that entries sit in the expected slots, that no row was mistyped,
 * and that the bounds check works.
 *
 * Typos in a data table are the easiest bug in the project to make
 * and the hardest to spot by eye. A misplaced decimal point in
 * Saturn's semi-major axis compiles, runs, and puts Saturn in the
 * wrong place forever.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>   /* strcmp */

#include "orbital.h"

static int checks_run = 0;
static int checks_failed = 0;

static void check(double actual, double expected, double tolerance,
                  const char *name)
{
    checks_run++;

    if (fabs(actual - expected) > tolerance) {
        printf("  FAIL  %s\n", name);
        printf("        expected %.8f, got %.8f\n", expected, actual);
        checks_failed++;
    }
}

/* A separate helper for conditions that are simply true or false,
 * where there is no numeric tolerance to apply. */
static void check_true(int condition, const char *name)
{
    checks_run++;

    if (!condition) {
        printf("  FAIL  %s\n", name);
        checks_failed++;
    }
}
int main(void)
{
    printf("table access\n");

    /* THE roadmap acceptance criterion for B1. */
    const orbital_elements_t *earth = orbital_get(PLANET_EARTH);
    check_true(earth != NULL, "Earth entry exists");
    check(earth->a, 1.00000261, 1e-8, "Earth semi-major axis is 1 AU");

    /* strcmp returns 0 when two strings are identical. Non-zero
     * otherwise. It does NOT return true/false in the usual sense,
     * which trips people up constantly. */
    check_true(strcmp(earth->name, "Earth") == 0,
               "Earth is in the Earth slot");

    /* Confirm the enum order matches the table order. If a row were
     * inserted or deleted, every index below would shift and these
     * would catch it. */
    check_true(strcmp(orbital_get(PLANET_MERCURY)->name, "Mercury") == 0,
               "Mercury in slot 0");
    check_true(strcmp(orbital_get(PLANET_NEPTUNE)->name, "Neptune") == 0,
               "Neptune in slot 7");

    printf("bounds checking\n");

    /* Out-of-range ids must return NULL rather than reading past the
     * end of the array. */
    check_true(orbital_get(PLANET_COUNT) == NULL,
               "PLANET_COUNT is rejected");
    check_true(orbital_get((planet_id_t)-1) == NULL,
               "negative id is rejected");
    check_true(orbital_get((planet_id_t)99) == NULL,
               "large id is rejected");

    printf("physical sanity\n");

    /* Distances must increase outward. A transposed row or a
     * mistyped exponent would break this ordering. */
    for (int i = 1; i < PLANET_COUNT; i++) {
        const orbital_elements_t *inner = orbital_get((planet_id_t)(i - 1));
        const orbital_elements_t *outer = orbital_get((planet_id_t)i);

        check_true(outer->a > inner->a,
                   "each planet is farther out than the previous");
    }

    /* Eccentricity must lie in [0, 1). Exactly 0 is a circle; 1 or
     * above is a parabola or hyperbola, meaning an unbound orbit -
     * which would not be a planet. A misplaced decimal point in this
     * column would very likely land outside the range. */
    for (int i = 0; i < PLANET_COUNT; i++) {
        const orbital_elements_t *p = orbital_get((planet_id_t)i);

        check_true(p->e >= 0.0 && p->e < 1.0,
                   "eccentricity is a valid closed orbit");
    }

    /* Every planet must move forward. A negative or zero rate would
     * mean a planet orbiting backwards or standing still. */
    for (int i = 0; i < PLANET_COUNT; i++) {
        const orbital_elements_t *p = orbital_get((planet_id_t)i);

        check_true(p->L_rate > 0.0, "mean longitude advances");
    }

    /* Inner planets orbit faster than outer ones - Kepler's third
     * law. Mercury's rate must exceed Neptune's by a wide margin. */
    check_true(orbital_get(PLANET_MERCURY)->L_rate >
               orbital_get(PLANET_NEPTUNE)->L_rate * 100.0,
               "Mercury orbits far faster than Neptune");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}