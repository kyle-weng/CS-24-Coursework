#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

#include "client_thread.h"
#include "buffer.h"
#include "cache.h"

#define BUFFER_SIZE 8192

static int open_client_fd(char *hostname, int port, int *err) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return -1;
    }

    /* Fill in the server's IP address and port */
    struct addrinfo *address;
    char port_str[sizeof("65535")];
    sprintf(port_str, "%d", port);
    *err = getaddrinfo(hostname, port_str, NULL, &address);
    if (*err != 0) {
        return -2;
    }

    /* Establish a connection with the server */
    bool success = connect(client_fd, address->ai_addr, address->ai_addrlen) >= 0;
    freeaddrinfo(address);
    return success ? client_fd : -1;
}

/* Writes a string to a file descriptor, returns whether successful */
static bool write_string(int fd, char *str) {
    return write(fd, str, strlen(str)) >= 0;
}

/* Sends a status message to client with the status line specified by
 * status with a message body described by msg.
 * Returns whether successful */
static bool send_status_code(int client_fd, char *status, char *msg) {
    char *format =
        "HTTP/1.0 %s\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>"
            "<head><title>%s</title></head>"
            "<body>%s</body>"
        "</html>";

    /* Fill out the response template and send it to the client */
    char response[strlen(format) + 2 * strlen(status) + strlen(msg)];
    sprintf(response, format, status, status, msg);
    return write_string(client_fd, response);
}

/* Opens connection to full_host and returns the file descriptor or
 * returns -1 on error */
static int open_server_connection(int client_fd, char *full_host) {
    int port;
    char *port_str = strchr(full_host, ':');
    if (port_str == NULL) {
        port = 80;
    }
    else {
        /* Host string was separated into hostname and port */
        full_host[port_str - full_host] = '\0';
        port = atoi(port_str + 1);
        if (port <= 0 || port > 65535) {
            verbose_printf("Malformed request string: Invalid port\n");
            return -1;
        }
    }

    /* Open connection to requested server */
    int server_error;
    int server_fd = open_client_fd(full_host, port, &server_error);
    if (server_fd == -1) {
        verbose_printf("open_client_fd error: %s\n", strerror(errno));
        return -1;
    }
    if (server_fd == -2) {
        switch (server_error) {
            case EAI_FAIL:
            case EAI_NONAME:
                /* Don't bother checking exit code, since we are returning error
                 * afterwards anyway */
                send_status_code(client_fd, "502 Bad Gateway",
                    "DNS could not resolve address.");
                return -1;
            case EAI_AGAIN:
                /* Don't bother checking exit code, since we are returning error
                 * afterwards anyway */
                send_status_code(client_fd, "502 Bad Gateway",
                    "DNS temporarily could not resolve address.");
                return -1;
            case EAI_NODATA:
                /* Don't bother checking exit code, since we are returning error
                 * afterwards anyway */
                send_status_code(client_fd, "502 Bad Gateway",
                    "DNS could has no network addresses for host.");
                return -1;
        }
        verbose_printf("getaddrinfo error: %s\n", gai_strerror(server_error));
        return -1;
    }

    return server_fd;
}

/* Send get request to server, returns whether successful */
static bool send_get_header(int server_fd, char *path) {
    return write_string(server_fd, "GET ") &&
        write_string(server_fd, path) &&
        write_string(server_fd, " HTTP/1.0\r\n");
}

/* Reads a line from fd until a \r\n is reached.  Returns an allocated
 * buffer that must be freed by the user containing the line read in upon
 * success.  Returns NULL on error. */
static buffer_t *read_full_line(int fd) {
    buffer_t *line = buffer_create(BUFFER_SIZE);

    /* Read until we reach \r\n */
    char c = '\0', last_c;
    do {
        last_c = c;
        ssize_t chars_read = read(fd, &c, sizeof(c));
        if (chars_read <= 0) {
            if (chars_read < 0) {
                /* Error occurred */
                verbose_printf("Read error: %s\n", strerror(errno));
            }
            buffer_free(line);
            return NULL;
        }

        buffer_append_char(line, c);
    } while (!(last_c == '\r' && c == '\n'));

    return line;
}

