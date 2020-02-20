#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define DEFAULT_SIZE 3

typedef struct arrayintlist_t {
	size_t size;
	int* data;
} IntArray;

int main() {
	IntArray* arr = (IntArray*)malloc(sizeof(IntArray));
	arr->data = (int*)malloc(DEFAULT_SIZE * sizeof(int));
	for (size_t i = 0; i < 3; i++) {
		arr->data[i] = i + 1;
	}
	arr->size = DEFAULT_SIZE;
	return 0;
}
