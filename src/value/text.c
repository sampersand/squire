#include <squire/text.h>
#include <squire/shared.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct sq_text sq_text_empty = SQ_TEXT_STATIC("");

static struct sq_text *allocate_text(unsigned length) {
	sq_assert_nz(length);
	struct sq_text *text = sq_mallocv(struct sq_text);

	text->length = length;

	return text;
}

struct sq_text *sq_text_new2(char *ptr, unsigned length) {
	sq_assert_nn(ptr);
	sq_assert_eq(length, strlen(ptr));

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

	text->ptr = sq_malloc_heap(length + 1);

	return text;
}

void sq_text_deallocate(struct sq_text *text) {
	free(text->ptr);
}


char *sq_text_to_c_str(const struct sq_text *text) {
	char *ret = sq_malloc_vec(char, text->length + 1);
	memcpy(ret, text->ptr, text->length);
	ret[text->length] = '\0';
	return ret;
}

static char num2hex(char c) {
	return c + (c <= 9 ? '0' : 'A' - 10);
}

void sq_text_dump(FILE *out, const struct sq_text *text) {
	fputc('"', out);

	for (unsigned i = 0; i < text->length; ++i) {
		unsigned char c = text->ptr[i];

		switch (c) {
		case '\n': fputs("\\n", out); break;
		case '\t': fputs("\\t", out); break;
		case '\f': fputs("\\f", out); break;
		case '\v': fputs("\\v", out); break;
		case '\r': fputs("\\r", out); break;

		case '\\':
		case '\"':
			fputc('\\', out);

			if (0) {
		default:
				if (c < ' ' || '~' < c) {
					fputs("\\x", out);
					fputc(num2hex(c >> 4), out);
					fputc(num2hex(c & 0xf), out);
					break;
				}
			}

			fputc(c, out);
		}
	}

	fputc('"', out);
}
