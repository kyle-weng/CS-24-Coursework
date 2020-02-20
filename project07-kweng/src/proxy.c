#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_thread.h"
#include "cache.h"

/* Maximum number of connections to queue up */
#define LISTENQ 1024

static int open_listen_fd(int port) {
    /* Create a socket descriptor */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    int value = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
        return -1;
    }

    /* listen_fd will be an endpoint for all requests to port
       on any IP address for this host */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listen_fd, LISTENQ) < 0) {
        return -1;
    }

    return listen_fd;
}

static void cleanup(void) {
}

static void sigint_handler(int sig) {
    (void) sig;
    exit(0);
}

static void usage(char *program) {
    printf("Usage: %s <port>\n", program);
    exit(1);
}

int main(int argc, char *argv[]) {
    /* Ignore broken pipes */
    signal(SIGPIPE, SIG_IGN);
    /* Stop process when CTRL+C is pressed */
    signal(SIGINT, sigint_handler);

    if (argc != 2) {
        usage(argv[0]);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        usage(argv[0]);
    }

    /* Open listen socket */
    int listen_fd = open_listen_fd(port);
    if (listen_fd < 0) {
        perror("Listen error");
        return 1;
    }

    /* Register cleanup code to run at exit */
    if (atexit(cleanup) != 0) {
        printf("Could not register clean up function\n");
        return 1;
    }

    printf("Proxy listening on port %d\n", port);

	pthread_t tid;
    while (true) {
        int *cfd = malloc(sizeof(int));
        assert(cfd != NULL);
        *cfd = accept(listen_fd, NULL, NULL);
        if (*cfd == -1) {
            perror("Accept error");
            free(cfd);
            continue;
        }
		pthread_create(&tid, NULL, handle_request, cfd);
		pthread_detach(tid);
    }
}
