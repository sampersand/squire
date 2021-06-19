#ifndef SQ_ARRAY_H

#include "value.h"
#include "shared.h"
#include <assert.h>
#include <stddef.h>
#include <stdalign.h>

/** An array within Squire.
 * 
 * This is refcounted; when `refcount` reaches zero, the array should be passed
 * to `sq_array_deallocate` to free its resources.
 */
struct sq_array {
	/* The elements associated with this array. */
	SQ_VALUE_ALIGN sq_value *elements;

	/* How many elements are in this array. */
	size_t length;

	/* How many elements the array can hold before reallocating. */
	size_t capacity;

	/** How many outstanding referencing there are to this array. */
	unsigned refcount;
};

/** Creates a new array from the given `length`, `capacity`, and `elements`.
 *
 * Note that `length` should be less than or equal to `capacity`.
 */
struct sq_array *sq_array_new(size_t length, size_t capacity, sq_value *elements);

// Creates a new array where the initial capacity is `length`.
static inline struct sq_array *sq_array_new2(size_t length, sq_value *elements) {
    return sq_array_new(length, length, elements);
}

// Creates a new array with the given capacity.
static inline struct sq_array *sq_array_allocate(size_t capacity) {
    return sq_array_new(0, capacity, xmalloc(sizeof(sq_value [capacity])));
}

/** Deallocates the memory associated with `array`.
 * 
 * This should only ever be called when `array` has a zero refcount.
 */
void sq_array_deallocate(struct sq_array *array);

// Simply increases the refcount of the array.
static inline struct sq_array *sq_array_clone(struct sq_array *array) {
	assert(array->refcount);

	++array->refcount;

	return array;
}

// Frees memory associated with the array.
static inline void sq_array_free(struct sq_array *array) {
	assert(array->refcount);

	if (!--array->refcount)
		sq_array_deallocate(array);
}

/** Converts an array to its string representation */
struct sq_string *sq_array_to_string(const struct sq_array *array);

/** Inserts `value` at the given index.
 * 
 * If `index` is out of bounds, the array is expanded and the missing elements
 * are replaced with `SQ_NULL`.
 */
void sq_array_insert(struct sq_array *array, size_t index, sq_value value);

/** Removes an element at `index` from the array, returning it.
 *
 * If `index` is out of bounds, `SQ_NULL` is returned.
 */
sq_value sq_array_delete(struct sq_array *array, size_t index);

/**
 * Fetches the `index`th element from `array`.
 * 
 * If `index` is out of bounds, `SQ_NULL` is returend.
 */
sq_value sq_array_index(const struct sq_array *array, size_t index);

/** Replaces the `index`th element of `array` with `value.
 * 
 * If `index` is out of bounds, this fills the missing values with `SQ_NULL`s.
 **/
void sq_array_index_assign(struct sq_array *array, size_t index, sq_value value);

/** Dumps a debugging representation of `array` to `out`.
 * 
 * This does not write a trailing newline.
 */
void sq_array_dump(FILE *out, const struct sq_array *array);

#ifndef __HAS_SSIZET_DEFINED__
typedef long ssize_t;
#endif /* !__HAS_SSIZET_DEFINED__ */

/** Fix `index`, making it relative to `array`'s length.
 * 
 * If it's positive, it's simply returned. If it's negative, it starts from the
 * end. If it's still negative after that, `sq_throw` is called.
 */
size_t sq_array_fix_index(const struct sq_array *array, ssize_t index);

// Same as `sq_array_insert`, but it accepts negative indices.
static inline void sq_array_insert2(struct sq_array *array, ssize_t index, sq_value value) {
	sq_array_insert(array, sq_array_fix_index(array, index), value);
}

// Same as `sq_array_delete`, but it accepts negative indices.
static inline void sq_array_delete2(struct sq_array *array, ssize_t index) {
	sq_array_delete(array, sq_array_fix_index(array, index));
}

// Same as `sq_array_index`, but it accepts negative indices.
static inline sq_value sq_array_index2(const struct sq_array *array, ssize_t index) {
	return sq_array_index(array, sq_array_fix_index(array, index));
}

// Same as `sq_array_index`, but it accepts negative indices.
static inline void sq_array_index_assign2(struct sq_array *array, ssize_t index, sq_value value) {
	sq_array_index_assign(array, sq_array_fix_index(array, index), value);
}

#endif /* !SQ_ARRAY_H */
