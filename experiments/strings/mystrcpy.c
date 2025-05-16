#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mystrcpy(char *dest, char *src);

int main(void)
{
    char *str = "Hi", *str2 = (char *)malloc(sizeof(10));
    strcpy(str2, str);
    printf("%s | %s\n", str, str2);

    mystrcpy(str2, "This is!");
    printf("%s | %s\n", str, str2);

    free(str2);

    return 0;
}

int mystrcpy(char *dest, char *src)
{
    int i = 0;
    while (*(src + i) != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return 0;
}