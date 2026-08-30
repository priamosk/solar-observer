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
---

## Why six numbers describe an orbit

An orbit is an ellipse positioned in three-dimensional space.
Describing one completely takes exactly six values - no fewer, no
more. They fall into three groups:

**Shape** (2 values)
- `a`, semi-major axis: the size, in astronomical units. One AU is
  Earth's mean distance from the Sun, about 150 million km.
- `e`, eccentricity: how flattened. Zero is a perfect circle. Near 1
  is highly elongated, like a comet. At 1 or above the orbit is open -
  the object leaves and never returns, so it is not a planet.

**Orientation** (3 values)
- `i`, inclination: how tilted the orbital plane is relative to
  Earth's. All eight planets sit within about 7 degrees, which is why
  they always appear in the same narrow band of sky - the zodiac.
- `node`, longitude of ascending node: where the orbit crosses Earth's
  plane heading north.
- `w_bar`, longitude of perihelion: which direction the closest point
  of the orbit points.

**Position** (1 value)
- `L`, mean longitude: where the planet actually is along the orbit at
  a given instant.

The first five are geometry and change only through slow
gravitational perturbation. The sixth is the planet moving.

## Why every element is a pair

No orbital element is constant. Planets tug on each other
gravitationally, deforming orbits over centuries. So each element is
published as a value at the J2000 epoch plus a rate of change per
Julian century:

    value_now = value_at_J2000 + rate * centuries_since_j2000(jd)

The first five drift by fractions of a degree per century. Mean
longitude is different in kind - it is not drift, it is orbital
motion:

| Planet  | deg/century | orbits/century | year       |
|---------|-------------|----------------|------------|
| Mercury | 149472      | 415            | 88 days    |
| Earth   | 35999       | 100            | 365 days   |
| Mars    | 19140       | 53             | 687 days   |
| Jupiter | 3035        | 8.4            | 11.9 years |
| Neptune | 218         | 0.6            | 165 years  |

Earth completes exactly 100 orbits per century, which is not a
coincidence - the year is defined by that motion.

## Why Earth's row contains zeros

Earth's inclination is essentially zero and its node rate is exactly
zero, because the reference plane for all these elements - the
ecliptic - IS Earth's orbital plane. Earth cannot be inclined relative
to itself, and an orbit with no inclination has no crossing point to
measure a node from.

Earth's semi-major axis is 1.000 AU for the same reason: the
astronomical unit was originally defined as Earth's mean distance from
the Sun.

Earth is the origin of the whole coordinate system.

## Why perihelion precession matters historically

The `w_bar_rate` values mean each orbital ellipse slowly rotates in
its own plane. Mercury's precession did not match Newtonian gravity -
there was an unexplained residue of 43 arcseconds per century.
General relativity accounted for it exactly, and this was its first
observational confirmation.

Neptune has the opposite story. Uranus kept drifting off its predicted
path, so Le Verrier computed where a disturbing body must be. Neptune
was found within one degree of the prediction in 1846. The technique -
Keplerian elements plus perturbation rates - is exactly what this
project implements.

## Why static const on the element table

`static` limits the symbol's visibility to orbital.c. No other file
can reach the array directly; everything goes through `orbital_get()`.
If the storage layout ever changes, only one file changes.

`const` means the contents never change at runtime. On the ESP32-S3
this is not cosmetic: const data is placed in flash rather than RAM.
The chip has megabytes of flash and only a few hundred KB of RAM, so
keeping a ~900 byte table out of RAM costs nothing and gains
something.

## Why orbital_get returns a pointer, not a copy

The struct is around 104 bytes. Returning it by value would copy all
of it on every call, and the Kepler pipeline calls this repeatedly.
Returning `const orbital_elements_t *` hands back an address instead.
On the microcontroller the caller then reads directly out of flash,
with nothing copied into RAM at all.

The `const` in the return type is the compiler enforcing that callers
cannot modify the table through the pointer they were given.

## Why the bounds check returns NULL

`planet_id_t` is an int underneath. Nothing stops a caller passing -1
or 99. Without the check, C would read memory past the end of the
array - no crash, no warning, just whatever bytes happen to be there
flowing into every downstream calculation.

This is undefined behaviour, and it is dangerous precisely because it
usually appears to work. Returning NULL forces the caller to notice; a
caller that ignores it crashes immediately on dereference, loud and
close to the mistake. Quiet corruption far from the mistake is what we
are avoiding.

## Property tests versus example tests

There are two kinds of check in this project's test suites.

**Example tests** assert a specific input gives a specific output:
`norm_360(-10)` must return 350. These catch the bug you anticipated.

**Property tests** assert a rule that must always hold: every
eccentricity must be below 1, every planet must be farther out than
the one before, every mean longitude rate must be positive. These
catch bugs you did not anticipate.

For a hand-typed table of 104 numbers, property tests are the only
realistic defence. A misplaced decimal point turning Saturn's
eccentricity from 0.054 into 0.534 would pass every example test that
does not happen to check that exact field - but it fails the property
check immediately, because an eccentricity above 1 describes an object
leaving the solar system.