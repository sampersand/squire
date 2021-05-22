#include "array.h"
#include "shared.h"
#include <stdio.h>

struct sq_array *sq_array_new(unsigned len, sq_value *eles) {
	struct sq_array *array = xmalloc(sizeof(struct sq_array));

	array->len = array->cap = len;
	array->eles = eles;
	array->refcount = 1;

	return array;
}

void sq_array_dump(const struct sq_array *array) {
	putchar('[');

	for (unsigned i = 0; i < array->len; ++i) {
		if (i != 0) printf(", ");
		sq_value_dump(array->eles[i]);
	}

	putchar(']');
}

struct sq_array *sq_array_clone(struct sq_array *array) {
	assert(array->refcount);

	array->refcount++;

	return array;
}

void sq_array_free(struct sq_array *array) {
	assert(array->refcount);

	if (--array->refcount) return;

	for (unsigned i = 0; i < array->len; ++i)
		sq_value_free(array->eles[i]);

	free(array->eles);
	free(array);
}

static unsigned fix_index(const struct sq_array *array, int index) {
	if (index < 0)
		index += array->len;

	if (index < 0)
		die("index '%d' out of bounds!", index - array->len);

	return index;
}

void sq_array_resize(struct sq_array *array) {
	if (array->cap == 0)
		array->eles = xmalloc(sizeof(sq_value[array->cap=4]));
	else
		array->eles = xrealloc(array->eles, sizeof(sq_value[array->cap*=2]));
}


void sq_array_insert(struct sq_array *array, int sindex, sq_value value) {
	unsigned index = fix_index(array, sindex);

	if (array->len < index) {
		sq_array_index_assign(array, index, value);
		return;
	}

	if (array->len + 1 >= array->cap)
		sq_array_resize(array);

	for (unsigned i = ++array->len; i != index; --i)
		array->eles[i] = array->eles[i - 1];

	sq_value_free(array->eles[index]);
	array->eles[index] = value;
}

sq_value sq_array_delete(struct sq_array *array, int index) {
	return sq_value_clone(array->eles[fix_index(array, index)]);
}

sq_value sq_array_index(struct sq_array *array, int sindex) {
	unsigned index = fix_index(array, sindex);

	if (array->len <= index)
		return SQ_NULL;

	return sq_value_clone(array->eles[index]);
}

void sq_array_index_assign(struct sq_array *array, int sindex, sq_value value) {
	unsigned index = fix_index(array, sindex);

	if (array->cap <= index)
		array->eles = xrealloc(array->eles, sizeof(sq_value[array->cap = index + 1]));

	if (index < array->len) {
		sq_value_free(array->eles[index]);
	} else {
		while (array->len <= index)
			array->eles[array->len++] = SQ_NULL;
	}

	array->eles[index] = value;
}
