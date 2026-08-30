/*
 * orbital.c - the eight-planet orbital element table.
 *
 * Values from JPL, "Approximate Positions of the Planets"
 * (E. M. Standish), keplerian elements valid 1800-2050.
 *
 * Each pair is (value at J2000, rate of change per Julian century).
 */

#include "orbital.h"

#include <stddef.h>   /* NULL */

/*
 * The table itself.
 *
 * `static` means this symbol is visible only inside orbital.c. Nothing
 * outside can reach the array directly - callers must go through
 * orbital_get(). That keeps the data read-only from the outside.
 *
 * `const` means the contents never change at runtime. This matters on
 * the microcontroller: const data is placed in flash memory rather
 * than RAM. The ESP32-S3 has megabytes of flash and only a few hundred
 * KB of RAM, so keeping a ~900 byte table out of RAM is worth doing
 * for free.
 *
 * The [PLANET_COUNT] size comes from the enum. Add a planet to the
 * enum and this array grows automatically.
 */
static const orbital_elements_t planets[PLANET_COUNT] = {
        /* Mercury. Fastest mover: 149472 degrees per century is one
     * complete orbit every 88 days. Also the most eccentric of the
     * eight at 0.2056, so its distance from the Sun varies by 40%. */
    {
        "Mercury",
        0.38709927,    0.00000037,     /* a,     AU        */
        0.20563593,    0.00001906,     /* e                */
        7.00497902,   -0.00594749,     /* i,     deg       */
      252.25032350, 149472.67411175,   /* L,     deg       */
       77.45779628,    0.16047689,     /* w_bar, deg       */
       48.33076593,   -0.12534081      /* node,  deg       */
    },

    /* Venus. Nearly circular orbit - eccentricity 0.0068 is the
     * lowest of any planet. */
    {
        "Venus",
        0.72333566,    0.00000390,
        0.00677672,   -0.00004107,
        3.39467605,   -0.00078890,
      181.97909950, 58517.81538729,
      131.60246718,    0.00268329,
       76.67984255,   -0.27769418
    },

    /* Earth. Semi-major axis is 1.000 AU by definition - the
     * astronomical unit was originally defined as Earth's mean
     * distance from the Sun.
     *
     * Inclination is essentially zero because the ecliptic - the
     * reference plane for all these elements - IS Earth's orbital
     * plane. Earth cannot be inclined to its own plane.
     *
     * Node rate is 0 for the same reason: an orbit with no
     * inclination has no meaningful crossing point. */
    {
        "Earth",
        1.00000261,    0.00000562,
        0.01671123,   -0.00004392,
       -0.00001531,   -0.01294668,
      100.46457166, 35999.37244981,
      102.93768193,    0.32327364,
        0.0,           0.0
    },

    /* Mars. Eccentricity 0.0934 is high enough that opposition
     * distance varies significantly - which is why some Mars
     * oppositions are far better for observing than others. */
    {
        "Mars",
        1.52371034,    0.00001847,
        0.09339410,    0.00007882,
        1.84969142,   -0.00813131,
       -4.55343205, 19140.30268499,
      -23.94362959,    0.44441088,
       49.55953891,   -0.29257343
    },
        /* Jupiter. Contains more mass than all other planets combined.
     * Its 11.86 year orbit means L_rate is only ~3034 deg/century -
     * a fiftieth of Mercury's. */
    {
        "Jupiter",
        5.20288700,   -0.00011607,
        0.04838624,   -0.00013253,
        1.30439695,   -0.00183714,
       34.39644051,  3034.74612775,
       14.72847983,    0.21252668,
      100.47390909,    0.20469106
    },

    /* Saturn. The a_rate of -0.00125060 is the largest drift in the
     * table - Jupiter's gravity tugs on Saturn noticeably. Their
     * orbital periods sit near a 5:2 resonance, so the tugs
     * accumulate instead of averaging out. */
    {
        "Saturn",
        9.53667594,   -0.00125060,
        0.05386179,   -0.00050991,
        2.48599187,    0.00193609,
       49.95424423,  1222.49362201,
       92.59887831,   -0.41897216,
      113.66242448,   -0.28867794
    },

    /* Uranus. First planet discovered by telescope (1781). At 84
     * years per orbit, it has completed fewer than three full laps
     * since then. */
    {
        "Uranus",
       19.18916464,   -0.00196176,
        0.04725744,   -0.00004397,
        0.77263783,   -0.00242939,
      313.23810451,   428.48202785,
      170.95427630,    0.40805281,
       74.01692503,    0.04240589
    },

    /* Neptune. Found in 1846 by mathematics before observation:
     * Uranus kept drifting off its predicted path, Le Verrier
     * computed where a disturbing planet must be, and Neptune was
     * found within one degree of the prediction. The same technique
     * this project implements - Keplerian elements plus
     * perturbation rates - is what made that possible. */
    {
        "Neptune",
       30.06992276,    0.00026291,
        0.00859048,    0.00005105,
        1.77004347,    0.00035372,
      -55.12002969,   218.45945325,
       44.96476227,   -0.32241464,
      131.78422574,   -0.00508664
    }
};
/*
 * Look up one planet's elements.
 *
 * The bounds check matters. planet_id_t is just an int underneath -
 * nothing stops a caller passing 42 or -1. Without the check, that
 * would read memory past the end of the array: no crash, no warning,
 * just garbage numbers flowing into every calculation downstream.
 * Reading out of bounds is undefined behaviour, one of C's most
 * dangerous failure modes precisely because it usually LOOKS like it
 * works.
 *
 * Returning NULL forces the caller to notice. A caller that ignores
 * it crashes immediately on dereference - loud and close to the
 * mistake, which is exactly what we want. Quiet corruption far from
 * the mistake is what we are avoiding.
 */
const orbital_elements_t *orbital_get(planet_id_t id)
{
    if (id < 0 || id >= PLANET_COUNT) {
        return NULL;
    }

    return &planets[id];
}