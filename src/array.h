#ifndef SQ_ARRAY_H

#include "value.h"
#include "shared.h"
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>

#ifndef __HAS_SSIZET_DEFINED__
typedef ptrdiff_t ssize_t;
#endif /* !__HAS_SSIZET_DEFINED__ */

struct sq_array {
	size_t length, capacity, refcount;
	sq_value *elements;
};

/** Creates a new array from the given `length`, `capacity`, and `elements`.
 *
 * Note that `length` should be less than or equal to `capacity`.
 */
struct sq_array *sq_array_new(size_t length, size_t capacity, sq_value *elements);

// Creates a new array with the given length, where the initial capacity is the length.
static inline struct sq_array *sq_array_new2(size_t length, sq_value *elements) {
	return sq_array_new(length, length, elements);
}

// Creates a new array with the given capacity.
static inline struct sq_array *sq_array_allocate(size_t capacity) {
	return sq_array_new(0, capacity, xmalloc(sizeof(sq_value [capacity])));
}

// Simply increases the refcount of the array.
static inline struct sq_array *sq_array_clone(struct sq_array *array) {
	assert(array->refcount);

	++array->refcount;

	return array;
}

/** Deallocates the memory associated with `array`.
 * 
 * Note that its refcount should be
 */
void sq_array_deallocate(struct sq_array *array);

// Frees memory associated with the array.
static inline void sq_array_free(struct sq_array *array) {
	assert(array->refcount);

	if (!--array->refcount)
		sq_array_deallocate(array);
}

void sq_array_dump(FILE *, const struct sq_array *array);

struct sq_string *sq_array_to_string(const struct sq_array *array);
void sq_array_insert(struct sq_array *array, ssize_t index, sq_value value);
sq_value sq_array_delete(struct sq_array *array, ssize_t index);
sq_value sq_array_index(const struct sq_array *array, ssize_t index);
void sq_array_index_assign(struct sq_array *array, ssize_t index, sq_value value);

#endif /* !SQ_ARRAY_H */
