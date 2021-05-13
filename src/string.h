#ifndef SQ_STRING_H
#define SQ_STRING_H

#include <stdbool.h>

struct sq_string {
	char *ptr;
	int refcount;
	unsigned length;
	bool borrowed;
};

extern struct sq_string sq_string_empty;

struct sq_string *sq_string_alloc(unsigned length);
struct sq_string *sq_string_new(char *ptr);
struct sq_string *sq_string_new2(char *ptr, unsigned length);
struct sq_string *sq_string_borrowed(char *ptr);
#define SQ_STRING_STATIC(literal) \
	{ \
		.ptr = (literal), \
		.refcount = -1, \
		.length = sizeof(literal) - 1, \
	}

struct sq_string *sq_string_clone(struct sq_string *string);
void sq_string_free(struct sq_string *string);
void sq_string_combine(const struct sq_string *lhs, const struct sq_string *rhs);

#endif /* !SQ_STRING_H */
