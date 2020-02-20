#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "bt.h"

int maybe(int a) {
	printf("i'm in maybe\n");
    int arr[5];
    (void) arr;
    backtrace();
    return a + 5;
}

int call_me(int a) {
	printf("i'm in call_me\n");
    int arr[100];
    return maybe(a + arr[0]);
}

int main() {
	printf("i'm in main\n");
    call_me(1);
}
