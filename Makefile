# Makefile for the solar observer project.

CC := gcc

CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -g

LDFLAGS := -lm

solar: src/angles.c src/main.c
	$(CC) $(CFLAGS) src/angles.c src/main.c -o solar $(LDFLAGS)

clean:
	rm -f solar

.PHONY: clean