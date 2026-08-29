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

test: test_angles test_julian
	./test_angles
	./test_julian
clean:
	rm -f solar test_angles test_julian

.PHONY: clean test test_julian