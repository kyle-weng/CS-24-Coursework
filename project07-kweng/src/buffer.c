#include "buffer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct buffer_t {
    uint8_t *data;
    size_t length;
    size_t capacity;
};

/* Resizes buffer to have given new capacity */
static void buffer_resize(buffer_t *buf, size_t new_capacity) {
    if (new_capacity <= buf->capacity) {
        /* Already large enough */
        return;
    }

    /* Grow by at least a factor of 2 */
    size_t grown_capacity = buf->capacity * 2;
    if (new_capacity < grown_capacity) {
        new_capacity = grown_capacity;
    }

    buf->data = realloc(buf->data, sizeof(uint8_t[new_capacity]));
    assert(buf->data != NULL);
    buf->capacity = new_capacity;
}

buffer_t *buffer_create(size_t initial_capacity) {
    buffer_t *buf = malloc(sizeof(*buf));
    assert(buf != NULL);

    if (initial_capacity == 0) {
        initial_capacity = 1;
    }
    buf->data = NULL;
    buf->capacity = 0;
    buffer_resize(buf, initial_capacity);
    buf->length = 0;
    return buf;
}

void buffer_free(buffer_t *buf) {
    if (buf == NULL) {
        return;
    }

    free(buf->data);
    free(buf);
}

uint8_t *buffer_data(buffer_t *buf) {
    assert(buf != NULL);

    return buf->data;
}

char *buffer_string(buffer_t *buf) {
    buffer_resize(buf, buffer_length(buf) + 1);
    buf->data[buf->length] = '\0';
    return (char *) buf->data;
}

size_t buffer_length(buffer_t *buf) {
    assert(buf != NULL);

    return buf->length;
}

void buffer_append_char(buffer_t *buf, char c) {
    buffer_append_bytes(buf, (uint8_t *) &c, 1);
}

void buffer_append_bytes(buffer_t *buf, uint8_t *bytes, size_t length) {
    buffer_resize(buf, buffer_length(buf) + length);

    /* memcpy is safe here since we've ensured our buffer is large enough */
    memcpy(buf->data + buf->length, bytes, length);
    buf->length += length;
}
