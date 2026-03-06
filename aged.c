#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>

#define SOCKET_PATH "/run/aged.sock"
#define AGE_FILE "/etc/age_bracket"
#define BUFFER_SIZE 128

/*
 * Age Brackets:
 * 0: Under 13
 * 1: 13-15
 * 2: 16-17
 * 3: 18+
 */

// Global mutex for thread-safe file access
pthread_mutex_t age_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Reads the age bracket from the system file.
 * Returns -1 if file doesn't exist or is invalid.
 */
int read_age_bracket() {
    int bracket = -1;
    pthread_mutex_lock(&age_mutex);
    FILE *f = fopen(AGE_FILE, "r");
    if (f) {
        if (fscanf(f, "%d", &bracket) != 1) {
            bracket = -1;
        }
        fclose(f);
    }
    pthread_mutex_unlock(&age_mutex);
    return bracket;
}

/**
 * Writes the age bracket to the system file.
 * Requires root privileges.
 */
int write_age_bracket(int bracket) {
    if (bracket < 0 || bracket > 3) return -1;
    
    pthread_mutex_lock(&age_mutex);
    FILE *f = fopen(AGE_FILE, "w");
    if (!f) {
        pthread_mutex_unlock(&age_mutex);
        return -1;
    }
    fprintf(f, "%d\n", bracket);
    fclose(f);
    pthread_mutex_unlock(&age_mutex);
    return 0;
}

/**
 * Handles individual client connections.
 */
void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return NULL;
    }
    buffer[n] = '\0';

    if (strncmp(buffer, "GET", 3) == 0) {
        int bracket = read_age_bracket();
        snprintf(buffer, sizeof(buffer), "BRACKET=%d", bracket);
        send(client_fd, buffer, strlen(buffer), 0);
    } 
    else if (strncmp(buffer, "SET ", 4) == 0) {
        int new_val = atoi(buffer + 4);
        if (write_age_bracket(new_val) == 0) {
            send(client_fd, "OK", 2, 0);
        } else {
            send(client_fd, "ERR_PERM", 8, 0);
        }
    }

    close(client_fd);
    return NULL;
}

int main() {
    int server_fd, *client_fd;
    struct sockaddr_un addr;

    // Check if we can write to /etc/age_bracket (usually requires root)
    if (access("/etc", W_OK) != 0) {
        fprintf(stderr, "Warning: Daemon likely lacks write permissions to /etc. Run as root.\n");
    }

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Allow users to connect to the socket
    chmod(SOCKET_PATH, 0666);

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Age Verification Daemon started on %s\n", SOCKET_PATH);

    while (1) {
        client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, NULL, NULL);
        if (*client_fd < 0) {
            free(client_fd);
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, client_fd) != 0) {
            perror("pthread_create");
            close(*client_fd);
            free(client_fd);
        } else {
            pthread_detach(thread);
        }
    }

    close(server_fd);
    return 0;
}
