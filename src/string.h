#ifndef SQ_STRING_H
#define SQ_STRING_H

#include <string.h>
#include "value.h"

struct sq_string {
	SQ_VALUE_ALIGN char *ptr;
	int refcount;
	unsigned length;
};

extern struct sq_string sq_string_empty;

struct sq_string *sq_string_alloc(unsigned length);
struct sq_string *sq_string_new2(char *ptr, unsigned length);

static inline struct sq_string *sq_string_new(char *ptr) {
	return sq_string_new2(ptr, strlen(ptr));
}


#define SQ_STRING_STATIC(literal) \
	{ \
		.ptr = (literal), \
		.refcount = -1, \
		.length = sizeof(literal) - 1, \
	}

static inline struct sq_string *sq_string_clone(struct sq_string *string) {
	assert(string->refcount);

	if (0 < string->refcount)
		++string->refcount;

	return string;
}

void sq_string_dealloc(struct sq_string *stirng);

static inline void sq_string_free(struct sq_string *string) {
	assert(string->refcount);

	if (0 < string->refcount && !--string->refcount)
		sq_string_dealloc(string);
}

void sq_string_combine(const struct sq_string *lhs, const struct sq_string *rhs);
void sq_string_sprintf_repr(const struct sq_string *string, char **out, unsigned *len, unsigned *cap, unsigned *pos);

#endif /* !SQ_STRING_H */
