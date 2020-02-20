#include <inttypes.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

const uint64_t MAX_CHOICE = 256;

char *secret = "secret!";

page_t *init_pages() {
    return calloc(MAX_CHOICE, sizeof(page_t));
}

void do_access(page_t *probe_array, size_t idx) {
    force_read(&probe_array[(int)secret[idx]]);
}

char guess_accessed_page(page_t *pages, size_t str_idx) {
	int t1 = 0, t2 = 0;
	int idx = 256;
	double min_ratio = 100;
	page_t *p;

	for (uint64_t i = 0; i < MAX_CHOICE; i++) {
		p = (page_t *)((uint8_t *)pages + i * sizeof(page_t));
		flush_cache_line((void *)p);
	}

    do_access(pages, str_idx);

	for (uint64_t i = 0; i < MAX_CHOICE; i++) {
		p = (page_t *)((uint8_t *)pages + i * sizeof(page_t));
		t1 = time_read((void *)p);
		t2 = time_read((void *)p);

		if ((double)t1 / t2 < min_ratio) {
			min_ratio = (double)t1 / t2;
			idx = (int)i;
		}
	}

	return (char)idx;
}

int main() {
    page_t *probe_array = init_pages();
    for (size_t i = 0; i < strlen(secret); i++) {
		char t = guess_accessed_page(probe_array, i);
		printf("%c", t);
	}
	printf("\n");
}
