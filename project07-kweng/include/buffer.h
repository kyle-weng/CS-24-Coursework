#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

/* Growable byte buffer */
typedef struct buffer_t buffer_t;

/* Allocate the data necessary for the buffer and returns it */
buffer_t *buffer_create(size_t initial_capacity);
/* Free the buffer */
void buffer_free(buffer_t *);
/* Get the underlying bytes of the buffer */
uint8_t *buffer_data(buffer_t *);
/* '\0'-terminate the buffer so it can be interpreted as a string */
char *buffer_string(buffer_t *);
/* Get vector length */
size_t buffer_length(buffer_t *);
/* Append char to buffer */
void buffer_append_char(buffer_t *, char c);
/* Append byte array to buffer */
void buffer_append_bytes(buffer_t *, uint8_t *bytes, size_t length);

#endif // BUFFER_H
