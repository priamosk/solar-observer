/*
 * test_jpl.c - validation against JPL Horizons reference positions.
 *
 * The other test suites verify the code is self-consistent. This one
 * verifies it agrees with reality.
 *
 * Reference data from NASA JPL Horizons, generated with:
 *   Ephemeris Type:    Observer Table
 *   Observer Location: Geocentric [500]
 *   Quantities:        1 (Astrometric RA & DEC), 20 (Observer range)
 *   Angle format:      decimal degrees
 *   Refraction:        none (airless)
 *
 * Horizons uses numerical integration of the full gravitational
 * problem, not Keplerian elements. Its planetary accuracy is a
 * fraction of an arcsecond, so any disagreement is our error.
 */

#include <stdio.h>
#include <math.h>

#include "equatorial.h"
#include "julian.h"
#include "angles.h"

static int checks_run = 0;
static int checks_failed = 0;
static double worst_error = 0.0;
static const char *worst_planet = "none";

/*
 * Angular separation between two points on a sphere, in degrees.
 *
 * Naive subtraction of RA and Dec is wrong for two reasons: RA wraps
 * at 360, and a degree of RA covers less sky near the poles than at
 * the equator - by a factor of cos(declination).
 *
 * The spherical law of cosines gives the true separation:
 *
 *   cos(d) = sin(dec1)sin(dec2) + cos(dec1)cos(dec2)cos(ra1 - ra2)
 */
static double angular_separation(double ra1, double dec1,
                                 double ra2, double dec2)
{
    double d1 = deg2rad(dec1);
    double d2 = deg2rad(dec2);
    double dra = deg2rad(norm_180(ra1 - ra2));

    double cos_sep = sin(d1) * sin(d2) + cos(d1) * cos(d2) * cos(dra);

    /* Clamp before acos - rounding can push this past 1.0 and produce
     * NaN, which then defeats every comparison downstream. */
    if (cos_sep >  1.0) cos_sep =  1.0;
    if (cos_sep < -1.0) cos_sep = -1.0;

    return rad2deg(acos(cos_sep));
}

/*
 * Compare one computed position against its JPL reference.
 *
 * Tolerances are in arcminutes for angle, AU for distance.
 */
static void check_against_jpl(planet_id_t id, const char *name,
                              int year, int month, int day,
                              double ref_ra, double ref_dec,
                              double ref_distance,
                              double tolerance_arcmin)
{
    checks_run++;

    double jd = jd_from_utc(year, month, day, 0, 0, 0.0);

    equatorial_t eq;

    if (equatorial_of(id, jd, &eq) != 0) {
        printf("  FAIL  %s %04d-%02d-%02d: calculation failed\n",
               name, year, month, day);
        checks_failed++;
        return;
    }

    double separation_deg = angular_separation(eq.right_ascension,
                                               eq.declination,
                                               ref_ra, ref_dec);
    double separation_arcmin = separation_deg * 60.0;

    double distance_error = fabs(eq.distance - ref_distance);

    /* Track the worst case for the summary line, which is the number
     * that goes in the README. */
    if (separation_arcmin > worst_error) {
        worst_error = separation_arcmin;
        worst_planet = name;
    }

    printf("  %-8s %04d-%02d-%02d   %6.2f'   %.6f AU\n",
           name, year, month, day, separation_arcmin, distance_error);

    if (separation_arcmin > tolerance_arcmin) {
        printf("        FAIL: %.2f arcmin exceeds tolerance of %.2f\n",
               separation_arcmin, tolerance_arcmin);
        checks_failed++;
        return;
    }

    /* Distance tolerance of 0.01 AU is generous - measured errors are
     * under 0.001 - but distance is a secondary check. */
    if (distance_error > 0.01) {
        printf("        FAIL: distance error %.6f AU exceeds 0.01\n",
               distance_error);
        checks_failed++;
    }
}
int main(void)
{
    /* Tolerance in arcminutes.
     *
     * Two tiers, because the error is a property of the planet rather
     * than of the code. Jupiter and Saturn have orbital periods in a
     * near 5:2 ratio, so their mutual perturbations accumulate
     * systematically rather than averaging out - the Great
     * Inequality. A linear "value plus rate" element model cannot
     * represent a periodic perturbation.
     *
     * Holding all planets to the same tolerance would either fail the
     * five that are accurate to half an arcminute, or hide how much
     * better they are than the two that are not. */
    const double TOL_NORMAL = 2.0;    /* the five well-behaved planets */
    const double TOL_GIANT  = 10.0;   /* Jupiter and Saturn           */

    printf("\nJPL Horizons validation\n");
    printf("  planet   date         error    distance error\n");
    printf("  ---------------------------------------------\n");

    /* --- 2026-09-01, all seven planets ---------------------------- */

    check_against_jpl(PLANET_MERCURY, "Mercury", 2026, 9, 1,
                      164.316908989, 8.388871512, 1.37677948876899,
                      TOL_NORMAL);

    check_against_jpl(PLANET_VENUS, "Venus", 2026, 9, 1,
                      199.806941232, -12.009893824, 0.55438100299357,
                      TOL_NORMAL);

    check_against_jpl(PLANET_MARS, "Mars", 2026, 9, 1,
                      104.203926136, 23.330610785, 1.85004078767173,
                      TOL_NORMAL);

    check_against_jpl(PLANET_JUPITER, "Jupiter", 2026, 9, 1,
                      135.953235761, 17.323916147, 6.19550708769135,
                      TOL_GIANT);

    check_against_jpl(PLANET_SATURN, "Saturn", 2026, 9, 1,
                      13.269849177, 2.806007560, 8.60080617910757,
                      TOL_GIANT);

    check_against_jpl(PLANET_URANUS, "Uranus", 2026, 9, 1,
                      63.384092400, 21.026754707, 19.3687484666110,
                      TOL_NORMAL);

    check_against_jpl(PLANET_NEPTUNE, "Neptune", 2026, 9, 1,
                      3.576449092, 0.009597001, 28.9617153711556,
                      TOL_NORMAL);

    /* --- 2000-01-01, the J2000 epoch itself -----------------------
     *
     * Element values are published for this instant, so the elements
     * themselves carry no extrapolation error here. Any residual is
     * the model, not the arithmetic. */

    check_against_jpl(PLANET_MARS, "Mars", 2000, 1, 1,
                      330.153016459, -13.320398365, 1.84697688138615,
                      TOL_NORMAL);

    check_against_jpl(PLANET_JUPITER, "Jupiter", 2000, 1, 1,
                      23.851957842, 8.586215421, 4.61342273127327,
                      TOL_GIANT);

    check_against_jpl(PLANET_SATURN, "Saturn", 2000, 1, 1,
                      38.776655468, 12.617180164, 8.64562193601431,
                      TOL_GIANT);

    /* --- The extremes --------------------------------------------
     *
     * Ten years before the epoch and forty-nine years after, near the
     * edge of the element table's stated 1800-2050 validity. */

    check_against_jpl(PLANET_JUPITER, "Jupiter", 1990, 1, 1,
                      95.819305354, 23.214421397, 4.17057854607155,
                      TOL_GIANT);

    check_against_jpl(PLANET_SATURN, "Saturn", 2049, 1, 1,
                      288.199414642, -22.143267347, 11.0006193616934,
                      TOL_GIANT);

    printf("  ---------------------------------------------\n");
    printf("  worst case: %.2f arcminutes (%s)\n\n",
           worst_error, worst_planet);
    printf("  %d comparisons, %d failures\n\n", checks_run, checks_failed);

    return checks_failed;
}