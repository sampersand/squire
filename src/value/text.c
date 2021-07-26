#include <squire/text.h>
#include <squire/shared.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct sq_text sq_text_empty = SQ_TEXT_STATIC("");

static struct sq_text *allocate_text(unsigned length) {
	assert(length != 0);
	struct sq_text *text = xmalloc(sizeof(struct sq_text));

	text->refcount = 1;
	text->length = length;

	return text;
}

struct sq_text *sq_text_new2(char *ptr, unsigned length) {
	assert(ptr != NULL);
	assert(strlen(ptr) == length);

	if (length == 0) {
		free(ptr);
		return &sq_text_empty;
	}

	struct sq_text *text = allocate_text(length);
	text->ptr = ptr;

	return text;
}

struct sq_text *sq_text_allocate(unsigned length) {
	if (length == 0)
		return &sq_text_empty;

	struct sq_text *text = allocate_text(length);

	text->ptr = xmalloc(length + 1);

	return text;
}

void sq_text_dealloc(struct sq_text *text) {
	assert(!text->refcount);

	free(text->ptr);
	free(text);
}
