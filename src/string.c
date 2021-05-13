#include "string.h"
#include "assert.h"
#include "shared.h"
#include <stdlib.h>

struct sq_string *sq_string_new2(char *ptr, unsigned length) {
	struct sq_string *string = xmalloc(sizeof(struct sq_string));

	string->ptr = ptr;
	string->refcount = 1;
	string->length = length;

	return string;
}

struct sq_string *sq_string_alloc(unsigned length) {
	return sq_string_new2(xmalloc(length), length);
}


void sq_string_clone(struct sq_string *string) {
	assert(string->refcount);

	if (0 < string->refcount)
		++string->refcount;
}

void sq_string_free(struct sq_string *string) {
	return;
	assert(string->refcount);

	if (string->refcount < 0 || !--string->refcount)
		return;

	free(string->ptr);
	free(string);
}
