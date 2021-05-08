#pragma once

struct sq_string {
	char *ptr;
	int refcount;
	unsigned length;
};

struct sq_string *sq_string_alloc(unsigned length);
struct sq_string *sq_string_new2(char *ptr, unsigned length);

extern unsigned long strlen(const char *);

static inline struct sq_string *sq_string_new(char *ptr) {
	return sq_string_new2(ptr, strlen(ptr));
}


void sq_string_clone(struct sq_string *string);
void sq_string_free(struct sq_string *string);
void sq_string_combine(const struct sq_string *lhs, const struct sq_string *rhs);
