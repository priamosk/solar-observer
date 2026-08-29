/*
 * julian.c - implementation of the Julian Date conversion.
 *
 * The algorithm is the standard one from Meeus, "Astronomical
 * Algorithms", chapter 7. It is compact but the integer arithmetic
 * is subtle, so each step is explained below.
 */

#include "julian.h"

#include <math.h>   /* floor */

double centuries_since_j2000(double jd)
{
    /* Elapsed days since the epoch, divided by days per century.
     * Both operands are double, so this is real division - no
     * truncation. */
    return (jd - JD_J2000) / DAYS_PER_JULIAN_CENTURY;
}

double jd_from_utc(int year, int month, int day,
                   int hour, int minute, double second)
{
    /* --- Step 1: shift January and February into the previous year ---
     *
     * The algorithm treats March as the first month of the year. This
     * is not arbitrary: it puts the leap day at the END of the year,
     * so the leap year rule never interrupts the month-length pattern
     * partway through.
     *
     * So January 2026 is handled as "month 13 of 2025", and February
     * 2026 as "month 14 of 2025".
     *
     * We modify the parameters directly. In C, parameters are copies -
     * changing them here does not affect the caller's variables. */
    if (month <= 2) {
        year  -= 1;
        month += 12;
    }
        /* --- Step 2: the Gregorian calendar correction ---
     *
     * The Gregorian leap rule: a year divisible by 4 is a leap year,
     * EXCEPT centuries, UNLESS the century is divisible by 400.
     * So 2000 was a leap year but 1900 was not.
     *
     * `a` is the century number.
     *
     * NOTE ON INTEGER DIVISION: year / 100 here is INTEGER division,
     * which truncates toward zero. That truncation is exactly what we
     * want - we are asking "which century" and discarding the
     * remainder deliberately.
     *
     * This is the single most important thing to notice in this
     * function. In Step 4 below, the same-looking division would be a
     * bug. Whenever you see division in numerical C, stop and ask
     * which of the two it is. */
    int a = year / 100;

    /* `b` corrects for the days dropped by the Gregorian reform.
     * a / 4 is again deliberate integer division: it expresses the
     * divisible-by-400 rule in terms of centuries. */
    int b = 2 - a + (a / 4);
        /* --- Step 3: assemble the day count at midnight ---
     *
     * 365.25 * (year + 4716) is the number of days in all complete
     * years since the Julian epoch, at an average of 365.25 days per
     * year. The 4716 offset moves the origin back to 4713 BC.
     *
     * NOTE: year is an int, but 365.25 is a double, so year is
     * promoted to double and this is FLOATING-POINT multiplication.
     * floor() then takes the whole number of days. Writing
     * 365 * (year + 4716) with all integers would silently lose every
     * leap day.
     *
     * 30.6001 * (month + 1) is a fitted constant that reproduces the
     * cumulative day count of the irregular month lengths. This only
     * works because Step 1 moved February to the end of the year. The
     * trailing 0.0001 defends against floor() tipping the wrong way on
     * a value that lands exactly on an integer boundary.
     *
     * The final -1524.5 sets the origin correctly and applies the
     * half-day shift for the noon-based convention. */
    double jd_at_midnight =
          floor(365.25 * (year + 4716))
        + floor(30.6001 * (month + 1))
        + (double)day
        + (double)b
        - 1524.5;
            /* --- Step 4: add the time of day as a fraction of a day ---
     *
     * THE TRAP. The casts to double are essential here.
     *
     * hour and minute are int. Without the cast, hour / 24 is INTEGER
     * division and every time before 24:00 evaluates to zero, meaning
     * every calculation silently uses midnight.
     *
     * Compare with Step 2 above: there, integer division was exactly
     * what the algorithm required. Here, it destroys the result. Same
     * operator, opposite meaning, twenty lines apart.
     *
     * This bug does not crash. It produces plausible-looking numbers
     * that are wrong by up to a full day. */
    double day_fraction =
          ((double)hour   / 24.0)
        + ((double)minute / 1440.0)      /* 24 * 60 minutes per day */
        + (second         / 86400.0);    /* 24 * 60 * 60 seconds    */

    return jd_at_midnight + day_fraction;
}