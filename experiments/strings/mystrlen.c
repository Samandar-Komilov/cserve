#include <stdio.h>
#include <string.h>

int mystrlen(char *str);

int main(void)
{
    char *str = "Hello World!";
    printf("Actual strlen: %d\n", mystrlen(str));
    printf("My strlen: %d\n", strlen(str));

    return 0;
}

int mystrlen(char *str)
{
    int ln = 0;
    while (*(str + ln) != '\0')
    {
        ln++;
    }
    return ln;
}