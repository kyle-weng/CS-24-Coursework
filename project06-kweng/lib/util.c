#include <x86intrin.h>

#include "util.h"

void force_read(const void *p) {
    *(volatile char *) p;
}

void flush_cache_line(const void *memory) {
    _mm_clflush(memory);
    _mm_mfence();
    for (volatile int i = 0; i < 10000; i++) {}
}

uint64_t time_read(const void *memory) {
    uint64_t start = __rdtsc();
    _mm_lfence();
    force_read(memory);
    _mm_mfence();
    _mm_lfence();
    uint64_t result = __rdtsc() - start;
    _mm_mfence();
    _mm_lfence();
    return result;
}