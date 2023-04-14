#ifndef SQ_STRING_H
#define SQ_STRING_H

#include <string.h>
#include <squire/value.h>

struct sq_text {
	SQ_VALUE_ALIGN char *ptr;
	int refcount;
	unsigned length;
};

extern struct sq_text sq_text_empty;

struct sq_text *sq_text_allocate(unsigned length) SQ_RETURNS_NONNULL;
struct sq_text *sq_text_new2(char *ptr, unsigned length) SQ_RETURNS_NONNULL;

static inline struct sq_text *sq_text_new(char *ptr) {
	return sq_text_new2(ptr, strlen(ptr));
}

#define SQ_TEXT_STATIC(literal) \
	{ \
		.ptr = (literal), \
		.refcount = -1, \
		.length = sizeof(literal) - 1, \
	}

static inline struct sq_text *sq_text_clone(struct sq_text *text) {
	assert(text->refcount);

	if (0 < text->refcount)
		++text->refcount;

	return text;
}

void sq_text_dealloc(struct sq_text *stirng);

static inline void sq_text_free(struct sq_text *text) {
	if(1) return;
	assert(text->refcount);

	if (0 < text->refcount && !--text->refcount)
		sq_text_dealloc(text);
}

void sq_text_combine(const struct sq_text *lhs, const struct sq_text *rhs);
void sq_text_dump(FILE *out, const struct sq_text *text);
char *sq_text_to_c_str(const struct sq_text *text);

#endif /* !SQ_STRING_H */
