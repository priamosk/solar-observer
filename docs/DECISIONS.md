# Architecture decision records

Each entry records a decision, the alternatives considered, and the
reasoning. Decisions are never deleted - if one is reversed, a new
entry supersedes it.

---

## ADR-001 — Target platform: ESP32-S3, not Raspberry Pi

**Status:** Accepted

**Context.** The device needs a touchscreen UI, Wi-Fi for weather data,
and enough compute for orbital mechanics.

**Decision.** ESP32-S3 with integrated capacitive touchscreen and
PSRAM, programmed with ESP-IDF in C.

**Alternatives.** A Raspberry Pi running Python would make the project
trivially easy, and teach neither C nor embedded systems. A Pi running
C is better, but it is full Linux - no memory constraints worth
respecting, no peripheral registers, no RTOS. That is Linux
application programming.

**Consequences.** Real memory constraints, FreeRTOS underneath, and a
mainstream commercial toolchain. ESP-IDF provides a HAL, so this
project teaches using an RTOS rather than writing one - the correct
trade for a first serious embedded project.

---

## ADR-002 — Astronomy computed on-device, not fetched from an API

**Status:** Accepted

**Decision.** Compute planet positions locally in C from Keplerian
orbital elements.

**Reasoning.** This is the technical core of the project. An API call
is twenty lines of HTTP; an ephemeris engine is real numerical
software with an iterative solver and a chain of coordinate
transformations. It also means the device works without internet,
which matters for a field instrument.

**Consequences.** Significantly more work, and accuracy must be
validated rather than assumed.

---

## ADR-003 — Phase 0 developed on the host, not on hardware

**Status:** Accepted

**Decision.** Write and validate the ephemeris engine with gcc on the
development machine. Port to the ESP32 in Phase 3.

**Reasoning.** Instant compile times, a real debugger, and unit tests
that can run in CI. Flashing hardware to test a maths bug wastes
minutes per iteration. Separating pure logic from hardware-dependent
code is also the only way to get meaningful test coverage in an
embedded project.

---

## ADR-004 — Accuracy validated against JPL Horizons

**Status:** Accepted

**Decision.** Correctness is measured against reference positions from
NASA's JPL Horizons system, across multiple planets and dates.
Maximum angular error is reported in the README.

**Reasoning.** "It looks about right in Stellarium" is not
verification. A published error figure against an authoritative source
is.

**Target.** Better than 5 arcminutes for all planets, 1800-2050.

---

## ADR-005 — Embedded-style constraints from day one

**Status:** Accepted

**Decision.** Library code follows embedded discipline even while
developing on the host:

- No dynamic allocation. No malloc, no free.
- No I/O in library code. No printf outside main.c and tests.
- Lookup tables are `static const` so they land in flash, not RAM.
- Fixed iteration caps on every convergence loop.
- Errors reported via return codes, never global state.

**Reasoning.** Code written without these constraints has to be
rewritten to run on a microcontroller. Code written with them ports
unchanged.

---

## ADR-006 — Warnings are errors

**Status:** Accepted

**Decision.** Build with `-std=c11 -Wall -Wextra -Werror -pedantic`.

**Reasoning.** C will happily compile code with undefined behaviour.
Most beginner mistakes are caught by warnings. Making them fatal means
bad habits never form.

---

## ADR-007 — Minimal in-house test harness

**Status:** Accepted

**Decision.** A small `check()` helper per test file rather than
vendoring Unity.

**Reasoning.** Adding a third-party framework before the first
function exists is complexity for its own sake. Writing the check
ourselves teaches floating-point comparison, which we need anyway.

**Consequences.** No test discovery or fixtures. Acceptable at this
size; the pain of managing it manually is what will motivate the
switch later.

---

## ADR-008 — Orbital