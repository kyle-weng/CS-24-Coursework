#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

const uint64_t MAX_CHOICE = 256;

void do_access(page_t *pages);

page_t *init_pages() {
    return calloc(MAX_CHOICE, sizeof(page_t));
}

void guess_accessed_page(page_t *pages) {
	int t1 = 0, t2 = 0;
	int idx = 256;
	double min_ratio = 100;
	page_t *p;

	// flush
	for (uint64_t i = 0; i < MAX_CHOICE; i++) {
		p = (page_t *)((uint8_t *)pages + i * sizeof(page_t));
		flush_cache_line((void *)p);
	}
    do_access(pages);
	for (uint64_t i = 0; i < MAX_CHOICE; i++) {
		p = (page_t *)((uint8_t *)pages + i * sizeof(page_t));

		t1 = time_read((void *)p);
		t2 = time_read((void *)p);

		if ((double)t1 / t2 < min_ratio) {
			//printf("changed\n");
			min_ratio = (double)t1 / t2;
			idx = (int)i;
		}
		printf("i\t%lu\tt1\t%d\tt2\t%d\n", i, t1, t2);
	}
	printf("I guess %d, which has a min_ratio of %f\n", idx, min_ratio);
}

int main() {
    page_t *pages = init_pages();
    guess_accessed_page(pages);
}
