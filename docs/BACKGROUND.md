# Background

The reasoning, history, and mathematics behind the technical choices in
this project. The code says what happens; this says why it happens that
way.

---

## Why Julian Dates

Calendar dates are built for humans reading a wall calendar, not for
arithmetic. Asking "how much time passed between 28 February 2024 23:00
and 1 March 2024 01:00" requires knowing that 2024 was a leap year,
which requires the leap rule, which requires handling day, month and
possibly year rollovers.

The Julian Date is a single continuous count of days - including the
fractional part - from a fixed origin. The same question becomes one
subtraction of two doubles.

The origin is 1 January 4713 BC at noon, chosen in 1583 by Joseph
Scaliger. He wanted a starting point earlier than any recorded
historical event so that every usable date falls on a positive number.
Named after his father Julius Caesar Scaliger, not after Julius Caesar
or the Julian calendar.

Today is around JD 2,461,281.

## Why the Julian day starts at noon

Astronomers observe at night. If the date rolled over at midnight,
every observing session would span two dates and half of every logbook
entry would need two timestamps. Rolling over at noon keeps one night's
work inside one Julian day.

The practical consequence: midnight UTC always lands on a .5 boundary.
If you compute a JD for midnight and get a whole number, you have a
bug.

## Why March is treated as month 1

February changes length. If January is month 1, the variable-length
month sits at position 2 - in the middle of the year - so every formula
computing "days elapsed from the start of the year to month X" must
check whether February has been passed.

Treating March as month 1 moves February to the end, where it disturbs
nothing. Month lengths then form a regular repeating pattern that a
single fitted constant (30.6001) can reproduce.

This numbering is inherited from the Roman calendar, which is why
September (7), October (8), November (9) and December (10) have names
that no longer match their positions. They used to match.

## Why the Gregorian correction exists

The Julian calendar assumed a year of 365.25 days: one leap year every
four, no exceptions. The true tropical year is 365.2422 days. The
0.0078 day annual error accumulated to a full 10 days over roughly 1600
years, and Easter had drifted noticeably into the wrong season.

In 1582 Pope Gregory XIII did two things: deleted 10 days (4 October
was followed directly by 15 October), and changed the rule so that
century years are not leap years unless divisible by 400.

The expression `b = 2 - a + a/4` computes how many days to remove for a
given date so it aligns with the continuous Julian day count.

## Why fmod needs a sign correction

C's `fmod` keeps the sign of the dividend, not the divisor. So
`fmod(-10.0, 360.0)` returns -10.0, not 350.0, because the truncation
in `x - y * trunc(x/y)` rounds toward zero.

Python's `%` operator uses `floor` instead and returns 350. Many
astronomical algorithms are published as pseudocode assuming
Python-style behaviour. Translating them directly to C breaks silently
on every negative input - the code runs, the numbers look plausible,
and every angle is wrong by a full turn.

This is why `norm_360` has an explicit `if (result < 0.0)` correction.

## Why two normalisation ranges

Angles are cyclic; numbers are linear. Representing a circle with a
number line requires cutting the circle somewhere, and at that cut the
value jumps discontinuously. The seam never disappears - you only
choose where to put it.

The rule: put the seam where nothing important happens.

`[0, 360)` puts the seam at 0/360. Used for azimuth (measured from
north, and nothing special happens at north), right ascension, and mean
longitude.

`[-180, 180)` puts the seam at ±180. Used for hour angle, where zero is
the moment an object crosses the meridian - the highest point in its
path and the best moment to observe it. With `[0, 360)` the value would
jump from 359.9 to 0 exactly at the interesting moment. With
`[-180, 180)` it passes smoothly through zero, and the sign tells you
directly whether the object is still rising or already setting.

The same range gives shortest angular distance for free: `norm_180(a -
b)` returns the short way around the circle, with a sign for direction.
Naive subtraction gives 350 - 10 = 340 instead of the correct 20.

## Why double and not float

A Julian Date has 7 digits before the decimal point and needs roughly 5
after it for sub-second resolution. `float` provides about 7
significant digits total - it cannot distinguish 2451545.0 from
2451545.000012.

`double` provides 15-16, leaving room for error accumulation through
the pipeline: Julian Date, orbital elements, an iterative Kepler
solver, then three coordinate rotations. Each step rounds slightly.

The ESP32-S3's FPU handles single precision in hardware but emulates
double precision in software, roughly 10-50x slower. This is a real
cost on the target, accepted deliberately: correctness before
optimisation, and the workload is a handful of calculations per second
on a 240 MHz processor. If profiling later shows it matters, the fix is
targeted conversion of the hot path only, validated against JPL
reference data.

## Why binary floating point cannot be compared with ==

Most decimal fractions have no exact binary representation, the same
way 1/3 has no exact decimal representation. `0.1 + 0.2` evaluates to
0.30000000000000004, so `0.1 + 0.2 == 0.3` is false.

All numeric assertions in the test suite compare absolute difference
against a tolerance instead. The appropriate tolerance depends on the
quantity: 1e-9 for angles in degrees, a few arcminutes for validated
planet positions.