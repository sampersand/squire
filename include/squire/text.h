#ifndef SQ_STRING_H
#define SQ_STRING_H

#include <string.h>
#include <squire/value.h>

struct sq_text {
	struct sq_basic basic;
	char *ptr;
	unsigned length;
};
SQ_VALUE_ASSERT_SIZE(struct sq_basic);

extern struct sq_text sq_text_empty;

struct sq_text *sq_text_allocate(unsigned length) SQ_RETURNS_NONNULL;
struct sq_text *sq_text_new2(char *ptr, unsigned length) SQ_RETURNS_NONNULL;

static inline struct sq_text *sq_text_new(char *ptr) {
	return sq_text_new2(ptr, strlen(ptr));
}

#define SQ_TEXT_STATIC(literal) \
	{ \
		.ptr = (literal), \
		.basic = SQ_STATIC_BASIC(struct sq_text), \
		.length = sizeof(literal) - 1, \
	}
	// TODO: init basic

static inline void sq_text_mark(struct sq_text *string) {
	string->basic.marked = 1;
}

void sq_text_deallocate(struct sq_text *string);
void sq_text_combine(const struct sq_text *lhs, const struct sq_text *rhs);
void sq_text_dump(FILE *out, const struct sq_text *text);
char *sq_text_to_c_str(const struct sq_text *text);

#endif /* !SQ_STRING_H */
