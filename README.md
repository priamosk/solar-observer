# Solar Observer

An astronomical ephemeris engine written in C, computing planet
positions from Keplerian orbital elements. No astronomy API, no
external libraries — the orbital mechanics are implemented from
first principles and validated against NASA JPL Horizons.

Phase 0 of a handheld observation device built on an ESP32-S3 with a
touchscreen.

## What it computes

Given a date, a time, and an observer's coordinates, it answers where
each planet is in the sky right now.

    UTC
     └─ Julian Date
         └─ orbital elements at that instant
             └─ Kepler's equation, solved by Newton-Raphson
                 └─ position in the orbital plane
                     └─ three rotations into heliocentric ecliptic
                         └─ minus Earth's position → geocentric
                             └─ rotation by obliquity → RA / Dec
                                 └─ sidereal time → altitude / azimuth

Kepler's equation, `M = E - e·sin(E)`, has no closed-form solution —
Bruns proved in 1900 that none exists in elementary functions. It is
solved iteratively, converging in three or four steps.

## Accuracy

Validated against NASA JPL Horizons: 12 reference positions, seven
planets, four epochs spanning 1990–2049.

**Maximum error: 9.4 arcminutes. Median: 0.5 arcminutes.**

| Planet  | Error       | |
|---------|-------------|---|
| Mercury | 0.39'       | |
| Venus   | 0.43'       | |
| Mars    | 0.23–0.29'  | |
| Jupiter | 1.23–5.88'  | 5:2 resonance with Saturn |
| Saturn  | 4.99–9.42'  | 5:2 resonance with Jupiter |
| Uranus  | 0.26'       | |
| Neptune | 0.50'       | |

For scale: the full moon spans 30 arcminutes, and naked-eye resolution
is roughly 60. Every result places its planet comfortably inside the
centre of a binocular field.

Jupiter and Saturn dominate the error budget. Their orbital periods
sit in a near 5:2 ratio, so their mutual perturbations accumulate
systematically rather than averaging out — the Great Inequality. A
linear "value plus rate" element model cannot represent a periodic
perturbation. Saturn's worst case falls at the J2000 epoch itself
rather than at the extremes, confirming this is a limitation of the
mode
## Usage

```
./solar YYYY-MM-DD HH:MM LAT LON
```

All four arguments are required. Times are UTC. Latitude is positive
north, longitude positive east. There is no default location — on the
target hardware the observer's position comes from the GPS module or a
stored setting, never from a constant, and the tool mirrors that.

```
$ ./solar 2026-09-01 21:00 47.70 8.63

2026-09-01 21:00 UTC    JD 2461285.37500
Observer  47.7000N  8.6300E

Planet            RA       Dec  Altitude   Azimuth   Distance
               (deg)     (deg)     (deg)     (deg)       (AU)
---------------------------------------------------------------
Mercury      166.062     7.601   -22.937   315.171     1.3778
Venus        200.510   -12.436   -18.663   271.446     0.5477
Mars         104.804    23.286   -16.531    18.451     1.8451
Jupiter      136.164    17.266   -23.919   346.913     6.1876
Saturn        13.302     2.812    15.706   103.478     8.5932  up
Uranus        63.396    21.026    -2.616    54.243    19.3484
Neptune        3.550    -0.001    19.760   113.278    28.9548  up
```

Planets marked `up` are above the horizon.

## Building

```
make          build the command line tool
make test     build and run all nine test suites
make clean    remove build output
```

Requires gcc and make. No external dependencies.

## Testing

**4557 checks across nine suites.** Three kinds:

**Example tests** assert specific known values. `jd_from_utc(2000, 1,
1, 12, 0, 0)` must return exactly 2451545.0 — the J2000 epoch, by
definition.

**Property tests** assert rules that must hold whatever the numbers
are. Every eccentricity below 1. Each planet farther out than the
previous. The Sun–Earth–planet triangle inequality, checked for every
planet at 37 epochs. Orbits closing after one sidereal year.