static bool starts_with(char *str, char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* Produces a GET header from client's GET header
 * Sets *full_host to the 'host:port' string specified in the GET
 * Sets *path to the part of the GET request after the port, excluding
 * the leading /
 * *full_host and *path must be freed by the user if function returns 0.
 * Returns whether successful. */
static bool make_get_header(int client_fd, char **full_host, char **path) {
    *full_host = NULL;
    *path = NULL;

    /* Read the first line (with the GET request) separately */
    buffer_t *buf = read_full_line(client_fd);
    if (buf == NULL) {
        verbose_printf("No request string\n");
        goto MALFORMED_ERROR;
    }

    /* Parse first line.  We are expecting one of a few cases:
     * GET http://<HOST>/[<PATH>[/]] HTTP/..
     * GET http://<HOST>:#..#/[<PATH>[/]] HTTP/..
     *
     * We reject any other request (and terminate the connection),
     * because we believe it to be malformed.
     */
    char *data = buffer_string(buf);
	char *saveptr;

	char *prefix = strtok_r(data, " ", &saveptr);
    char *url     = strtok_r(NULL, " ", &saveptr);
    char *version = strtok_r(NULL, " ", &saveptr);

	if (prefix == NULL || url == NULL || version == NULL || strtok_r(NULL, " ", &saveptr) != NULL) {
        verbose_printf("Malformed request string: GET requests have"
                       " three parts\n");
        goto MALFORMED_ERROR;
    }
    if (strcmp(prefix, "GET") != 0) {
        verbose_printf("Unsupported request string: This proxy only"
               " handles GET requests\n");
        goto NOT_IMPLEMENTED_ERROR;
    }
    if (!starts_with(version, "HTTP/")) {
        verbose_printf("Malformed request string: The third part of the"
               " GET request should be an HTTP version\n");
        goto MALFORMED_ERROR;
    }
    if (!starts_with(url, "http://")) {
        verbose_printf("Malformed request string: The URL of the request"
               " should start with 'http://'\n");
        goto MALFORMED_ERROR;
    }

    char *host = url + strlen("http://");
    /* Allocate path separately so we can free line buffer.
     * The path starts at the first '/' in the URL.
     * If there is no '/' (e.g. "http://ucla.edu"), the path is just "/". */
    char *path_start = strchr(host, '/');
    *path = strdup(path_start == NULL ? "/" : path_start);
    assert(*path != NULL);

    /* Copy host, so that we have a separate copy for later */
    if (path_start != NULL) {
        *path_start = '\0';
    }
    *full_host = strdup(host);
    assert(*full_host != NULL);
    printf("Handling Request: %s%s\n", *full_host, *path);

    buffer_free(buf);
    return true;

    MALFORMED_ERROR:
        /* Don't bother checking exit code, since we are returning error
         * afterwards anyway */
        send_status_code(client_fd, "400 Bad Request",
                "Invalid request sent to proxy.");
        goto ERROR;

    NOT_IMPLEMENTED_ERROR:
        /* Don't bother checking exit code, since we are returning error
         * afterwards anyway */
        send_status_code(client_fd, "501 Not Implemented",
                "Invalid request sent to proxy.");
        goto ERROR;

    ERROR:
        buffer_free(buf);
        free(*path);
        free(*full_host);
        return false;
}

/* To be called after make_get_header.  Reads the remaining headers from
 * clientfd and sends them to serverfd after modification as follows:
 *
 * All Keep-Alive headers are dropped
 * Connection headers have their value replaced with 'close'
 * Proxy-Connection headers have their value replaced with 'close'
 * If a Host header is not found, a Host header is added
 *
 * Returns whether successful
*/
static bool filter_rest_headers(int client_fd, int server_fd, char *host) {
    bool sent_host_header = false, sent_connection_header = false;
    while (true) {
        buffer_t *buf = read_full_line(client_fd);

        /* read_full_line() errored out */
        if (buf == NULL) {
            verbose_printf("Malformed header: Not terminated by new line\n");
            return false;
        }

        char *line = buffer_string(buf);
        /* Detect end of header (make sure we sent host line) */
        if (strcmp(line, "\r\n") == 0) {
            buffer_free(buf);
            break;
        }

        /* Remove Keep-Alive line */
        if (starts_with(line, "Keep-Alive:")) {
            buffer_free(buf);
            continue;
        }

        /* Deal with host line (if we recieve one */
        if (starts_with(line, "Host:")) {
            sent_host_header = true;
        }
        /* Connection: * -> Connection: close */
        else if (starts_with(line, "Connection:")) {
            line = "Connection: close\r\n";
            sent_connection_header = 1;
        }
        /* Proxy-Connection: * -> Proxy-Connection: close */
        else if (starts_with(line, "Proxy-Connection:")) {
            line = "Proxy-Connection: close\r\n";
        }

        /* Send line to server */
        bool success = write_string(server_fd, line);
        buffer_free(buf);
        if (!success) {
            return false;
        }
    }

    /* Done sending headers. Make sure the necessary headers were sent */
    if (!sent_host_header) {
        bool success = write_string(server_fd, "Host: ") &&
            write_string(server_fd, host) &&
            write_string(server_fd, "\r\n");
        if (!success) {
            return false;
        }
    }
    if (!sent_connection_header) {
        if (!write_string(server_fd, "Connection: close\r\n")) {
            return false;
        }
    }
    return write_string(server_fd, "\r\n");
}

/* Sends the server's response to the client.
 * Returns whether successful */
static bool send_response(int client_fd, int server_fd, char *host, char *path, cache_t *cache) {
	// construct the key-- the extra bit is for the null terminator
	char *key = malloc(strlen(host) + strlen(path) + 1);
	strcpy(key, host);
	strcat(key, path);

	/*
	 * If the given value is present in the cache, move it to the front of the
	 * queue and return true.
	 */
	node_t *node = contains(cache, key, get_lock(cache));
	if (node != NULL) {
		queue_update(cache, node);
		buffer_t *buffer = peek(cache);
		(void)buffer;
		return true;
	}

	buffer_t *buffer = buffer_create(BUFFER_SIZE);
	
	/* Loop until server sends an EOF */
    while (true) {
        uint8_t buf[BUFFER_SIZE];
        ssize_t bytes_read = read(server_fd, buf, sizeof(buf));
        if (bytes_read < 0) {
            verbose_printf("read error: %s\n", strerror(errno));
            return false;
        }

        /* Server sent EOF */
        if (bytes_read == 0) {
            return true;
        }

        ssize_t bytes_written = write(client_fd, buf, bytes_read);
        if (bytes_written < 0) {
            return false;
        }
		else {
			/*
			 * If the given value is not present in the hash table, add a 
			 * corresponding node to the hash table. Then, move the node that
			 * contains the value to the front of the queue.
			 */
			buffer_append_bytes(buffer, buf, bytes_written);
			node_t *node = cache_insert(cache, key, buffer);
			if (node != NULL) {
				queue_update(cache, node);
				buffer_t *buffer = peek(cache);
				(void)buffer;
			}
		}
    }
}

void *handle_request(void *cfd) {
	cache_t *cache = init_cache();

    int client_fd = *(int *) cfd;
    free(cfd);

    char *host = NULL, *path = NULL;
    if (!make_get_header(client_fd, &host, &path)) {
        goto CLIENT_ERROR;
    }

    /* Establish connection with requested server */
    int server_fd = open_server_connection(client_fd, host);
    if (server_fd < 0) {
        goto CLIENT_ERROR;
    }

    /* Send GET request to server */
    if (!send_get_header(server_fd, path)) {
        goto SERVER_ERROR;
    }

    /* Modify and send request headers to ensure no persistent connections and
     * ensure the presence of a Host header */
    if (!filter_rest_headers(client_fd, server_fd, host)) {
        verbose_printf("filter_rest_headers error: %s\n", strerror(errno));
        goto SERVER_ERROR;
    }

    /* Forward response from server to client, and store the response in the
     * cache if possible */
    if (!send_response(client_fd, server_fd, host, path, cache)) {
        verbose_printf("send_reponse error: %s\n", strerror(errno));
        /* Fall through, since we're done anyway */
    }

    close(server_fd);

    /* Close the write end of the client socket and wait for it to send EOF. */
    if (shutdown(client_fd, SHUT_WR) < 0) {
        verbose_printf("shutdown error: %s\n", strerror(errno));
        goto CLIENT_ERROR;
    }
    uint8_t discard_buffer[BUFFER_SIZE];
    if (read(client_fd, discard_buffer, sizeof(discard_buffer)) < 0) {
        verbose_printf("read error: %s\n", strerror(errno));
        goto CLIENT_ERROR;
    }
    close(client_fd);

    free(host);
    free(path);
    return NULL;

    SERVER_ERROR:
        verbose_printf("Error in writing to server\n");
        close(server_fd);

    CLIENT_ERROR:
        close(client_fd);
        free(host);
        free(path);
        return NULL;
}
