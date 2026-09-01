/*
 * main.c - command line tool for planet positions.
 *
 * Usage:
 *   ./solar YYYY-MM-DD HH:MM LAT LON
 *
 * All four arguments are required. There is no default location:
 * on the target hardware the observer's position comes from the GPS
 * module or from a stored user setting, never from a constant in the
 * source. This tool mirrors that - it is given a position, it does
 * not assume one.
 *
 * Per ADR-005 this is the only file in src/ permitted to call printf.
 * Library code computes and returns; presentation happens here.
 */

#include <stdio.h>
#include <stdlib.h>   /* strtod, EXIT_SUCCESS, EXIT_FAILURE */

#include "horizon.h"
#include "equatorial.h"
#include "julian.h"
#include "orbital.h"

static void print_usage(const char *program_name)
{
    printf("Usage: %s YYYY-MM-DD HH:MM LAT LON\n\n", program_name);
    printf("  YYYY-MM-DD   date, UTC\n");
    printf("  HH:MM        time, UTC - not local time\n");
    printf("  LAT          latitude, positive north\n");
    printf("  LON          longitude, positive east\n\n");
    printf("Example: %s 2026-08-31 21:00 47.70 8.63\n", program_name);
}
/*
 * Parse "YYYY-MM-DD" into three integers.
 *
 * Returns 0 on success, non-zero on failure.
 *
 * sscanf reads a string according to a pattern. "%d-%d-%d" means:
 * an integer, a literal hyphen, an integer, a hyphen, an integer.
 * Each %d needs the ADDRESS of where to store the result, hence &.
 *
 * sscanf returns how many items it successfully converted. Anything
 * other than 3 means the input did not match the pattern.
 */
static int parse_date(const char *text, int *year, int *month, int *day)
{
    if (sscanf(text, "%d-%d-%d", year, month, day) != 3) {
        return -1;
    }

    /* sscanf checked the SHAPE of the input. It did not check the
     * values make sense - "9999-77-88" matches the pattern perfectly.
     * Range checking is a separate job. */
    if (*month < 1 || *month > 12) {
        return -2;
    }

    if (*day < 1 || *day > 31) {
        return -3;
    }

    return 0;
}

/*
 * Parse "HH:MM" into two integers.
 */
static int parse_time(const char *text, int *hour, int *minute)
{
    if (sscanf(text, "%d:%d", hour, minute) != 2) {
        return -1;
    }

    if (*hour < 0 || *hour > 23) {
        return -2;
    }

    if (*minute < 0 || *minute > 59) {
        return -3;
    }

    return 0;
}

/*
 * Parse a decimal number, rejecting anything that is not one.
 *
 * strtod converts leading numeric text and sets `end` to the first
 * character it could not use. If that character is not the string
 * terminator, there was trailing garbage - "47.7abc" would otherwise
 * silently become 47.7.
 *
 * This is why strtod is used rather than atof. atof("abc") returns
 * 0.0 with no way to tell it apart from a genuine zero, and 0.0 is a
 * perfectly valid latitude.
 */
static int parse_double(const char *text, double *value)
{
    char *end;

    *value = strtod(text, &end);

    /* end == text means nothing at all was converted.
     * *end != '\0' means conversion stopped before the string did. */
    if (end == text || *end != '\0') {
        return -1;
    }

    return 0;
}
int main(int argc, char *argv[])
{
    /* argv[0] is the program name, so four arguments means argc == 5. */
    if (argc != 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int year, month, day;
    int hour, minute;
    double latitude, longitude;

    if (parse_date(argv[1], &year, &month, &day) != 0) {
        printf("Bad date: %s\n", argv[1]);
        printf("Expected YYYY-MM-DD, for example 2026-08-31\n");
        return EXIT_FAILURE;
    }

    if (parse_time(argv[2], &hour, &minute) != 0) {
        printf("Bad time: %s\n", argv[2]);
        printf("Expected HH:MM in 24-hour UTC, for example 21:00\n");
        return EXIT_FAILURE;
    }

    if (parse_double(argv[3], &latitude) != 0) {
        printf("Bad latitude: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    if (parse_double(argv[4], &longitude) != 0) {
        printf("Bad longitude: %s\n", argv[4]);
        return EXIT_FAILURE;
    }

    if (latitude < -90.0 || latitude > 90.0) {
        printf("Latitude must be between -90 and 90, got %.4f\n", latitude);
        return EXIT_FAILURE;
    }

    if (longitude < -180.0 || longitude > 180.0) {
        printf("Longitude must be between -180 and 180, got %.4f\n",
               longitude);
        return EXIT_FAILURE;
    }

    double jd = jd_from_utc(year, month, day, hour, minute, 0.0);

    observer_t observer = { latitude, longitude };

    printf("\n%04d-%02d-%02d %02d:%02d UTC    JD %.5f\n",
           year, month, day, hour, minute, jd);
    printf("Observer  %.4f%c  %.4f%c\n\n",
           latitude  >= 0 ? latitude  : -latitude,  latitude  >= 0 ? 'N' : 'S',
           longitude >= 0 ? longitude : -longitude, longitude >= 0 ? 'E' : 'W');

    /* Column headers. The %-9s left-aligns within nine characters,
     * which keeps the columns lined up regardless of name length. */
    printf("%-9s %10s %9s %9s %9s %10s\n",
           "Planet", "RA", "Dec", "Altitude", "Azimuth", "Distance");
    printf("%-9s %10s %9s %9s %9s %10s\n",
           "", "(deg)", "(deg)", "(deg)", "(deg)", "(AU)");
    printf("---------------------------------------------------------------\n");

    for (int p = 0; p < PLANET_COUNT; p++) {
        /* Earth has no position relative to itself. */
        if (p == PLANET_EARTH) {
            continue;
        }

        const orbital_elements_t *el = orbital_get((planet_id_t)p);

        equatorial_t eq;
        horizontal_t h;

        if (equatorial_of((planet_id_t)p, jd, &eq) != 0 ||
            horizon_of((planet_id_t)p, jd, observer, &h) != 0) {
            printf("%-9s  calculation failed\n", el->name);
            continue;
        }

        /* Mark whether the planet is currently above the horizon.
         * This is the single most useful piece of information for an
         * observer, so it gets its own visible marker rather than
         * requiring the reader to check the sign of the altitude. */
        const char *visible = (h.altitude > 0.0) ? "  up" : "";

        printf("%-9s %10.3f %9.3f %9.3f %9.3f %10.4f%s\n",
               el->name,
               eq.right_ascension,
               eq.declination,
               h.altitude,
               h.azimuth,
               h.distance,
               visible);
    }

    printf("\n");

    return EXIT_SUCCESS;
}