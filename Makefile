# Makefile for the solar observer project.

CC := gcc

CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -g

LDFLAGS := -lm

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

test: test_angles test_julian test_orbital test_kepler
	./test_angles
	./test_julian
	./test_orbital
	./test_kepler
clean:
	rm -f solar test_angles test_julian test_orbital test_kepler

.PHONY: clean test