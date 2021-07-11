#ifndef SQ_BOOK_H

#include "value.h"
#include "shared.h"
#include <assert.h>
#include <stddef.h>
#include <stdalign.h>

/** An array within Squire.
 * 
 * This is refcounted; when `refcount` reaches zero, the book should be passed
 * to `sq_book_deallocate` to free its resources.
 */
struct sq_book {
	/* The pages associated with this book. */
	SQ_VALUE_ALIGN sq_value *pages;

	/* How many pages are in this book. */
	size_t length;

	/* How many pages the book can hold before reallocating. */
	size_t capacity;

	/** How many outstanding referencing there are to this book. */
	unsigned refcount;
};

/** Creates a new book from the given `length`, `capacity`, and `pages`.
 *
 * Note that `length` should be less than or equal to `capacity`.
 */
struct sq_book *sq_book_new(size_t length, size_t capacity, sq_value *pages);

// Creates a new book where the initial capacity is `length`.
static inline struct sq_book *sq_book_new2(size_t length, sq_value *pages) {
    return sq_book_new(length, length, pages);
}

// Creates a new book with the given capacity.
static inline struct sq_book *sq_book_allocate(size_t capacity) {
    return sq_book_new(0, capacity, xmalloc(sizeof(sq_value [capacity])));
}

/** Deallocates the memory associated with `book`.
 * 
 * This should only ever be called when `book` has a zero refcount.
 */
void sq_book_deallocate(struct sq_book *book);

// Simply increases the refcount of the book.
static inline struct sq_book *sq_book_clone(struct sq_book *book) {
	assert(book->refcount);

	++book->refcount;

	return book;
}

// Frees memory associated with the book.
static inline void sq_book_free(struct sq_book *book) {
	assert(book->refcount);

	if (!--book->refcount)
		sq_book_deallocate(book);
}

/** Inserts `value` at the given index.
 * 
 * If `index` is out of bounds, the book is expanded and the missing pages
 * are replaced with `SQ_NI`.
 */
void sq_book_insert(struct sq_book *book, size_t index, sq_value value);

/** Removes an element at `index` from the book, returning it.
 *
 * If `index` is out of bounds, `SQ_NI` is returned.
 */
sq_value sq_book_delete(struct sq_book *book, size_t index);

/**
 * Fetches the `index`th element from `book`.
 * 
 * If `index` is out of bounds, `SQ_NI` is returend.
 */
sq_value sq_book_index(const struct sq_book *book, size_t index);

/** Replaces the `index`th element of `book` with `value.
 * 
 * If `index` is out of bounds, this fills the missing values with `SQ_NI`s.
 **/
void sq_book_index_assign(struct sq_book *book, size_t index, sq_value value);

/** Dumps a debugging representation of `book` to `out`.
 * 
 * This does not write a trailing newline.
 */
void sq_book_dump(FILE *out, const struct sq_book *book);

#ifndef __HAS_SSIZET_DEFINED__
typedef long ssize_t;
#endif /* !__HAS_SSIZET_DEFINED__ */

/** Fix `index`, making it relative to `book`'s length.
 * 
 * If it's positive, it's simply returned. If it's negative, it starts from the
 * end. If it's still negative after that, `sq_throw` is called.
 */
size_t sq_book_fix_index(const struct sq_book *book, ssize_t index);

// Same as `sq_book_insert`, but it accepts negative indices.
static inline void sq_book_insert2(struct sq_book *book, ssize_t index, sq_value value) {
	sq_book_insert(book, sq_book_fix_index(book, index), value);
}

// Same as `sq_book_delete`, but it accepts negative indices.
static inline sq_value sq_book_delete2(struct sq_book *book, ssize_t index) {
	return sq_book_delete(book, sq_book_fix_index(book, index));
}

// Same as `sq_book_index`, but it accepts negative indices.
static inline sq_value sq_book_index2(const struct sq_book *book, ssize_t index) {
	return sq_book_index(book, sq_book_fix_index(book, index));
}

// Same as `sq_book_index`, but it accepts negative indices.
static inline void sq_book_index_assign2(struct sq_book *book, ssize_t index, sq_value value) {
	sq_book_index_assign(book, sq_book_fix_index(book, index), value);
}


/** Converts an book to its text representation */
struct sq_text *sq_book_to_text(const struct sq_book *book);

struct sq_codex *sq_book_to_codex(const struct sq_book *book);
struct sq_book *sq_book_repeat(const struct sq_book *book, unsigned amnt);
struct sq_text *sq_book_join(const struct sq_book *book, const struct sq_text *sep);
struct sq_book *sq_book_product(const struct sq_book *book, const struct sq_book *rhs);
struct sq_book *sq_book_map(const struct sq_book *book, const struct sq_journey *func);
struct sq_book *sq_book_select(const struct sq_book *book, const struct sq_journey *func);
sq_value sq_book_reduce(const struct sq_book *book, const struct sq_journey *func);

#endif /* !SQ_BOOK_H */
