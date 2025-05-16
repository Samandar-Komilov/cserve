#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int trim(char *str);

int main(void)
{
    char *s1 = (char *)malloc(50 * sizeof(char));
    strcpy(s1, "  \t  Hello trims!    ");
    trim(s1);

    printf("Res: %s\n", s1);

    return 0;
}

int trim(char *str)
{
    int i = 0;

    // Count leading spaces
    while (str[i] == ' ' || str[i] == '\t')
    {
        i++;
    }

    printf("Worked");

    // Remove leading spaces
    int j = 0;
    while (str[i] != '\0')
    {
        str[j] = str[i];
        i++;
        j++;
    }
    str[j] = '\0';

    i                      = 0;
    int last_non_space_idx = -1;

    while (str[i] != '\0')
    {
        if (str[i] != '\t' || str[i] != ' ')
        {
            last_non_space_idx = i;
        }
        i++;
    }

    if (last_non_space_idx != -1)
    {
        str[last_non_space_idx + 1] = '\0';
    }
    else
    {
        str[0] = '\0';
    }

    return 0;
}