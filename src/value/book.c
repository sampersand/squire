#include <stdio.h>
#include <string.h>
#include <squire/book.h>
#include <squire/shared.h>
#include <squire/exception.h>
#include <squire/text.h>
#include <squire/journey.h>

struct sq_book *sq_book_new(size_t length, size_t capacity, sq_value *pages) {
	assert(length <= capacity);
	assert(!(length != 0 && pages == NULL));

	struct sq_book *book = sq_malloc_single(struct sq_book);

	book->basic = SQ_BASIC_DEFAULT;
	book->capacity = capacity;
	book->length = length;
	book->pages = pages;

	return book;
}

void sq_book_dump(FILE *out, const struct sq_book *book) {
	fputc('[', out);

	for (size_t i = 0; i < book->length; ++i) {
		if (i)
			fputs(", ", out);

		sq_value_dump(out, book->pages[i]);
	}

	fputc(']', out);
}

void sq_book_mark(struct sq_book *book) {
	SQ_GUARD_MARK(book);

	for (size_t i = 0; i < book->length; ++i)
		sq_value_mark(book->pages[i]);
}

void sq_book_deallocate(struct sq_book *book) {
	free(book->pages);
	// free(book);
}

size_t sq_book_fix_index(const struct sq_book *book, ssize_t index) {
	if (index < 0)
		index += book->length + 1;

	if (!index--)
		sq_throw("cannot index by N.");

	if (index < 0)
		sq_throw("index '-%zu' out of bounds!", (size_t) index);

	return index;
}


static void expand_book(struct sq_book *book, size_t length) {
	if (length < book->length)
		return;

	if (book->capacity <= length) {
		// todo: increase capacity by two.
		book->capacity = length * 2 + 1;
		book->pages = sq_realloc_vec(sq_value, book->pages, book->capacity);
	}

	while (book->length < length)
		book->pages[book->length++] = SQ_NI;
}

void sq_book_insert(struct sq_book *book, size_t index, sq_value value) {
	expand_book(book, (book->length < index ? index : book->length) + 1);

	memmove(
		&book->pages[index + 1],
		&book->pages[index],
		sq_sizeof_array(sq_value, book->length - index)
	);

	book->pages[index] = value;
}

sq_value sq_book_delete(struct sq_book *book, size_t index) {

	if (book->length <= index)
		return SQ_NI;

	sq_value result = book->pages[index];

	memmove(
		&book->pages[index],
		&book->pages[index + 1],
		sq_sizeof_array(sq_value, book->length - index)
	);

	--book->length;

	return result;
}

sq_value sq_book_index(const struct sq_book *book, size_t index) {
	if (book->length <= index)
		return SQ_NI;

	return book->pages[index];
}

void sq_book_index_assign(struct sq_book *book, size_t index, sq_value value) {
	expand_book(book, index + 1);
	book->pages[index] = value;
}

struct sq_text *sq_book_to_text(const struct sq_book *book) {
	unsigned len = 0, cap = 64;
	char *str = sq_malloc(cap);
	str[len++] = '[';

	for (unsigned i = 0; i < book->length; ++i) {
		if (i) {
			if (cap <= len + 2)
				str = sq_realloc(str, cap *= 2);
			str[len++] = ',';
			str[len++] = ' ';
		}

		struct sq_text *inner = sq_value_to_text(book->pages[i]);
	
		if (cap <= inner->length + len)
			str = sq_realloc(str, cap = inner->length + len * 2);
	
		memcpy(str + len, inner->ptr, inner->length);
		len += inner->length;
	}

	str = sq_realloc(str, len + 2);
	str[len++] = ']';
	str[len] = '\0';

	return sq_text_new2(str, len);
}

struct sq_codex *sq_book_to_codex(const struct sq_book *book) {
	(void) book;
	sq_todo(__FUNCTION__);
}

struct sq_book *sq_book_repeat(const struct sq_book *book, unsigned amnt) {
	struct sq_book *new = sq_book_allocate(book->length * amnt);

	for (unsigned i = 0; i < amnt; ++i)
		for (unsigned j = 0; j < book->length; ++j)
			new->pages[new->length++] = book->pages[j];

	return new;
}

struct sq_text *sq_book_join(const struct sq_book *book, const struct sq_text *sep) {
	unsigned len = 0, cap = 64, seplen = strlen(sep->ptr);
	char *str = sq_malloc(cap);

	for (unsigned i = 0; i < book->length; ++i) {
		if (i) {
			if (cap <= len + seplen)
				str = sq_realloc(str, cap = cap * 2 + seplen);

			memcpy(str + len, sep->ptr, seplen);
			len += seplen;
		}

		struct sq_text *text = sq_value_to_text(book->pages[i]);
		if (cap <= text->length + len)
			str = sq_realloc(str, cap = cap * 2 + text->length);

		memcpy(str + len, text->ptr, text->length);
		len += text->length;
	}

	str = sq_realloc(str, len + 1);
	str[len] = '\0';

	return sq_text_new2(str, len);
}

struct sq_book *sq_book_product(const struct sq_book *book, const struct sq_book *rhs) {
	struct sq_book *result = sq_book_allocate(book->length + rhs->length);

	for (unsigned i = 0; i < book->length; ++i)
		for (unsigned j = 0; j < rhs->length; ++j) {
			struct sq_book *new = sq_book_allocate(2);
			new->pages[new->length++] = book->pages[i];
			new->pages[new->length++] = rhs->pages[i];
			result->pages[result->length++] = sq_value_new_book(new);
		}

	return result;
}

struct sq_book *sq_book_map(const struct sq_book *book, const struct sq_journey *func) {
	struct sq_book *result = sq_book_allocate(book->length);

	for (unsigned i = 0; i < book->length; ++i)
		result->pages[result->length++] = sq_journey_run_deprecated(func, 1, &book->pages[i]);

	return result;
}

struct sq_book *sq_book_select(const struct sq_book *book, const struct sq_journey *func) {
	struct sq_book *result = sq_book_allocate(book->length);

	for (unsigned i = 0; i < book->length; ++i)
		if (sq_value_to_veracity(sq_journey_run_deprecated(func, 1, &book->pages[i])))
			result->pages[result->length++] = book->pages[i];

	return result;
}

sq_value sq_book_reduce(const struct sq_book *book, const struct sq_journey *func) {
	if (!book->length) return SQ_NI;
	sq_value acc[2] = { book->pages[0] };

	for (unsigned i = 1; i < book->length; ++i) {
		acc[1] = book->pages[i];
		acc[0] = sq_journey_run_deprecated(func, 2, acc);
	}

	return acc[0];
}
