#include "array.h"
#include "shared.h"
#include "exception.h"
#include <stdio.h>
#include <string.h>

struct sq_array *sq_array_new(size_t length, size_t capacity, sq_value *elements) {
	assert(length <= capacity);
	assert(!(length != 0 && elements == NULL));

	struct sq_array *array = xmalloc(sizeof(struct sq_array));

	array->capacity = capacity;
	array->length = length;
	array->elements = elements;
	array->refcount = 1;

	return array;
}

void sq_array_dump(FILE *out, const struct sq_array *array) {
	fprintf(out, "Array(");

	for (size_t i = 0; i < array->length; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(array->elements[i]);
	}

	putc(')', out);
}

void sq_array_deallocate(struct sq_array *array) {
	assert(!array->refcount);

	for (size_t i = 0; i < array->length; ++i)
		sq_value_free(array->elements[i]);

	free(array->elements);
	free(array);
}

static size_t fix_index(const struct sq_array *array, ssize_t index) {
	if (index < 0)
		index += array->length;

	if (index < 0)
		sq_throw("index '-%zu' out of bounds!", (size_t) index);

	return index;
}


static void expand_array(struct sq_array *array, size_t length) {
	if (length < array->length)
		return;

	if (array->capacity <= length) {
		// todo: increase capacity by two.
		array->capacity = length * 2 + 1;
		array->elements = xrealloc(array->elements, sizeof(sq_value[array->capacity]));
	}

	while (array->length < length)
		array->elements[array->length++] = SQ_NULL;
}

void sq_array_insert(struct sq_array *array, ssize_t sindex, sq_value value) {
	size_t index = fix_index(array, sindex);

	expand_array(array, (array->length < index ? index : array->length) + 1);

	memmove(
		&array->elements[index + 1],
		&array->elements[index],
		sizeof(sq_value[array->length - index])
	);

	sq_value_free(array->elements[index]);
	array->elements[index] = value;
}

sq_value sq_array_delete(struct sq_array *array, ssize_t sindex) {
	size_t index = fix_index(array, sindex);

	if (array->length <= index)
		return SQ_NULL;

	sq_value result = array->elements[index];

	memmove(
		&array->elements[index],
		&array->elements[index + 1],
		sizeof(sq_value[array->length - index])
	);

	--array->length;

	return result;
}

sq_value sq_array_index(const struct sq_array *array, ssize_t sindex) {
	size_t index = fix_index(array, sindex);

	if (array->length <= index)
		return SQ_NULL;

	return sq_value_clone(array->elements[index]);
}

void sq_array_index_assign(struct sq_array *array, ssize_t sindex, sq_value value) {
	size_t index = fix_index(array, sindex);

	expand_array(array, index + 1);

	sq_value_free(array->elements[index]);
	array->elements[index] = value;
}

struct sq_string *sq_array_to_string(const struct sq_array *array) {
	(void)array;
	die("todo!");
}
