#include "string.h"
#include "assert.h"
#include "shared.h"
#include <stdlib.h>
#include <string.h>

struct sq_string sq_string_empty = SQ_STRING_STATIC("");

static struct sq_string *allocate_string(unsigned length) {
	assert(length != 0);
	struct sq_string *string = xmalloc(sizeof(struct sq_string));

	string->refcount = 1;
	string->length = length;

	return string;
}

struct sq_string *sq_string_new2(char *ptr, unsigned length) {
	assert(ptr != NULL);
	assert(strlen(ptr) == length);

	if (length == 0) {
		free(ptr);
		return &sq_string_empty;
	}

	struct sq_string *string = allocate_string(length);
	string->ptr = ptr;

	return string;
}

struct sq_string *sq_string_alloc(unsigned length) {
	if (length == 0)
		return &sq_string_empty;

	struct sq_string *string = allocate_string(length);

	string->ptr = xmalloc(length + 1);

	return string;
}

void sq_string_dealloc(struct sq_string *string) {
	assert(!string->refcount);

	free(string->ptr);
	free(string);
}
