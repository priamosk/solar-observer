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
---

## Kepler's second law and why speed varies

A planet does not move at constant speed. Kepler found in 1609 that
the line from Sun to planet sweeps equal areas in equal times.

Near the Sun the radius is short, so covering the same area requires
travelling a long arc - fast. Far away the radius is long, so a short
arc suffices - slow. Earth moves at 30.3 km/s in January and 29.3 km/s
in July.

Everything that follows exists to handle this variation.

## The three anomalies

"Anomaly" here is an old astronomical term meaning simply "angle of
position". Three of them describe where a planet is, and converting
between them is the hard part.

**Mean anomaly M - the fictitious planet.** Imagine a planet moving at
constant speed around a perfect circle, completing one lap in the same
period as the real one. M is that planet's angle. It corresponds to
nothing physical - it is time disguised as an angle - which is why it
falls straight out of the element table as `M = L - w_bar` and
increases perfectly linearly.

**True anomaly v - the real angle.** The actual angle of the actual
planet, measured from the Sun. This is what we ultimately want, and it
does not relate simply to M.

**Eccentric anomaly E - the bridge.** A geometric construction. Draw a
circle around the ellipse with radius equal to the semi-major axis.
From the planet's position, move vertically until you meet that
circle. E is the angle of that point, measured from the CENTRE of the
ellipse rather than from the Sun. It has no physical meaning; it
exists because it makes the mathematics work.

## Why Kepler's equation cannot be solved

Kepler proved the equal-areas law leads to:

    M = E - e * sin(E)

Read as: the uniform angle equals the real angle minus a correction.
The `e * sin(E)` term is how far the planet runs ahead of or behind
the fictitious uniform one.

Sanity check: if e = 0 the orbit is a circle, the equation collapses
to M = E, and no correction is needed - which is correct, because
motion on a circle really is uniform.

We know M and need E. But E appears both linearly and inside a sine,
and no algebraic manipulation separates them. This is a transcendental
equation. Kepler struggled with it for years; in 1900 Bruns proved no
closed-form solution in elementary functions exists.

The solution is not undiscovered. It is impossible.

## Why Newton-Raphson

Since the equation cannot be solved, it is solved approximately.
Rewrite it so it equals zero:

    f(E)  = E - e * sin(E) - M
    f'(E) = 1 - e * cos(E)

Now find where f crosses zero. Guess an E; f(E) tells you how wrong
the guess is, and f'(E) tells you the slope. Follow the tangent line
to where it crosses zero and use that as the next guess:

    E_next = E - f(E) / f'(E)

Starting from E = M works well because for small eccentricity E is
already close to M.

Worked example, Earth (e = 0.0167, M = 1.0 rad):

    E0 = 1.000000
    E1 = 1.014181
    E2 = 1.014180   error below 1e-12, done

Three iterations.

## Why convergence is so fast

Newton-Raphson converges quadratically: the number of correct digits
doubles at every step.

    step 1:   2 correct digits
    step 2:   4
    step 3:   8
    step 4:  16   more than a double can hold

This is why four iterations exhaust double precision. The 30-iteration
limit in the code is not a working value - it is a safety net.

## Why every loop needs a hard iteration cap

Newton-Raphson can diverge. If the derivative `1 - e * cos(E)`
approaches zero, the division blows up and the next guess flies off.

For our planets this cannot happen: the largest eccentricity is
Mercury's 0.2056, so the derivative stays above 0.79 - comfortable
margin. The cap still exists, because on a microcontroller an
unbounded loop does not merely run slowly, it hangs the device.

There is no Ctrl+C on an ESP32. The screen freezes, the watchdog timer
reboots the chip, and the user sees a device restarting for no visible
reason. Every loop in this project has a provable upper bound.

## Why status is returned and data goes through a pointer

`kepler_solve` has two things to report: the answer, and whether it
succeeded. C returns one value, so one of them must travel through a
pointer.

The status is the return value because that makes it awkward to
ignore. Writing `E = solve(M, e)` and silently using a garbage E is
easy. Writing `solve(M, e, &E)` and not checking the result at least
looks wrong to a reader.

This pattern - status returned, data written through output
parameters - is standard in embedded C, and it is why so many
functions in this project return int rather than the value they
compute.
## Why the Kepler tests use inversion instead of reference values

Every other test suite in this project compares against known-correct
values. The Kepler solver cannot: there is no formula that produces a
correct E, which is exactly why the solver exists.

Instead the answer is substituted back into the original equation:

    residual = E - e * sin(E) - M

If the residual is essentially zero, E satisfies Kepler's equation and
is therefore correct by definition.

This is stronger than a table of precomputed values. A table only
verifies the inputs someone thought to tabulate; inversion verifies
every input the test happens to try, including the 24-step sweep
across a full revolution and eccentricities up to 0.95.

The residual tolerance is 1e-10 rather than the solver's own 1e-12,
because the solver stops when the STEP size falls below tolerance, and
the residual after that final step is a related but distinct quantity.
1e-10 radians is about 0.00002 arcseconds.
## Why vectors return by value here

Every other module in this project returns a status code and writes
results through pointers (ADR-005). The vector functions do not - they
take structs by value and return structs by value.

The rule exists for functions that can FAIL. `kepler_solve` can fail:
it iterates, and iteration can diverge. Subtracting two vectors
cannot fail. There is no error to report, so there is nothing for a
return code to carry.

