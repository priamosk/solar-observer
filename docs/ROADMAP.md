# Roadmap

Tick a box only when its acceptance criterion is met — not when the code
compiles, and not when it "looks right". Every item below has a specific
condition that says it is done.

---

## Phase 0 — ephemeris engine (host only, no hardware)

### Block A — foundations

- [x] A1 Toolchain, project layout, Makefile
      *Done when:* `make` builds without warnings under `-Werror`
- [x] A2 Angle conversion and normalisation
      *Done when:* 15 checks pass in `test_angles`
- [x] A3 Julian Date conversion
      *Done when:* `jd_from_utc(2000,1,1,12,0,0)` returns exactly 2451545.0
- [x] A4 Test harness
      *Done when:* `make test` runs both suites and reports 0 failures

### Block B — orbital data and the Kepler solver

- [x] B1 `orbital_elements_t` struct and the eight-planet table
      *Done when:* Earth's semi-major axis reads 1.000 AU from the table
- [x] B2 Kepler equation solver (Newton-Raphson)
      *Done when:* for 100 random (M, e) pairs, `E - e*sin(E) - M` < 1e-10
- [x] B3 `vec3_t` type and vector helpers
      *Done when:* magnitude of (3,4,0) returns exactly 5.0

### Block C — the coordinate pipeline

- [ ] C1 Heliocentric ecliptic position
      *Done when:* Earth's distance from the Sun stays within 0.983–1.017 AU
      across a full year
- [ ] C2 Geocentric position (subtract Earth)
      *Done when:* Earth-to-Earth distance computes as 0
- [ ] C3 Equatorial coordinates (RA / Dec)
      *Done when:* the Sun's declination reads about +23.4 deg at the June
      solstice and -23.4 deg in December
- [ ] C4 Sidereal time, altitude and azimuth
      *Done when:* results match Stellarium for Zurich to within 1 degree

### Block D — validation and tooling

- [ ] D1 Command-line tool taking date, latitude, longitude
      *Done when:* `./solar 2026-08-29 47.7 8.6` prints all eight planets
- [ ] D2 JPL Horizons reference dataset
      *Done when:* 20 reference positions across different planets and dates
      all agree to better than 5 arcminutes

**Phase 0 exit criterion:** maximum error against JPL published in the README.
No hardware is ordered before this is met.

---

## Phase 1 — display bring-up

- [ ] Order the ESP32-S3 touchscreen board
- [ ] Install ESP-IDF and the VS Code extension
- [ ] Flash the vendor hello-world example
      *Done when:* something appears on the screen
- [ ] Integrate LVGL
      *Done when:* a button responds to touch

## Phase 2 — static UI

- [ ] `planet_info_t` struct and data table (separate from orbital elements)
      *Done when:* all eight planets have complete factual data
- [ ] Planet list screen
- [ ] Planet detail screen with static facts
- [ ] Navigation between screens
      *Done when:* all eight planets are reachable and back works
## Phase 3 — port the ephemeris

- [ ] Migrate Make to CMake for ESP-IDF
- [ ] Compile Phase 0 code unchanged on the ESP32
      *Done when:* the test suite passes on-device
- [ ] Wi-Fi connection and NTP time sync
      *Done when:* the clock is correct after a cold boot
- [ ] Live positions shown in the UI

## Phase 4 — weather and the observation verdict

- [ ] HTTP client and JSON parsing
- [ ] Open-Meteo cloud cover for the observer's location
- [ ] Moon phase and moon altitude
- [ ] Combined visibility score per planet
      *Done when:* the device says "Saturn: good, 21:40–02:15, 38 deg, 15%
      cloud"

## Phase 5 — sensors and persistence

- [ ] BME280 over I2C, dew point calculation
- [ ] GPS over UART for location and UTC
- [ ] DS3231 real-time clock as a fallback
- [ ] Settings saved across reboots

## Phase 6 — finish

- [ ] Enclosure
- [ ] README with architecture diagram and accuracy figures
- [ ] GitHub Actions running the host tests on every push
- [ ] Demo video

---

## Rules

1. Nothing gets ticked without its acceptance criterion met.
2. `make test` must pass before every commit.
3. No hardware is ordered until Phase 0 exits.
4. Every non-obvious decision goes in `DECISIONS.md`.
5. Every piece of reasoning worth remembering goes in `BACKGROUND.md`.

---

## Ideas parked for later

**Planet info data (Phase 2).** Encyclopedic facts for the detail
screen, kept in a separate struct from the orbital elements. Orbital
elements are computational input; these are display strings. Mixing
them would drag description text through every Kepler iteration.

Fields to include:
- diameter, and ratio to Earth
- mass, and ratio to Earth
- surface gravity
- mean temperature, and min/max where meaningful
- atmospheric composition
- length of day, length of year
- number of moons
- distance from the Sun in AU
- one or two lines of description