#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <limits.h>
#include <errno.h>

int main(void)
{
    char fullpath[PATH_MAX];
    const char *relative_path = "html/index.html";
    if (snprintf(fullpath, sizeof(fullpath), "%s/%s", realpath(".", NULL), relative_path) < 0)
    {
        perror("snprintf error");
        return EXIT_FAILURE;
    }
    if (access(fullpath, R_OK) != 0)
    {
        perror("Access error");
        return 404;
    }

    int fd = open(fullpath, O_RDONLY);

    if (fd == -1)
    {
        perror("Failed to open file");
        return 404;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == 0)
    {
        printf("Information about %s:\n", fullpath);
        printf("  File size: %lld bytes\n", (long long)file_info.st_size);
        printf("  Permissions: %o\n", file_info.st_mode & 0777); // Octal representation
        printf("  Last modification time: %ld\n", file_info.st_mtime);
    }
    else
    {
        perror("Error getting file information via fstat");
        close(fd);
        return 1;
    }

    char *absolute_path = realpath(fullpath, NULL);
    if (absolute_path != NULL)
    {
        printf("The real path of %s is: %s\n", fullpath, absolute_path);
        free(absolute_path); // Remember to free the allocated memory
    }
    else
    {
        perror("Error getting real path");
        return 1;
    }

    char buffer[1024];

    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

    printf("%s\n\n=== Bytes read: %ld\n", buffer, bytes_read);

    close(fd);
    return 0;
}