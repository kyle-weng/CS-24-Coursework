#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

size_t get_kernel_data_length() {
    size_t i, j;

    FILE *f = fopen("/sys/kernel/kernel_data/length", "r");
    j = fscanf(f, "%zu", &i);
    (void)j;
    fclose(f);
    return i;
}

void *get_kernel_data_address() {
    long unsigned int p;
    size_t j;

    FILE *f = fopen("/sys/kernel/kernel_data/address", "r");
    j = fscanf(f, "%lx\n", &p);
    (void)j;
    fclose(f);
    return (void *)p;
}

int main() {
    size_t len = get_kernel_data_length();
    long unsigned int addr = (long unsigned int)get_kernel_data_address();

    printf("length is %zu\n", len);
    printf("address is %lx\n", addr);
}
