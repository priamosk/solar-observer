/*
 * test_equatorial.c - tests for equatorial coordinates.
 *
 * The strongest check available here uses the Sun. Its declination
 * traces the seasons exactly: +23.44 degrees at the June solstice,
 * -23.44 in December, zero at both equinoxes. These are not
 * approximations - they are the definition of the solstices.
 *
 * The Sun is not in the element table, but its geocentric position is
 * simply Earth's heliocentric position negated.
 */

#include <stdio.h>
#include <math.h>

#include "equatorial.h"
#include "position.h"
#include "julian.h"
#include "angles.h"
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
 * Declination of the Sun as seen from Earth, in degrees.
 *
 * The Sun's geocentric position is Earth's heliocentric position
 * negated - if Earth is at (x,y,z) from the Sun, the Sun is at
 * (-x,-y,-z) from Earth.
 *
 * The rotation and conversion are repeated here rather than reusing
 * equatorial_of, because that function only accepts planet ids.
 */
static double sun_declination(double jd)
{
    vec3_t earth;

    if (position_heliocentric(PLANET_EARTH, jd, &earth) != 0) {
        return 999.0;   /* impossible value, so a failure cannot pass */
    }

    vec3_t sun = vec3_scale(earth, -1.0);

    double eps_rad = deg2rad(obliquity(jd));

    /* Only the z component is needed. Declination is the angle above
     * the equatorial plane, which depends on z and the distance -
     * the y component would only be needed for right ascension. */
    double z = sun.y * sin(eps_rad) + sun.z * cos(eps_rad);

    double distance = vec3_length(sun);

    return rad2deg(asin(z / distance));
}
int main(void)
{
    double j2000 = jd_from_utc(2000, 1, 1, 12, 0, 0.0);

    printf("obliquity\n");

    /* At J2000 the mean obliquity is 23.439291 degrees by
     * definition - that is the epoch value in the formula. */
    check_range(obliquity(j2000), 23.4392, 23.4394,
                "obliquity at J2000");

    /* It decreases with time, by about 47 arcseconds per century.
     * Earth's tilt is slowly straightening on a 41000 year cycle. */
    check_true(obliquity(j2000 + 36525.0) < obliquity(j2000),
               "obliquity decreases over a century");

    /* And was larger in the past. */
    check_true(obliquity(j2000 - 36525.0) > obliquity(j2000),
               "obliquity was larger a century ago");

    printf("solar declination through the year\n");

    /* THE roadmap acceptance criterion for C3.
     *
     * The June solstice is the moment the Sun reaches its most
     * northerly declination, equal to the obliquity. In 2026 it falls
     * on 21 June. Sampling at midday gives a value very close to the
     * maximum. */
    double june_solstice = jd_from_utc(2026, 6, 21, 12, 0, 0.0);
    check_range(sun_declination(june_solstice), 23.2, 23.5,
                "Sun is at maximum northern declination in June");

    /* December solstice: the mirror image. */
    double dec_solstice = jd_from_utc(2026, 12, 21, 12, 0, 0.0);
    check_range(sun_declination(dec_solstice), -23.5, -23.2,
                "Sun is at maximum southern declination in December");

    /* At the equinoxes the Sun crosses the celestial equator, so its
     * declination passes through zero. The crossing is a precise
     * instant, and sampling at noon on the nominal date lands within
     * a fraction of a degree of it. */
    double march_equinox = jd_from_utc(2026, 3, 20, 12, 0, 0.0);
    check_range(sun_declination(march_equinox), -0.5, 0.5,
                "Sun crosses the equator at the March equinox");

    double sept_equinox = jd_from_utc(2026, 9, 22, 12, 0, 0.0);
    check_range(sun_declination(sept_equinox), -0.5, 0.5,
                "Sun crosses the equator at the September equinox");

    /* The Sun's declination can never exceed the obliquity in either
     * direction. This is what defines the tropics: the Sun is
     * directly overhead at 23.44 degrees north at most, which is the
     * Tropic of Cancer. */
    for (int day = 0; day < 365; day += 5) {
        double d = sun_declination(j2000 + (double)day);

        check_range(d, -23.5, 23.5,
                    "solar declination never exceeds the obliquity");
    }

    printf("planetary coordinates are well formed\n");

    /* Every planet, sampled across ten years. Right ascension must
     * stay in [0, 360) and declination in [-90, 90]. Values outside
     * these ranges mean a normalisation bug or a NaN. */
    for (int day = 0; day < 3650; day += 100) {
        double jd = j2000 + (double)day;

        for (int p = 0; p < PLANET_COUNT; p++) {
            if (p == PLANET_EARTH) {
                continue;
            }

            equatorial_t eq;
            check_true(equatorial_of((planet_id_t)p, jd, &eq) == 0,
                       "coordinates computed successfully");

            check_range(eq.right_ascension, 0.0, 360.0,
                        "right ascension in range");
            check_range(eq.declination, -90.0, 90.0,
                        "declination in range");
            check_true(eq.distance > 0.0, "distance is positive");
        }
    }

    printf("planets stay near the ecliptic\n");

    /* All eight orbits lie within about 7 degrees of the ecliptic,
     * and the ecliptic itself is tilted 23.44 degrees to the equator.
     * So no planet can have a declination beyond roughly 31 degrees.
     *
     * This is why planets always appear in the same narrow band of
     * sky - the zodiac. A rotation applied in the wrong order or by
     * the wrong angle would break this immediately. */
    for (int day = 0; day < 7300; day += 100) {
        double jd = j2000 + (double)day;

        for (int p = 0; p < PLANET_COUNT; p++) {
            if (p == PLANET_EARTH) {
                continue;
            }

            equatorial_t eq;
            equatorial_of((planet_id_t)p, jd, &eq);

            check_range(eq.declination, -32.0, 32.0,
                        "planet stays within the zodiac band");
        }
    }

    printf("error handling\n");

    equatorial_t dummy;
    check_true(equatorial_of(PLANET_MARS, j2000, NULL) != 0,
               "NULL result rejected");
    check_true(equatorial_of(PLANET_COUNT, j2000, &dummy) != 0,
               "invalid planet id rejected");

    /* Earth's geocentric position is the origin, so no direction
     * exists. The function must report this rather than returning
     * NaN. */
    check_true(equatorial_of(PLANET_EARTH, j2000, &dummy) != 0,
               "Earth is rejected - no direction from itself");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}