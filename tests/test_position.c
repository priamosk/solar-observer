/*
 * test_position.c - tests for heliocentric position.
 *
 * VERIFICATION STRATEGY
 *
 * We do not yet have JPL reference positions - that is Block D. What
 * we can check here is that the output obeys physical laws that must
 * hold regardless of the exact numbers:
 *
 *   - each planet stays within its known perihelion and aphelion
 *   - orbits are closed: the position after one full period returns
 *   - the ecliptic is Earth's plane, so Earth's z stays near zero
 *   - inner planets stay inside outer ones
 *
 * These catch structural errors - a wrong rotation order, a swapped
 * sign, a missing factor - even without a reference ephemeris.
 */

#include <stdio.h>
#include <math.h>

#include "position.h"
#include "julian.h"
#include "vec3.h"

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

static void check_range(double value, double low, double high,
                        const char *name)
{
    checks_run++;

    if (value < low || value > high) {
        printf("  FAIL  %s\n", name);
        printf("        %.6f is outside [%.6f, %.6f]\n", value, low, high);
        checks_failed++;
    }
}

/*
 * Distance from the Sun at a given moment. Returns -1 on failure so
 * a solver error cannot masquerade as a plausible distance.
 */
static double distance_at(planet_id_t id, double jd)
{
    vec3_t position;

    if (position_heliocentric(id, jd, &position) != 0) {
        return -1.0;
    }

    return vec3_length(position);
}
int main(void)
{
    double j2000 = jd_from_utc(2000, 1, 1, 12, 0, 0.0);

    printf("Earth orbital distance\n");

    /* THE roadmap acceptance criterion for C1.
     *
     * Earth's perihelion is 0.9833 AU and aphelion 1.0167 AU. Sample
     * every 5 days through a full year - the distance must never
     * leave that band.
     *
     * This single test catches an enormous range of errors. A wrong
     * eccentricity factor, a missing -e term, a broken rotation, a
     * units mix-up: all of them push the distance outside these
     * bounds. */
    for (int day = 0; day < 365; day += 5) {
        double d = distance_at(PLANET_EARTH, j2000 + (double)day);

        check_range(d, 0.983, 1.017,
                    "Earth stays between perihelion and aphelion");
    }

    printf("Earth stays in the ecliptic plane\n");

    /* The ecliptic IS Earth's orbital plane, so Earth's z coordinate
     * must be essentially zero. The table gives Earth an inclination
     * of -0.00001531 degrees, which at 1 AU works out to well under
     * 0.001 AU of vertical displacement.
     *
     * If the rotations were applied in the wrong order, this is one
     * of the first tests that would break. */
    for (int day = 0; day < 365; day += 30) {
        vec3_t position;
        position_heliocentric(PLANET_EARTH, j2000 + (double)day, &position);

        check_true(fabs(position.z) < 0.001,
                   "Earth's z coordinate is near zero");
    }

    printf("orbits are closed\n");

    /* After one full orbital period a planet must return to
     * essentially the same place. Earth's year is 365.256 days - the
     * sidereal year, not the 365.242 day tropical year.
     *
     * Tolerance is loose because the orbit precesses slightly and
     * the period is not exactly 365.256 days. But a planet that
     * drifts far from its starting point after one year has a
     * broken mean longitude rate. */
    vec3_t start, after_year;
    position_heliocentric(PLANET_EARTH, j2000, &start);
    position_heliocentric(PLANET_EARTH, j2000 + 365.256, &after_year);

    check_true(vec3_length(vec3_sub(start, after_year)) < 0.02,
               "Earth returns after one sidereal year");

    printf("all planets within known bounds\n");

    /* Perihelion and aphelion for each planet, in AU, with a small
     * margin. Sampled across 20 years so every planet moves through
     * a meaningful part of its orbit - except Neptune, whose 165
     * year period means it barely moves. */
    struct {
        planet_id_t id;
        double min;
        double max;
        const char *name;
    } bounds[] = {
        { PLANET_MERCURY,  0.30,  0.48, "Mercury" },
        { PLANET_VENUS,    0.71,  0.74, "Venus"   },
        { PLANET_EARTH,    0.98,  1.02, "Earth"   },
        { PLANET_MARS,     1.38,  1.67, "Mars"    },
        { PLANET_JUPITER,  4.94,  5.47, "Jupiter" },
        { PLANET_SATURN,   9.00, 10.13, "Saturn"  },
        { PLANET_URANUS,  18.27, 20.10, "Uranus"  },
        { PLANET_NEPTUNE, 29.79, 30.35, "Neptune" }
    };

    for (int p = 0; p < 8; p++) {
        for (int day = 0; day < 7300; day += 100) {
            double d = distance_at(bounds[p].id, j2000 + (double)day);

            check_range(d, bounds[p].min, bounds[p].max, bounds[p].name);
        }
    }

    printf("planetary ordering\n");

    /* Inner planets must stay inside outer ones. Mercury's aphelion
     * is 0.47 AU and Venus's perihelion is 0.718, so they never
     * cross - and neither does any other adjacent pair. */
    for (int day = 0; day < 3650; day += 50) {
        double jd = j2000 + (double)day;

        for (int p = 1; p < PLANET_COUNT; p++) {
            double inner = distance_at((planet_id_t)(p - 1), jd);
            double outer = distance_at((planet_id_t)p, jd);

            check_true(outer > inner, "planets never cross orbits");
        }
    }
        printf("geocentric conversion\n");

    /* THE roadmap acceptance criterion for C2. Earth is at no
     * distance from itself, so its geocentric position must be
     * exactly the origin - not approximately, exactly, because the
     * same value is subtracted from itself. */
    vec3_t earth_geo;
    position_geocentric(PLANET_EARTH, j2000, &earth_geo);

    check_true(earth_geo.x == 0.0, "Earth geocentric x is exactly zero");
    check_true(earth_geo.y == 0.0, "Earth geocentric y is exactly zero");
    check_true(earth_geo.z == 0.0, "Earth geocentric z is exactly zero");

    /* Mars seen from Earth. The two planets' distance varies enormously
     * as they orbit: at opposition, when Earth passes between Mars and
     * the Sun, they come within about 0.37 AU. At conjunction, with the
     * Sun between them, they are up to 2.68 AU apart.
     *
     * This 7x variation is why some Mars observing seasons are
     * spectacular and others are not worth the effort. */
    for (int day = 0; day < 3650; day += 30) {
        vec3_t mars_geo;
        position_geocentric(PLANET_MARS, j2000 + (double)day, &mars_geo);

        check_range(vec3_length(mars_geo), 0.36, 2.70,
                    "Mars geocentric distance is within known bounds");
    }

    /* The triangle inequality, applied to the Sun-Earth-planet
     * triangle. The distance from Earth to a planet can never exceed
     * the sum of the two heliocentric distances, and can never be
     * less than their difference.
     *
     * This is pure geometry and must hold for every planet at every
     * moment. It is a strong check: any sign error or frame confusion
     * in the subtraction breaks it. */
    for (int day = 0; day < 1825; day += 50) {
        double jd = j2000 + (double)day;

        for (int p = 0; p < PLANET_COUNT; p++) {
            if (p == PLANET_EARTH) {
                continue;   /* the triangle degenerates for Earth */
            }

            vec3_t helio, geo, earth_pos;
            position_heliocentric((planet_id_t)p, jd, &helio);
            position_heliocentric(PLANET_EARTH, jd, &earth_pos);
            position_geocentric((planet_id_t)p, jd, &geo);

            double d_planet = vec3_length(helio);
            double d_earth  = vec3_length(earth_pos);
            double d_geo    = vec3_length(geo);

            /* A small epsilon absorbs floating-point rounding at the
             * exact conjunction and opposition points, where the
             * inequality becomes an equality. */
            check_true(d_geo <= d_planet + d_earth + 1e-9,
                       "triangle inequality upper bound");
            check_true(d_geo >= fabs(d_planet - d_earth) - 1e-9,
                       "triangle inequality lower bound");
        }
    }

    printf("error handling\n");

    vec3_t dummy;
    check_true(position_heliocentric(PLANET_EARTH, j2000, NULL) != 0,
               "NULL result rejected");
    check_true(position_heliocentric(PLANET_COUNT, j2000, &dummy) != 0,
               "invalid planet id rejected");
    check_true(position_heliocentric((planet_id_t)-1, j2000, &dummy) != 0,
               "negative planet id rejected");
    check_true(position_geocentric(PLANET_MARS, j2000, NULL) != 0,
               "geocentric NULL result rejected");
    check_true(position_geocentric(PLANET_COUNT, j2000, &dummy) != 0,
               "geocentric invalid planet id rejected");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}