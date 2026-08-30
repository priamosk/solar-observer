/*
 * test_vec3.c - tests for three-dimensional vector operations.
 *
 * These functions are short enough that a bug would be a typo rather
 * than a logic error - swapping y and z, or getting subtraction
 * backwards. The tests are built to catch exactly that.
 *
 * Note the asymmetric test values throughout. Using (1,1,1) or
 * (2,2,2) would let a component swap pass unnoticed, because every
 * component is identical. Distinct values in every slot make a swap
 * visible.
 */

#include <stdio.h>
#include <math.h>

#include "vec3.h"

#define TOLERANCE 1e-12

static int checks_run = 0;
static int checks_failed = 0;

static void check(double actual, double expected, const char *name)
{
    checks_run++;

    if (fabs(actual - expected) > TOLERANCE) {
        printf("  FAIL  %s\n", name);
        printf("        expected %.12f, got %.12f\n", expected, actual);
        checks_failed++;
    }
}

/*
 * Verify all three components of a vector at once.
 *
 * Counts as three checks, not one, because each component is an
 * independent opportunity for a typo.
 */
static void check_vec(vec3_t v, double x, double y, double z,
                      const char *name)
{
    check(v.x, x, name);
    check(v.y, y, name);
    check(v.z, z, name);
}
int main(void)
{
    /* Asymmetric values in every component. If vec3_add wrote b.y
     * into the x slot, these would catch it; (1,1,1) would not. */
    vec3_t a = { 1.0, 2.0, 3.0 };
    vec3_t b = { 4.0, 5.0, 6.0 };

    printf("addition\n");

    check_vec(vec3_add(a, b), 5.0, 7.0, 9.0, "a + b");

    /* Addition is commutative - order must not matter. */
    check_vec(vec3_add(b, a), 5.0, 7.0, 9.0, "b + a gives the same");

    /* Adding the zero vector must change nothing. */
    vec3_t zero = { 0.0, 0.0, 0.0 };
    check_vec(vec3_add(a, zero), 1.0, 2.0, 3.0, "adding zero is identity");

    printf("subtraction\n");

    /* THE critical direction test. vec3_sub(a, b) must be a - b.
     * Getting this backwards would place every planet exactly
     * opposite where it belongs - a result that looks entirely
     * plausible rather than obviously broken. */
    check_vec(vec3_sub(b, a), 3.0, 3.0, 3.0, "b - a is positive");
    check_vec(vec3_sub(a, b), -3.0, -3.0, -3.0, "a - b is negative");

    /* Subtracting a vector from itself must give exactly zero. This
     * is the Block C acceptance criterion in miniature: Earth minus
     * Earth is the origin. */
    check_vec(vec3_sub(a, a), 0.0, 0.0, 0.0, "a - a is zero");

    printf("scaling\n");

    check_vec(vec3_scale(a, 2.0), 2.0, 4.0, 6.0, "doubling");
    check_vec(vec3_scale(a, 0.0), 0.0, 0.0, 0.0, "scaling by zero");
    check_vec(vec3_scale(a, 1.0), 1.0, 2.0, 3.0, "scaling by one");
    check_vec(vec3_scale(a, -1.0), -1.0, -2.0, -3.0, "negation");

    printf("length\n");

    /* The roadmap acceptance criterion for B3. A 3-4-5 right triangle
     * gives an exactly representable answer, so this is one of the
     * rare floating-point results that is precisely correct. */
    vec3_t triangle = { 3.0, 4.0, 0.0 };
    check(vec3_length(triangle), 5.0, "3-4-5 triangle gives exactly 5");

    check(vec3_length(zero), 0.0, "zero vector has zero length");

    /* Unit vectors along each axis. Three separate checks because a
     * bug that only affects one component would hide behind the
     * other two. */
    check(vec3_length((vec3_t){ 1.0, 0.0, 0.0 }), 1.0, "unit x");
    check(vec3_length((vec3_t){ 0.0, 1.0, 0.0 }), 1.0, "unit y");
    check(vec3_length((vec3_t){ 0.0, 0.0, 1.0 }), 1.0, "unit z");

    /* Length must ignore sign - it is a magnitude. */
    check(vec3_length((vec3_t){ -3.0, -4.0, 0.0 }), 5.0,
          "length ignores sign");

    /* A 3-4-12-13 quadruple: sqrt(9 + 16 + 144) = sqrt(169) = 13. */
    check(vec3_length((vec3_t){ 3.0, 4.0, 12.0 }), 13.0,
          "three-dimensional length");

    printf("dot product\n");

    /* 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32 */
    check(vec3_dot(a, b), 32.0, "dot product of a and b");

    /* Commutative. */
    check(vec3_dot(b, a), 32.0, "dot product is symmetric");

    /* Perpendicular vectors have a dot product of exactly zero. This
     * is the property that makes the dot product useful for angles. */
    check(vec3_dot((vec3_t){ 1.0, 0.0, 0.0 },
                   (vec3_t){ 0.0, 1.0, 0.0 }), 0.0,
          "perpendicular vectors dot to zero");

    /* A vector dotted with itself gives its length squared. */
    check(vec3_dot(triangle, triangle), 25.0,
          "self dot product is length squared");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}