#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char argv[])
{
    printf("%d\n", argc);
    char *path    = "static/index.html";
    char *newpath = realpath("..", NULL);
    printf("%s\n", newpath);

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file");
        return 404;
    }

    char buffer[100];
    read(fd, buffer, sizeof(buffer));

    printf("%s\n", buffer);
    return 0;
}