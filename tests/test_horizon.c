/*
 * test_horizon.c - tests for altitude and azimuth.
 *
 * The strongest checks here use the Sun again, because its position
 * is tied to things we all directly experience: it is highest at
 * local noon, it is up longer in summer, and at the poles it does
 * not set at all in June.
 */

#include <stdio.h>
#include <math.h>

#include "horizon.h"
#include "position.h"
#include "equatorial.h"
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
        printf("        %.4f is outside [%.4f, %.4f]\n", value, low, high);
        checks_failed++;
    }
}

/*
 * Altitude of the Sun for a given observer and moment.
 *
 * The Sun is not in the element table, so its geocentric position is
 * built by negating Earth's heliocentric position, then run through
 * the same rotations as any planet.
 */
static double sun_altitude(double jd, observer_t obs)
{
    vec3_t earth;

    if (position_heliocentric(PLANET_EARTH, jd, &earth) != 0) {
        return -999.0;
    }

    vec3_t sun = vec3_scale(earth, -1.0);

    /* Ecliptic to equatorial. */
    double eps = deg2rad(obliquity(jd));
    double x = sun.x;
    double y = sun.y * cos(eps) - sun.z * sin(eps);
    double z = sun.y * sin(eps) + sun.z * cos(eps);

    double dist = sqrt(x * x + y * y + z * z);
    double ra   = norm_360(rad2deg(atan2(y, x)));
    double dec  = rad2deg(asin(z / dist));

    /* Equatorial to horizontal. */
    double H       = deg2rad(norm_180(lst(jd, obs.longitude) - ra));
    double dec_rad = deg2rad(dec);
    double lat_rad = deg2rad(obs.latitude);

    double sin_alt = sin(dec_rad) * sin(lat_rad)
                   + cos(dec_rad) * cos(lat_rad) * cos(H);

    return rad2deg(asin(sin_alt));
}
int main(void)
{
    /* Zurich, close enough to Schaffhausen for these tests. */
    observer_t zurich    = { 47.37,   8.54 };
    observer_t equator   = {  0.00,   0.00 };
    observer_t north_pole = { 89.90,  0.00 };
    observer_t sydney    = { -33.87, 151.21 };

    printf("sidereal time\n");

    /* GMST must always be a valid angle. */
    for (int day = 0; day < 400; day += 7) {
        double jd = JD_J2000 + (double)day;
        check_range(gmst(jd), 0.0, 360.0, "GMST is a valid angle");
    }

    /* Sidereal time gains about 3 minutes 56 seconds per solar day,
     * which is 0.9856 degrees. Over one day the difference between
     * GMST values should be that excess. */
    double g0 = gmst(JD_J2000);
    double g1 = gmst(JD_J2000 + 1.0);
    check_range(norm_180(g1 - g0), 0.98, 0.99,
                "sidereal time gains 0.9856 degrees per day");

    /* After one sidereal day - 0.99726957 solar days - the sky has
     * returned to the same orientation. */
    double g_sidereal = gmst(JD_J2000 + 0.99726957);
    check_range(fabs(norm_180(g_sidereal - g0)), 0.0, 0.001,
                "sidereal day returns to the same angle");

    /* Longitude shifts LST directly, one degree per degree. */
    check_range(norm_180(lst(JD_J2000, 10.0) - lst(JD_J2000, 0.0)),
                9.99, 10.01, "longitude shifts LST one for one");

    printf("solar altitude at the equator\n");

    /* At the equinox, on the equator, the Sun passes directly
     * overhead at local noon. Sampling through a day, the maximum
     * altitude must come very close to 90 degrees. */
    double equinox = jd_from_utc(2026, 3, 20, 0, 0, 0.0);
    double max_alt = -999.0;

    for (int minute = 0; minute < 1440; minute += 10) {
        double alt = sun_altitude(equinox + (double)minute / 1440.0,
                                  equator);
        if (alt > max_alt) {
            max_alt = alt;
        }
    }

    check_range(max_alt, 88.0, 90.0,
                "Sun is overhead at the equator on the equinox");

    printf("solar altitude at Zurich\n");

    /* At the June solstice the Sun reaches roughly
     * 90 - latitude + obliquity = 90 - 47.37 + 23.44 = 66 degrees. */
    double june = jd_from_utc(2026, 6, 21, 0, 0, 0.0);
    max_alt = -999.0;

    for (int minute = 0; minute < 1440; minute += 10) {
        double alt = sun_altitude(june + (double)minute / 1440.0, zurich);
        if (alt > max_alt) {
            max_alt = alt;
        }
    }

    check_range(max_alt, 65.0, 67.0,
                "Sun reaches about 66 degrees at Zurich in June");

    /* At the December solstice, roughly
     * 90 - 47.37 - 23.44 = 19 degrees. The low winter sun. */
    double december = jd_from_utc(2026, 12, 21, 0, 0, 0.0);
    max_alt = -999.0;

    for (int minute = 0; minute < 1440; minute += 10) {
        double alt = sun_altitude(december + (double)minute / 1440.0,
                                  zurich);
        if (alt > max_alt) {
            max_alt = alt;
        }
    }

    check_range(max_alt, 18.0, 20.0,
                "Sun reaches only about 19 degrees at Zurich in December");

    printf("midnight sun at the pole\n");

    /* At the north pole in June the Sun never sets. Its altitude
     * stays positive for the entire 24 hours. */
    for (int minute = 0; minute < 1440; minute += 30) {
        double alt = sun_altitude(june + (double)minute / 1440.0,
                                  north_pole);

        check_true(alt > 0.0, "Sun never sets at the pole in June");
    }

    /* And never rises in December. */
    for (int minute = 0; minute < 1440; minute += 30) {
        double alt = sun_altitude(december + (double)minute / 1440.0,
                                  north_pole);

        check_true(alt < 0.0, "Sun never rises at the pole in December");
    }

    printf("southern hemisphere seasons are reversed\n");

    /* Sydney's summer is in December. The Sun must be higher there in
     * December than in June - the opposite of Zurich. */
    double sydney_june = -999.0;
    double sydney_dec  = -999.0;

    for (int minute = 0; minute < 1440; minute += 10) {
        double a = sun_altitude(june + (double)minute / 1440.0, sydney);
        double b = sun_altitude(december + (double)minute / 1440.0, sydney);

        if (a > sydney_june) sydney_june = a;
        if (b > sydney_dec)  sydney_dec  = b;
    }

    check_true(sydney_dec > sydney_june,
               "Sydney's summer is in December");

    printf("planetary coordinates are well formed\n");

    /* Every planet, sampled across a year. Altitude must stay within
     * [-90, 90] and azimuth within [0, 360). */
    for (int day = 0; day < 365; day += 10) {
        double jd = JD_J2000 + (double)day;

        for (int p = 0; p < PLANET_COUNT; p++) {
            if (p == PLANET_EARTH) {
                continue;
            }

            horizontal_t h;
            check_true(horizon_of((planet_id_t)p, jd, zurich, &h) == 0,
                       "horizontal coordinates computed");

            check_range(h.altitude, -90.0, 90.0, "altitude in range");
            check_range(h.azimuth, 0.0, 360.0, "azimuth in range");
        }
    }

    printf("error handling\n");

    horizontal_t dummy;
    check_true(horizon_of(PLANET_MARS, JD_J2000, zurich, NULL) != 0,
               "NULL result rejected");
    check_true(horizon_of(PLANET_COUNT, JD_J2000, zurich, &dummy) != 0,
               "invalid planet id rejected");
    check_true(horizon_of(PLANET_EARTH, JD_J2000, zurich, &dummy) != 0,
               "Earth rejected");

    printf("\n%d checks, %d failures\n", checks_run, checks_failed);

    return checks_failed;
}