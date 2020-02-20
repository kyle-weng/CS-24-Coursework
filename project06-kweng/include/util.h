#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

#define PAGE_SIZE 4096

typedef uint8_t page_t[PAGE_SIZE];


// Forces a memory read of the byte at address p. This will result in the byte
// being loaded into cache.
void force_read(const void *p);

// Flushes the cache line containing the provided address
void flush_cache_line(const void *memory);

// Returns the number of clocks taken to read the provided byte of memory.
uint64_t time_read(const void *memory);

#endif /* _UTIL_H */
