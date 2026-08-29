/*
 * julian.h - calendar date to Julian Date conversion.
 *
 * Calendar dates are hostile to arithmetic: months have different
 * lengths, leap years exist, and centuries have their own leap rule.
 * The Julian Date sidesteps all of it by counting days - including the
 * fractional part - continuously from a fixed origin in 4713 BC.
 *
 * Two moments in time become two doubles, and the interval between
 * them is a subtraction.
 *
 * NOTE: the Julian day begins at NOON, not midnight. This is a
 * historical convention from astronomers who did not want the date to
 * change in the middle of an observing session. The consequence is
 * that midnight UTC always lands on a .5 boundary.
 */

#ifndef JULIAN_H
#define JULIAN_H

/* The J2000.0 epoch: 2000-01-01 12:00:00 UTC.
 * Orbital elements are published as a value at this instant plus a
 * rate of change per century from it. */
#define JD_J2000 2451545.0

/* A Julian century is defined as exactly 36525 days. Not 36525.25 -
 * the definition is fixed by convention so rate tables are
 * unambiguous. */
#define DAYS_PER_JULIAN_CENTURY 36525.0

/*
 * Convert a UTC calendar date and time to a Julian Date.
 *
 * Valid for Gregorian calendar dates (15 October 1582 onward).
 *
 *   year    full year, e.g. 2026
 *   month   1-12
 *   day     1-31
 *   hour    0-23, UTC - NOT local time
 *   minute  0-59
 *   second  0-59.999
 *
 * Passing local time instead of UTC shifts every planet position by
 * roughly 15 degrees per hour of offset.
 */
double jd_from_utc(int year, int month, int day,
                   int hour, int minute, double second);

/*
 * Julian centuries elapsed since J2000.0.
 * Negative for dates before the year 2000.
 */
double centuries_since_j2000(double jd);

#endif /* JULIAN_H */