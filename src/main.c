/*
 * main.c - temporary entry point for testing the angle functions.
 */

#include <stdio.h>

#include "angles.h"

int main(void)
{
    printf("Angle test\n");
    printf("==========\n");

    printf("deg2rad(180.0)  = %f\n", deg2rad(90.0));
    printf("rad2deg(3.1416) = %f\n", rad2deg(3.1416));
    printf("norm_360(-10.0) = %f\n", norm_360(-10.0));
    printf("norm_180(350.0) = %f\n", norm_180(350.0));

    return 0;
}