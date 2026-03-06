#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/run/aged.sock"

/**
 * Queries the daemon for the current user's age bracket.
 * Returns 0-3 on success, -1 on error.
 */
int get_age_bracket() {
    int fd;
    struct sockaddr_un addr;
    char buffer[64];

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    send(fd, "GET", 3, 0);
    ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    close(fd);

    if (n > 0) {
        buffer[n] = '\0';
        if (strncmp(buffer, "BRACKET=", 8) == 0) {
            return atoi(buffer + 8);
        }
    }

    return -1;
}

/**
 * Requests the daemon to update the age bracket.
 * Requires the daemon to have root privileges.
 */
int set_age_bracket(int bracket) {
    int fd;
    struct sockaddr_un addr;
    char buffer[64];

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "SET %d", bracket);
    send(fd, buffer, strlen(buffer), 0);
    
    ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    close(fd);

    if (n > 0) {
        buffer[n] = '\0';
        return (strcmp(buffer, "OK") == 0) ? 0 : -1;
    }

    return -1;
}

// Example usage
/*
#include <age.h>
*/
int main() {
    int age = get_age_bracket();
    if (age == -1) printf("Could not determine age.\n");
    else printf("Age bracket: %d\n", age);
    return 0;
}

