# Makefile for the solar observer project.

CC := gcc

CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -g

LDFLAGS := -lm

ASTRO_SRCS := src/position.c src/orbital.c src/kepler.c \
              src/angles.c src/julian.c src/vec3.c

solar: src/angles.c src/main.c
	$(CC) $(CFLAGS) src/angles.c src/main.c -o solar $(LDFLAGS)

test_angles: tests/test_angles.c src/angles.c
	$(CC) $(CFLAGS) tests/test_angles.c src/angles.c -o test_angles $(LDFLAGS)

test_julian: tests/test_julian.c src/julian.c
	$(CC) $(CFLAGS) tests/test_julian.c src/julian.c -o test_julian $(LDFLAGS)

test_orbital: tests/test_orbital.c src/orbital.c
	$(CC) $(CFLAGS) tests/test_orbital.c src/orbital.c -o test_orbital $(LDFLAGS)

test_kepler: tests/test_kepler.c src/kepler.c src/angles.c
	$(CC) $(CFLAGS) tests/test_kepler.c src/kepler.c src/angles.c -o test_kepler $(LDFLAGS)

test_vec3: tests/test_vec3.c src/vec3.c
	$(CC) $(CFLAGS) tests/test_vec3.c src/vec3.c -o test_vec3 $(LDFLAGS)

test_position: tests/test_position.c $(ASTRO_SRCS)
	$(CC) $(CFLAGS) tests/test_position.c $(ASTRO_SRCS) -o test_position $(LDFLAGS)

test_equatorial: tests/test_equatorial.c src/equatorial.c $(ASTRO_SRCS)
	$(CC) $(CFLAGS) tests/test_equatorial.c src/equatorial.c $(ASTRO_SRCS) -o test_equatorial $(LDFLAGS)

test: test_angles test_julian test_orbital test_kepler test_vec3 test_position test_equatorial
	./test_angles
	./test_julian
	./test_orbital
	./test_kepler
	./test_vec3
	./test_position
	./test_equatorial
clean:
	rm -f solar test_angles test_julian test_orbital test_kepler test_vec3 test_position test_equatorial

.PHONY: clean test