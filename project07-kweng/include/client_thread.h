#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include "cache.h"

/* If you want verbose output on error,
 * #define VERBOSE. */

#ifdef VERBOSE
# define verbose_printf(...) printf(__VA_ARGS__)
#else
# define verbose_printf(...)
#endif

/* Given a clientfd, handles the HTTP request sent on
 * cfd and sends the result back on cfd */
void *handle_request(void *cfd);

#endif
