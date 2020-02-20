/*
 * mm-bump.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

// note to self: this is 16
#define ALIGNMENT (2 * sizeof(size_t))

typedef struct {
    size_t header;
    /*
     * We don't know what the size of the payload will be, so we will
     * declare it as a zero-length array.  This allow us to obtain a
     * pointer to the start of the payload.
     */
    uint8_t payload[];
} block_t;

typedef struct node node;

typedef struct node {
	node* prev;
	node* next;
} free_node;

// Address of the payload of the last accessed free block of memory
free_node *last_node = NULL;

static size_t round_up(size_t size, size_t n) {
    return (size + n - 1) / n * n;
}

// ensures that the last bit (that is, is_allocated) is represented
static size_t get_size(block_t *block) {
    return block->header & ~0x1;
}

// last bit = is_allocated
static void set_header(block_t *block, size_t size, bool is_allocated) {
    block->header = size | is_allocated;
}

bool check_allocated(block_t *block) {
	return block->header & 0x1;
}

static void set_footer(block_t *block) {
	size_t *footer = (size_t *)((uint8_t *)block + get_size(block) - sizeof(size_t));
	*footer = get_size(block) | check_allocated(block);
}

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void) {
    /* Pad heap start so first payload is at ALIGNMENT. */
	mem_reset_brk();
	if ((long)mem_sbrk(ALIGNMENT - offsetof(block_t, payload)) < 0) {
        return -1;
    }
    return 0;
}

/* 
 * coalesce - Given a block to check, search for a contiguous chunk of
 * unallocated memory by checking the adjacent blocks and combine them into a
 * single block if applicable. Returns a pointer to the header of the coalesced
 * block.
 */
block_t *coalesce(block_t *b) {
	block_t *c;
	block_t *a;

	// Account for the case where b is the end of the heap
	if ((uintptr_t)b + get_size(b) >= (uintptr_t)mem_heap_hi()) {
		c = NULL;
	}
	else {
		c = (block_t *)((uint8_t *)b + get_size(b));
	}

	// Account for the case where b is the front of the heap
	if ((uintptr_t)b - get_size(b) <= (uintptr_t)mem_heap_lo() + ALIGNMENT - 
		offsetof(block_t, payload)) {
		a = NULL;
	}
	else {
		a = (block_t *)((uint8_t *)b - (*((size_t *)((uint8_t *)b - sizeof(size_t))) & ~0x1));
	}

	if (c != NULL) {
		if (!check_allocated(c)) {
			if (a != NULL) {
				if (!check_allocated(a)) {
					set_header(a, get_size(a) + get_size(b) + get_size(c), false);
					set_footer(a);
					return a;
				}
			}
			set_header(b, get_size(b) + get_size(c), false);
			set_footer(b);
			return b;
		}
	}
	if (a != NULL) {
		if (!check_allocated(a)) {
			set_header(a, get_size(a) + get_size(b), false);
			set_footer(a);
			return a;
		}
	}
	set_header(b, get_size(b), false);
	set_footer(b);
	return b;
}

block_t *find_fit(size_t size) {
	// First fit implementation
	uint8_t *heap_pointer = (uint8_t *)(mem_heap_lo() + ALIGNMENT - offsetof(block_t, payload));
	while ((uintptr_t)heap_pointer < (uintptr_t)mem_heap_hi()) {
		block_t *b = (block_t *)heap_pointer;
		if (get_size(b) >= size && !check_allocated(b)) {
			return b;
		}
		heap_pointer += get_size(b);
	}
	return NULL;
}

/*
 * split - Split a given block into two smaller blocks, with the first block
 * having size s. Returns a pointer to the first block.
 */
block_t* split(block_t *block, size_t s) {
	size_t original_size = get_size(block);
	size_t s_size = original_size - s;
	set_header(block, s, true);
	set_footer(block);
	if (s_size > 0) {
		block_t* new_block = (block_t*)((uintptr_t)block + s);
		set_header(new_block, s_size, false);
		set_footer(new_block);
	}
	return block;
}

/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */
void* malloc(size_t size) {
	size = round_up(sizeof(block_t) + size + sizeof(size_t), ALIGNMENT);
	block_t* block = find_fit(size);
	if (block == NULL) {
		block = mem_sbrk(size);
		if ((long)block < 0) {
			return NULL;
		}
		set_header(block, size, true);
		set_footer(block);
	}
	else {
		block = split(block, size);
	}
	return block->payload;
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void free(void *ptr) {
	if (ptr == NULL) {
		return;
	}
	block_t *block = (block_t *)((uint8_t *)ptr - offsetof(block_t, payload));
	block = coalesce(block);
	// Set the last accessed free_node's next to point to the payload of this
	// block.
	assert(!check_allocated(block));
	uint8_t *current_payload = (uint8_t *)block + sizeof(size_t);
	free_node* curr_node = (free_node *)current_payload;
	curr_node->prev = NULL;
	curr_node->next = NULL;
	if (last_node != NULL) {
		last_node->next = curr_node;
		//printf(" %p\n", last_node->next);
		curr_node->prev = last_node;
		/*printf("\n%p is the address of last_node\n", last_node);
		printf("%p is the address of last_node->next\n", last_node->next);
		printf("%p is the address of curr_node->prev\n", curr_node->prev);
		printf("%p is the address of the current payload\n", current_payload);
		assert((uintptr_t)last_node->next >= (uintptr_t)current_payload);*/
	}
	last_node = curr_node;
	return;
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.
 **/
void *realloc(void *old_ptr, size_t size) {
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        free(old_ptr);
        return NULL;
    }

    /* If old_ptr is NULL, then this is just malloc. */
    if (!old_ptr) {
        return malloc(size);
    }

    void *new_ptr = malloc(size);

    /* If malloc() fails, the original block is left untouched. */
    if (!new_ptr) {
        return NULL;
    }

    /* Copy the old data. */
    block_t *block = old_ptr - offsetof(block_t, payload);
    size_t old_size = get_size(block);
    if (size < old_size) {
        old_size = size;
    }
    memcpy(new_ptr, old_ptr, old_size);

    /* Free the old block. */
    free(old_ptr);

    return new_ptr;
}


/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc(size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *new_ptr = malloc(bytes);

    /* If malloc() fails, skip zeroing out the memory. */
    if (new_ptr) {
        memset(new_ptr, 0, bytes);
    }

    return new_ptr;
}

/*
 * mm_checkheap - So simple, it doesn't need a checker!
 */
void mm_checkheap(int verbose) {
	uint8_t* heap_pointer = (uint8_t*)(mem_heap_lo() + ALIGNMENT - offsetof(block_t, payload));
	while ((uintptr_t)heap_pointer < (uintptr_t)mem_heap_hi()) {
		block_t* b = (block_t*)heap_pointer;
		assert(get_size(b) > 0);
		assert(get_size(b) % ALIGNMENT == 0);
		heap_pointer += get_size(b);
	}
	(void)verbose;
}
