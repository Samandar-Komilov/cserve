/**
 * @file main.c
 * @brief Main entry point for the CServe (Nginx Clone) application.
 *
 * This file contains the main function which handles command-line arguments,
 * calculates the square root of a given number, and prints version information.
 * It demonstrates basic command-line parsing and mathematical operations.
 */

#include <stdio.h>
#include <stdlib.h> 
#include <math.h> 
#include <string.h> 
#include "cserve_config.h" 

/**
 * @brief The main function for the CServe application.
 *
 * This function processes command-line arguments. If no argument is provided,
 * it prints the application version and usage instructions. Otherwise, it
 * attempts to calculate and print the square root of the first argument.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @return 0 if the program executes successfully, 1 if there's an error
 * (e.g., missing argument).
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("%s Version %d.%d.%d\n", argv[0],
               Cserve_VERSION_MAJOR, Cserve_VERSION_MINOR, Cserve_VERSION_PATCH);
        printf("Usage: %s number\n", argv[0]);
        return 1;
    }

    double const inputValue = atof(argv[1]);

    double const outputValue = sqrt(inputValue);
    printf("The square root of %f is %f\n", inputValue, outputValue);
    return 0;
}
