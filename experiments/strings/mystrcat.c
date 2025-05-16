#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *mystrcat(char *dest, const char *src);

int main(void)
{
    char *s1 = (char *)malloc(30 * sizeof(char)), *s2 = (char *)malloc(30 * sizeof(char));
    strcpy(s1, "First");
    strcpy(s2, "Second");
    strcat(s1, s2);
    printf("Result with strcat: %s\n", s1);

    strcat(s1, "Third");
    printf("Result with mystrcat: %s\n", s1);

    free(s1);
    free(s2);
    return 0;
}

char *mystrcat(char *dest, const char *src)
{
    int i = 0, j = 0;
    while (*(dest + i) != '\0')
    {
        i++;
    }
    dest[i] = 0;

    while (*(src + j) != '\0')
    {
        dest[i + j] = src[j];
        j++;
    }
    dest[i + j] = '\0';

    return "";
}