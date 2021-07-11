#ifndef SQ_VALUE_H
#define SQ_VALUE_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "numeral.h"

struct sq_text;
struct sq_form;
struct sq_imitation;
struct sq_journey;
struct sq_book;
struct sq_codex;

typedef uint64_t sq_value;
typedef bool sq_veracity;

typedef enum {
	SQ_TCONST,
	SQ_TNUMERAL,
	SQ_TTEXT,
	SQ_TFORM,
	SQ_TIMITATION,
	SQ_TFUNCTION,
	SQ_TBOOK,
	SQ_TCODEX,
} sq_vtag;

#define SQ_VSHIFT 4
#define SQ_VMASK_BITS ((1<<SQ_VSHIFT)-1)
#define SQ_VMASK(value, kind) ((value) | (kind))
#define SQ_VTAG(value) ((value) & SQ_VMASK_BITS)
#define SQ_VUNMASK(value) ((value) & ~SQ_VMASK_BITS)
#define SQ_YAY SQ_VMASK((1 << SQ_VSHIFT), SQ_TCONST)
#define SQ_NAY SQ_VMASK(0, SQ_TCONST)
#define SQ_NI SQ_VMASK((2 << SQ_VSHIFT), SQ_TCONST)
#define SQ_UNDEFINED SQ_VMASK((3 << SQ_VSHIFT), SQ_TCONST)

#define SQ_VALUE_ALIGN _Alignas(1<<SQ_VSHIFT)


// #define SQ_TAG(kind) _Generic((kind){0}, \
// 	sq_numeral: SQ_TNUMERAL, \
// 	bool: SQ_TCONST, \
// 	struct sq_text: SQ_TTEXT, \
// 	struct sq_form: SQ_TFORM, \
// 	struct sq_imitation: SQ_TIMITATION, \
// 	struct sq_journey: SQ_TFUNCTION, \
// 	struct sq_array: SQ_TBOOK, \
// 	struct sq_codex: SQ_TCODEX \
// )

// #define sq_value_is(x, kind) (SQ_VTAG(x) == SQ_TAG(kind))
// #define sq_value_as(x, kind) (assert(sq_value_is(x, kind), _Generic(kind, \
// 	sq_numeral: (x) >> SQ_VSHIFT, \
// 	bool: (x) == SQ_YAY, \
// 	struct sq_text *: (kind) SQ_VUNMASK(x), \
// 	struct sq_form *: (kind) SQ_VUNMASK(x), \
// 	struct sq_imitation *: (kind) SQ_VUNMASK(x), \
// 	struct sq_journey *: (kind) SQ_VUNMASK(x), \
// 	struct sq_array *: (kind) SQ_VUNMASK(x), \
// 	struct sq_codex *: (kind) SQ_VUNMASK(x)\

// (SQ_VTAG(x) == SQ_TAG(kind))

#define sq_value_new(x) (_Generic(x, \
	sq_numeral: sq_value_new_numeral, \
	sq_veracity: sq_value_new_veracity, \
	struct sq_text *: sq_value_new_text, \
	struct sq_form *: sq_value_new_form, \
	struct sq_imitation *: sq_value_new_imitation, \
	struct sq_journey *: sq_value_new_function, \
	struct sq_book *: sq_value_new_book, \
	struct sq_codex *: sq_value_new_codex \
)(x))


static inline sq_value sq_value_new_numeral(sq_numeral numeral) {
	assert(numeral == (((sq_numeral) (((sq_value) numeral << SQ_VSHIFT)) >> SQ_VSHIFT)));
	return SQ_VMASK(((sq_value) numeral) << SQ_VSHIFT, SQ_TNUMERAL);
}

static inline sq_value sq_value_new_veracity(sq_veracity veracity) {
	return veracity ? SQ_YAY : SQ_NAY;
}

static inline sq_value sq_value_new_text(struct sq_text *text) {
	assert(!SQ_VTAG((sq_value) text));
	return SQ_VMASK((sq_value) text, SQ_TTEXT);
}

static inline sq_value sq_value_new_form(struct sq_form *form) {
	assert(!SQ_VTAG((sq_value) form));
	return SQ_VMASK((sq_value) form, SQ_TFORM);
}

static inline sq_value sq_value_new_imitation(struct sq_imitation *imitation) {
	assert(!SQ_VTAG((sq_value) imitation));
	return SQ_VMASK((sq_value) imitation, SQ_TIMITATION);
}