A vec3_t is 24 bytes, which fits in registers on both x86-64 and the
ESP32's Xtensa core, so passing by value costs nothing in practice.
The gain is that the code reads as mathematics:

    vec3_t geocentric = vec3_sub(mars, earth);

rather than:

    vec3_t geocentric;
    vec3_sub(&mars, &earth, &geocentric);

## Why test values are asymmetric

The vector tests use (1,2,3) and (4,5,6) rather than (1,1,1) and
(2,2,2).

With identical components, a bug that writes b.y into the x slot
produces the correct answer by accident, and the test passes. With
distinct values in every slot, any component swap changes the result
and is caught immediately.

The general rule: choose test inputs that make mistakes visible. A
test that cannot fail is not a test.

## Why subtraction order is tested explicitly

`vec3_sub(a, b)` must compute a - b. Reversing it would place every
planet exactly opposite where it belongs.

That failure mode is dangerous specifically because the output still
looks reasonable - planets at plausible distances, moving at plausible
speeds, just on the wrong side of the sky. There is no crash and no
obviously wrong number. The test asserts both directions so the sign
convention is pinned down before Block C depends on it.
---

## From an ellipse to a position in space

The Kepler solver gives eccentric anomaly E. Turning that into a
position takes two stages: place the planet on a flat ellipse, then
rotate that ellipse into its real orientation in space.

### Stage one: position in the orbital plane

Ignore three dimensions for a moment. The orbit is a flat ellipse.
Put it in an x-y plane with the Sun at the focus and perihelion on the
positive x axis:

    x = a * (cos E - e)
    y = a * sqrt(1 - e^2) * sin E
    z = 0

**Why the -e term.** E is measured from the CENTRE of the ellipse, but
the Sun sits at a FOCUS, displaced from the centre by a*e. The -e
subtracts that displacement so the result is measured from the Sun.

**Why the sqrt(1 - e^2) factor.** The semi-minor axis is
b = a * sqrt(1 - e^2). The circumscribed circle used to define E has
radius a in every direction. Squashing that circle into the ellipse
means scaling the vertical dimension by exactly this factor.

**Sanity check.** With e = 0 these reduce to x = a*cos E, y = a*sin E,
which is a circle of radius a. Correct.

### Stage two: three rotations into space

The flat ellipse must be oriented correctly in space. Three rotations,
applied in a specific order:

1. Rotate by w (argument of perihelion) about the z axis - places
   perihelion correctly WITHIN the orbital plane, measured from the
   ascending node.
2. Rotate by i (inclination) about the x axis - tilts the plane. This
   is what first gives z a non-zero value.
3. Rotate by node about the z axis - swings the tilted plane so the
   ascending node points the right way.

**Note on w.** The element table publishes `w_bar`, longitude of
perihelion, not `w`, argument of perihelion. They differ by the node:

    w = w_bar - node

### Why the rotation order cannot be changed

Rotations do not commute. Applying them in a different order puts the
planet somewhere else entirely.

The order follows from how each angle is defined. Read it inward from
the final frame:

- `node` describes where the ascending node lies IN THE FINAL FRAME
- `i` describes tilt RELATIVE TO THE NODE
- `w` describes perihelion WITHIN THE TILTED PLANE

Each angle is defined relative to the result of the ones outside it,
so they are applied innermost first: w, then i, then node.

### The resulting coordinate system

Heliocentric ecliptic coordinates:

- Origin: the Sun
- x-y plane: the ecliptic, which is Earth's orbital plane
- x axis: toward the vernal equinox - the direction of the Sun as seen
  from Earth at the March equinox
- z axis: ecliptic north
- Units: astronomical units

This is the frame every planet position is computed in before being
converted to something an observer on the ground can use.

## Why position tests check physics rather than values

Block D will compare against JPL reference positions. Before that
exists, the position code is verified against physical laws that must
hold whatever the exact numbers are:

- Each planet stays between its known perihelion and aphelion
- Earth's z coordinate stays near zero, because the ecliptic IS
  Earth's orbital plane
- A planet returns to its starting point after one sidereal year
- Adjacent planets never cross orbits

These are property tests at their most useful. A wrong rotation order,
a swapped sign, a missing sqrt(1 - e^2) factor, or a degrees-radians
mix-up all push results outside these bounds, even though none of the
tests knows a single correct answer in advance.

The Earth distance check alone runs 73 samples across a year and would
catch most structural errors on its own.

## Why sines and cosines are precomputed

The three combined rotation formulas reference cos_node four times,
and each other trigonometric value two or three times. Calling cos()
each time would recompute identical values repeatedly.

On the host this is negligible. On the ESP32-S3, where
double-precision arithmetic is emulated in software rather than
executed by the FPU, six trigonometric calls becoming eighteen is
three times the work for no benefit.

It also reads better: the formulas show the structure of the rotation
rather than a wall of function calls.

## Why mean anomaly is normalised before the solver

Mercury's mean longitude today is roughly 39972 degrees - it has
completed 111 orbits since J2000.

sin() and cos() handle large arguments correctly in principle, but
they must first subtract 111 full turns, and precision is lost in that
reduction. Normalising M into [-180, 180) first hands the trigonometry
small numbers, where it is most accurate.

norm_180 is used rather than norm_360 because the Kepler solver starts
its first guess at E = M, and starting near zero converges marginally
faster than starting near 360.