/*! \file
 * Manages references to values allocated in a memory pool.
 * Implements reference counting and garbage collection.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2010.
 * All rights reserved.
 */

#include "refs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "eval.h"
#include "mm.h"

/*! The alignment of value_t structs in the memory pool. */
#define ALIGNMENT 8

//// MODULE-LOCAL STATE ////

/*!
 * The start of the allocated memory pool.
 * Stored so that it can be free()d when the interpreter exits.
 */
static void *pool;

/*! 
 * Represents the current assignment of the "from" and "to" spaces. If f is
 * true, then the "from" space is the first half of the memory pool and the
 * "to" space is the second half; if f is false, then the "from" space is the
 * second half of the pool and the "to" space is the first half.
 */
bool f;

/*!
 * Used to denote the end of the first half of the pool and the beginning of
 * the second half.
 */
uint8_t *halfway_point;

/*!
 * The size of the memory pool. Set in init_refs().
 */
size_t actual_size;

/*!
 * This is the "reference table", which maps references to value_t pointers.
 * The value at index i is the location of the value_t with reference i.
 * An unused reference is indicated by storing NULL as the value_t*.
 */
static value_t **ref_table;

/*!
 * This is the number of references currently in the table, including unused ones.
 * Valid entries are in the range 0 .. num_refs - 1.
 */
static reference_t num_refs;

/*!
 * This is the maximum size of the ref_table.
 * If the table grows larger, it must be reallocated.
 */
static reference_t max_refs;


//// FUNCTION DEFINITIONS ////


/*!
 * This function initializes the references and the memory pool.
 * It must be called before allocations can be served.
 */
void init_refs(size_t memory_size, void *memory_pool) {
    /* Use the memory pool of the given size.
     * We round the size down to a multiple of ALIGNMENT so that values are aligned.
     */
	actual_size = memory_size / ALIGNMENT * ALIGNMENT;
    mm_init(actual_size, memory_pool);
    pool = memory_pool;
	halfway_point = (uint8_t *)pool + (actual_size / 2);
	f = false;

    /* Start out with no references in our reference-table. */
    ref_table = NULL;
    num_refs = 0;
    max_refs = 0;
}


/*! Allocates an available reference in the ref_table. */
static reference_t assign_reference(value_t *value) {
    /* Scan through the reference table to see if we have any unused slots
     * that can store this value. */
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] == NULL) {
            ref_table[i] = value;
            return i;
        }
    }

    /* If we are out of slots, increase the size of the reference table. */
    if (num_refs == max_refs) {
        /* Double the size of the reference table, unless it was 0 before. */
        max_refs = max_refs == 0 ? INITIAL_SIZE : max_refs * 2;
        ref_table = realloc(ref_table, sizeof(value_t *[max_refs]));
        if (ref_table == NULL) {
            fprintf(stderr, "could not resize reference table");
            exit(1);
        }
    }

    /* No existing references were unused, so use the next available one. */
    reference_t ref = num_refs;
    num_refs++;
    ref_table[ref] = value;
    return ref;
}


/*! Attempts to allocate a value from the memory pool and assign it a reference. */
reference_t make_ref(value_type_t type, size_t size) {
    /* Force alignment of data size to ALIGNMENT. */
    size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    /* Find a (free) location to store the value. */
    value_t *value = mm_malloc(size);

    /* If there was no space, then fail. */
    if (value == NULL) {
        return NULL_REF;
    }

    /* Initialize the value. */
    assert(value->type == VAL_FREE);
    value->type = type;
    value->ref_count = 1; // this is the first reference to the value

    /* Set the data area to a pattern so that it's easier to debug. */
    memset(value + 1, 0xCC, value->value_size - sizeof(value_t));

    /* Assign a reference_t to it. */
    return assign_reference(value);
}


/*! Dereferences a reference_t into a pointer to the underlying value_t. */
value_t *deref(reference_t ref) {
    /* Make sure the reference is actually a valid index. */
    assert(ref >= 0 && ref < num_refs);

    value_t *value = ref_table[ref];

    /* Make sure the reference's value is within the pool!
     * Also ensure that the value is not NULL, indicating an unused reference. */
    assert(is_pool_address(value));

    return value;
}