static inline sq_value sq_value_new_function(struct sq_journey *function) {
	assert(!SQ_VTAG((sq_value) function));
	return SQ_VMASK((sq_value) function, SQ_TFUNCTION);
}

static inline sq_value sq_value_new_book(struct sq_book *book) {
	assert(!SQ_VTAG((sq_value) book));
	return SQ_VMASK((sq_value) book, SQ_TBOOK);
}

static inline sq_value sq_value_new_codex(struct sq_codex *dict) {
	assert(!SQ_VTAG((sq_value) dict));
	return SQ_VMASK((sq_value) dict, SQ_TCODEX);
}

static inline bool sq_value_is_ni(sq_value value) {
	return value == SQ_NI;
}

static inline bool sq_value_is_numeral(sq_value value) {
	return SQ_VTAG(value) == SQ_TNUMERAL;
}

static inline bool sq_value_is_veracity(sq_value value) {
	return value == SQ_YAY || value == SQ_NAY;
}

static inline bool sq_value_is_text(sq_value value) {
	return SQ_VTAG(value) == SQ_TTEXT;
}

static inline bool sq_value_is_form(sq_value value) {
	return SQ_VTAG(value) == SQ_TFORM;
}

static inline bool sq_value_is_imitation(sq_value value) {
	return SQ_VTAG(value) == SQ_TIMITATION;
}

static inline bool sq_value_is_function(sq_value value) {
	return SQ_VTAG(value) == SQ_TFUNCTION;
}

static inline bool sq_value_is_book(sq_value value) {
	return SQ_VTAG(value) == SQ_TBOOK;
}

static inline bool sq_value_is_codex(sq_value value) {
	return SQ_VTAG(value) == SQ_TCODEX;
}

static inline sq_numeral sq_value_as_numeral(sq_value value) {
	assert(sq_value_is_numeral(value));
	return ((sq_numeral) value) >> SQ_VSHIFT;
}

static inline bool sq_value_as_veracity(sq_value value) {
	assert(sq_value_is_veracity(value));
	return value == SQ_YAY;
}

static inline struct sq_text *sq_value_as_text(sq_value value) {
	assert(sq_value_is_text(value));
	return (struct sq_text *) SQ_VUNMASK(value);
}

static inline struct sq_form *sq_value_as_form(sq_value value) {
	assert(sq_value_is_form(value));
	return (struct sq_form *) SQ_VUNMASK(value);
}

static inline struct sq_imitation *sq_value_as_imitation(sq_value value) {
	assert(sq_value_is_imitation(value));
	return (struct sq_imitation *) SQ_VUNMASK(value);
}

static inline struct sq_journey *sq_value_as_function(sq_value value) {
	assert(sq_value_is_function(value));
	return (struct sq_journey *) SQ_VUNMASK(value);
}

static inline struct sq_book *sq_value_as_book(sq_value value) {
	assert(sq_value_is_book(value));
	return (struct sq_book *) SQ_VUNMASK(value);
}

static inline struct sq_codex *sq_value_as_codex(sq_value value) {
	assert(sq_value_is_codex(value));
	return (struct sq_codex *) SQ_VUNMASK(value);
}

sq_value sq_value_clone(sq_value value);
void sq_value_dump(sq_value value);
void sq_value_dump_to(FILE *out, sq_value value);
void sq_value_free(sq_value value);
const char *sq_value_typename(sq_value value);
sq_value sq_value_kindof(sq_value value);

bool sq_value_not(sq_value arg);
bool sq_value_eql(sq_value lhs, sq_value rhs);
sq_numeral sq_value_cmp(sq_value lhs, sq_value rhs);
sq_value sq_value_neg(sq_value arg);
sq_value sq_value_add(sq_value lhs, sq_value rhs);
sq_value sq_value_sub(sq_value lhs, sq_value rhs);
sq_value sq_value_mul(sq_value lhs, sq_value rhs);
sq_value sq_value_div(sq_value lhs, sq_value rhs);
sq_value sq_value_mod(sq_value lhs, sq_value rhs);
sq_value sq_value_index(sq_value value, sq_value key);
void sq_value_index_assign(sq_value value, sq_value key, sq_value val);

size_t sq_value_length(sq_value value);
struct sq_text *sq_value_to_text(sq_value value);
sq_numeral sq_value_to_numeral(sq_value value);
bool sq_value_to_veracity(sq_value value);
struct sq_book *sq_value_to_book(sq_value value);
struct sq_codex *sq_value_to_codex(sq_value value);

#endif /* !SQ_VALUE_H */