**External validation** compares against JPL Horizons, which uses
numerical integration of the full gravitational problem rather than
Keplerian elements.

The distinction matters. The first 4545 checks verify the code is
self-consistent; none of them compares against reality. An error
affecting every calculation identically would pass all of them — if
the obliquity constant were 23.5 instead of 23.44, every test would
still pass and every planet would be 3.6 arcminutes wrong. Only the
external reference catches that.

Some behaviours the tests confirm, none of which were programmed
directly — they emerge from the orbital elements and the rotations:

- The Sun passes directly overhead at the equator on the equinox
- It reaches 66° at Zurich in June and 19° in December
- It never sets at the north pole in June, nor rises there in December
- Sydney's summer falls in December
## Layout

```
include/     public headers, one per module
src/         implementation
tests/       one standalone test binary per module
docs/        decision records, background, roadmap
```

| Module | Responsibility |
|---|---|
| `angles` | degree/radian conversion, normalisation |
| `julian` | calendar date to Julian Date |
| `orbital` | the eight-planet element table |
| `kepler` | Newton-Raphson solver |
| `vec3` | three-dimensional vectors |
| `position` | heliocentric and geocentric positions |
| `equatorial` | right ascension and declination |
| `horizon` | sidereal time, altitude and azimuth |

## Engineering constraints

Library code follows embedded discipline from the first commit, even
though Phase 0 runs on a laptop, so that porting to the
microcontroller requires no rewrite:

- **No dynamic allocation.** No `malloc`, no `free`, anywhere.
- **No I/O in library code.** `printf` appears only in `main.c` and
  in tests.
- **Lookup tables are `static const`** so they are placed in flash
  rather than RAM. The ESP32-S3 has megabytes of flash and a few
  hundred KB of RAM.
- **Every convergence loop has a hard iteration cap.** On a
  microcontroller an unbounded loop is not slow, it hangs the device —
  the watchdog reboots the chip and the user sees a restart with no
  diagnostic.
- **Errors travel as return codes**, never global state. Computed
  values are written through output parameters so the status is
  awkward to ignore.
- **Built with `-std=c11 -Wall -Wextra -Werror -pedantic`.** Warnings
  are fatal.

Full reasoning in [`docs/DECISIONS.md`](docs/DECISIONS.md); the
mathematics and history behind each choice in
[`docs/BACKGROUND.md`](docs/BACKGROUND.md).

## A note on what is not modelled

The output is the geometric position. Three effects are deliberately
omitted:

**Atmospheric refraction** lifts objects near the horizon by up to 34
arcminutes — slightly more than the Sun's own diameter, and why the
Sun appears to set several minutes after it geometrically has.
Refraction depends on temperature and pressure, both of which the
BME280 sensor will provide in Phase 5.

**Light travel time.** Jupiter is 35 to 50 light-minutes away, so we
see it where it was. The correction is a fraction of an arcminute,
below the accuracy of the element table.

**Topocentric parallax.** Positions are computed from Earth's centre
rather than the observer's location on the surface. Under an
arcsecond for planets. It would matter for the Moon, at roughly a
degree.

## Roadmap

Phase 0, the ephemeris engine, is complete. What follows runs on
hardware:

| Phase | |
|---|---|
| 1 | ESP32-S3 bring-up, LVGL on a capacitive touchscreen |
| 2 | Planet list and detail screens |
| 3 | Port the engine to the device, Wi-Fi, NTP time sync |
| 4 | Weather API, moon phase, observation quality scoring |
| 5 | BME280 for dew point, GPS for location and UTC |
| 6 | Enclosure, CI, demo |

The finished device answers one question: **is it worth going outside
tonight, and where should I look?**

Full task breakdown with acceptance criteria in
[`docs/ROADMAP.md`](docs/ROADMAP.md).