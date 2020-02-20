#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

const uint64_t REPEATS = 10000;

int main() {
    page_t *page = calloc(1, sizeof(page_t));
	uint64_t m;
	int n1 = 0, n2 = 0;

	for (uint64_t i = 0; i < REPEATS; i++) {
		// L3 cache miss (accomplished by flushing the cache)
		flush_cache_line((void *)page);
		m = time_read((void *)page);
		n1 += m;

		// L3 cache hit (page should already be in cache)
		m = time_read((void *)page);
		n2 += m;
	}

    printf("ratio = %f\n", (float)n2/n1);
    free(page);
}
