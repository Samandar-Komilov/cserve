#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mystrcmp(const char *first, const char *second);

int main(void)
{
    char *s1 = "Hello", *s2 = "Hello!";
    int a = strcmp(s1, s2);
    int b = mystrcmp(s1, s2);

    printf("Result: %d\n", a);
    printf("MyResult: %d\n", b);

    return 0;
}

int mystrcmp(const char *first, const char *second)
{
    int i = 0;
    while (*(first + i) != '\0' || *(second + i) != '\0')
    {
        int diff = first[i] - second[i];
        if (diff == 0)
        {
            i++;
            continue;
        }
        else
        {
            return diff;
        }
    }
}