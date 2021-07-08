#include "book.h"
#include "shared.h"
#include "exception.h"
#include "string.h"
#include "journey.h"
#include <stdio.h>
#include <string.h>

struct sq_book *sq_book_new(size_t length, size_t capacity, sq_value *pages) {
	assert(length <= capacity);
	assert(!(length != 0 && pages == NULL));

	struct sq_book *book = xmalloc(sizeof(struct sq_book));

	book->capacity = capacity;
	book->length = length;
	book->pages = pages;
	book->refcount = 1;

	return book;
}

void sq_book_dump(FILE *out, const struct sq_book *book) {
	fprintf(out, "Book(");

	for (size_t i = 0; i < book->length; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(book->pages[i]);
	}

	putc(')', out);
}

void sq_book_deallocate(struct sq_book *book) {
	assert(!book->refcount);

	for (size_t i = 0; i < book->length; ++i)
		sq_value_free(book->pages[i]);

	free(book->pages);
	free(book);
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
		book->pages = xrealloc(book->pages, sizeof(sq_value[book->capacity]));
	}

	while (book->length < length)
		book->pages[book->length++] = SQ_NULL;
}

void sq_book_insert(struct sq_book *book, size_t index, sq_value value) {
	expand_book(book, (book->length < index ? index : book->length) + 1);

	memmove(
		&book->pages[index + 1],
		&book->pages[index],
		sizeof(sq_value[book->length - index])
	);

	sq_value_free(book->pages[index]);
	book->pages[index] = value;
}

sq_value sq_book_delete(struct sq_book *book, size_t index) {

	if (book->length <= index)
		return SQ_NULL;

	sq_value result = book->pages[index];

	memmove(
		&book->pages[index],
		&book->pages[index + 1],
		sizeof(sq_value[book->length - index])
	);

	--book->length;

	return result;
}

sq_value sq_book_index(const struct sq_book *book, size_t index) {
	if (book->length <= index)
		return SQ_NULL;

	return sq_value_clone(book->pages[index]);
}

void sq_book_index_assign(struct sq_book *book, size_t index, sq_value value) {
	expand_book(book, index + 1);
	sq_value_free(book->pages[index]);
	book->pages[index] = value;
}

struct sq_string *sq_book_to_string(const struct sq_book *book) {
	unsigned len = 0, cap = 64;
	char *string = xmalloc(cap);
	string[len++] = '[';

	for (unsigned i = 0; i < book->length; ++i) {
		if (i) {
			if (cap <= len + 2)
				string = xrealloc(string, cap *= 2);
			string[len++] = ',';
			string[len++] = ' ';
		}

		struct sq_string *inner = sq_value_to_string(book->pages[i]);
	
		if (cap <= inner->length + len)
			string = xrealloc(string, cap = inner->length + len * 2);
	
		memcpy(string + len, inner->ptr, inner->length);
		len += inner->length;
		sq_string_free(inner);
	}

	string = xrealloc(string, len + 2);
	string[len++] = ']';
	string[len] = '\0';

	return sq_string_new2(string, len);
}

struct sq_codex *sq_book_to_codex(const struct sq_book *book) {
	(void) book;
	todo(__FUNCTION__);
}

struct sq_book *sq_book_repeat(const struct sq_book *book, unsigned amnt) {
	struct sq_book *new = sq_book_allocate(book->length * amnt);

	for (unsigned i = 0; i < amnt; ++i)
		for (unsigned j = 0; j < book->length; ++j)
			new->pages[new->length++] = sq_value_clone(book->pages[j]);

	return new;
}

struct sq_string *sq_book_join(const struct sq_book *book, const struct sq_string *sep) {
	unsigned len = 0, cap = 64, seplen = strlen(sep->ptr);
	char *string = xmalloc(cap);

	for (unsigned i = 0; i < book->length; ++i) {
		if (i) {
			if (cap <= len + seplen)
				string = xrealloc(string, cap = cap * 2 + seplen);

			memcpy(string + len, sep->ptr, seplen);
			len += seplen;
		}

		struct sq_string *str = sq_value_to_string(book->pages[i]);
		if (cap <= str->length + len)
			string = xrealloc(string, cap = cap * 2 + str->length);

		memcpy(string + len, str->ptr, str->length);
		len += str->length;
		sq_string_free(str);
	}

	string = xrealloc(string, len + 1);
	string[len] = '\0';

	return sq_string_new2(string, len);
}

struct sq_book *sq_book_product(const struct sq_book *book, const struct sq_book *rhs) {
	struct sq_book *result = sq_book_allocate(book->length + rhs->length);

	for (unsigned i = 0; i < book->length; ++i)
		for (unsigned j = 0; j < rhs->length; ++j) {
			struct sq_book *new = sq_book_allocate(2);
			new->pages[new->length++] = sq_value_clone(book->pages[i]);
			new->pages[new->length++] = sq_value_clone(rhs->pages[i]);
			result->pages[result->length++] = sq_value_new(new);
		}

	return result;
}

struct sq_book *sq_book_map(const struct sq_book *book, const struct sq_journey *func) {
	struct sq_book *result = sq_book_allocate(book->length);

	for (unsigned i = 0; i < book->length; ++i)
		result->pages[result->length++] = sq_journey_run(func, 1, &book->pages[i]);

	return result;
}

struct sq_book *sq_book_select(const struct sq_book *book, const struct sq_journey *func) {
	struct sq_book *result = sq_book_allocate(book->length);

	for (unsigned i = 0; i < book->length; ++i)
		if (sq_value_to_boolean(sq_journey_run(func, 1, &book->pages[i])))
			result->pages[result->length++] = sq_value_clone(book->pages[i]);

	return result;
}

sq_value sq_book_reduce(const struct sq_book *book, const struct sq_journey *func) {
	if (!book->length) return SQ_NULL;
	sq_value acc[2] = { sq_value_clone(book->pages[0]) };

	for (unsigned i = 0; i < book->length; ++i) {
		acc[1] = book->pages[i];
		acc[0] = sq_journey_run(func, 2, acc);
	}

	sq_value_free(acc[1]);

	return acc[0];
}
