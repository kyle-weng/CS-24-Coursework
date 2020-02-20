#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bt.h"

bool is_stack_address(void *addr) {
    return (size_t) addr >> 40 == 0x7f;
}

void backtrace() {
	printf("i'm in backtrace\n");
    uint8_t arr[10];
	size_t n = 0;
	void *rbp = (void *)0x00000000004005df;
	arr[0] = (uint8_t)rbp;
    while (is_stack_address(rbp)) {
        // TODO (student): Figure out what %rip is and print it out in a loop
		void *rip = arr + n * 8;
        printf("0x%016lx\n", (size_t) rip);
    }
}
