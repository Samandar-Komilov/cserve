#include <stdio.h>
#include <stdlib.h> // Required for atof
#include <math.h>   // Required for sqrt
#include <string.h> // Not used in this snippet, but often useful
#include "cserve_config.h" // Include the generated header file

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        // Now using Cserve_VERSION_MAJOR, MINOR, and PATCH
        printf("%s Version %d.%d.%d\n", argv[0],
               Cserve_VERSION_MAJOR, Cserve_VERSION_MINOR, Cserve_VERSION_PATCH);
        printf("Usage: %s number\n", argv[0]);
        return 1;
    }

    // convert input to double
    double const inputValue = atof(argv[1]);

    // calculate square root
    double const outputValue = sqrt(inputValue);
    printf("The square root of %f is %f\n", inputValue, outputValue);
    return 0;
}