/*! Returns the reference that maps to the given value. */
reference_t get_ref(value_t *value) {
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] == value) {
            return i;
        }
    }
    assert(!"Value has no reference");
}


/*! Returns the number of values in the memory pool. */
size_t refs_used() {
    size_t values = 0;
    for (reference_t i = 0; i < num_refs; i++) {
        if (ref_table[i] != NULL) {
            values++;
        }
    }
    return values;
}


//// REFERENCE COUNTING ////
void recurse(value_t *v, void (*f)(reference_t ref)) {
	switch (v->type) {
		case VAL_LIST: {
			list_value_t *l = (list_value_t *)v;
			f(l->values);
			break;
		}
		case VAL_DICT: {
			dict_value_t *d = (dict_value_t *)v;
			f(d->keys);
			f(d->values);
			break;
		}
		case VAL_REF_ARRAY: {
			ref_array_value_t *a = (ref_array_value_t *)v;
			for (size_t i = 0; i < a->capacity; i++) {
				f(a->values[i]);
			}
			break;
		}
		default:
			break;
	}
}

/*! Increases the reference count of the value at the given reference. */
void incref(reference_t ref) {
	value_t *v = deref(ref);
	v->ref_count++;
}

/*!
 * Decreases the reference count of the value at the given reference.
 * If the reference count reaches 0, the value is definitely garbage and should be freed.
 */
void decref(reference_t ref) {
	if (ref == NULL_REF || ref == TOMBSTONE_REF) {
		return;
	}
	value_t *v = deref(ref);
	if (v->ref_count > 0) {
		v->ref_count--;
	}
	if (v->ref_count == 0) {
		// Dealing with garbage values
		recurse(v, decref);

		ref_table[ref] = NULL;
		mm_free(v);
	}
}

//// END REFERENCE COUNTING ////


//// GARBAGE COLLECTOR ////

// This function should act on a specific global given a reference_t.
void operate_global(reference_t ref) {
	if (ref == NULL_REF || ref == TOMBSTONE_REF) {
		return;
	}

	value_t *v = ref_table[ref];

	if (is_pool_address(v)) {
		// if v is already in the "to" pool, increment its ref_count
		v->ref_count++;
		return;
	}
	uint8_t *old_pointer = (uint8_t *)v;
	uint8_t *new_pointer = (uint8_t *)mm_malloc(v->value_size);

	// copy over to the "to" space
	memcpy(new_pointer, old_pointer, v->value_size);

	// initialize the newly copied value's ref_count
	value_t *new_v = (value_t *)new_pointer;
	new_v->ref_count = 1;

	// update the ref_table
	ref_table[ref] = new_v; 

	// recurse as necessary on new_v
	recurse(new_v, operate_global);
}

/*!
 * This method only exists because I wanted to write my other method signature
 * without including the unused char * parameter. Just keeping code clean.
 */
void wrapper(const char *name, reference_t ref) {
	(void)name;
	operate_global(ref);
}

/*!
 * It's a pretty safe bet that this method collects garbage.
 * After iterating through each global variable, the ref_table is checked and
 * cleaned.
 */
void collect_garbage(void) {
    if (interactive) {
        fprintf(stderr, "Collecting garbage.\n");
    }
    size_t old_use = mem_used();

	// swap which half is the "from" and which half is the "to" space
	f = !f;

	if (f) {
		mm_init(actual_size / 2, halfway_point);
	}
	else {
		mm_init(actual_size / 2, (uint8_t *)pool);
	}

	// iterate on each global variable
	foreach_global(wrapper);

	// loop through ref table and null out (presumably) cycle refs
	for (size_t i = 0; i < (size_t)num_refs; i++) {
		if (!is_pool_address(ref_table[i])) {
			ref_table[i] = NULL;
		}
	}

    if (interactive) {
        // This will report how many bytes we were able to free in this garbage
        // collection pass.
        fprintf(stderr, "Reclaimed %zu bytes of garbage.\n", old_use - mem_used());
    }
}

//// END GARBAGE COLLECTOR ////


/*!
 * Clean up the allocator state.
 * This requires freeing the memory pool and the reference table,
 * so that the allocator doesn't leak memory.
 */
void close_refs(void) {
    free(pool);
    free(ref_table);
}

