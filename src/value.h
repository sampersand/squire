#ifndef SQ_VALUE_H
#define SQ_VALUE_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "number.h"

struct sq_string;
struct sq_form;
struct sq_imitation;
struct sq_function;
struct sq_array;
struct sq_codex;

typedef uint64_t sq_value;

typedef enum {
	SQ_TCONST,
	SQ_TNUMBER,
	SQ_TSTRING,
	SQ_TFORM,
	SQ_TIMITATION,
	SQ_TFUNCTION,
	SQ_TARRAY,
	SQ_TCODEX,
} sq_vtag;

#define SQ_VSHIFT 4
#define SQ_VMASK_BITS ((1<<SQ_VSHIFT)-1)
#define SQ_VMASK(value, kind) ((value) | (kind))
#define SQ_VTAG(value) ((value) & SQ_VMASK_BITS)
#define SQ_VUNMASK(value) ((value) & ~SQ_VMASK_BITS)
#define SQ_TRUE SQ_VMASK((1 << SQ_VSHIFT), SQ_TCONST)
#define SQ_FALSE SQ_VMASK(0, SQ_TCONST)
#define SQ_NULL SQ_VMASK((2 << SQ_VSHIFT), SQ_TCONST)
#define SQ_UNDEFINED SQ_VMASK((3 << SQ_VSHIFT), SQ_TCONST)

#define SQ_VALUE_ALIGN _Alignas(1<<SQ_VSHIFT)

static inline sq_value sq_value_new_number(sq_number number) {
	assert(number == (((sq_number) (((sq_value) number << SQ_VSHIFT)) >> SQ_VSHIFT)));
	return SQ_VMASK(((sq_value) number) << SQ_VSHIFT, SQ_TNUMBER);
}

static inline sq_value sq_value_new_boolean(bool boolean) {
	return boolean ? SQ_TRUE : SQ_FALSE;
}

static inline sq_value sq_value_new_string(struct sq_string *string) {
	assert(!SQ_VTAG((sq_value) string));
	return SQ_VMASK((sq_value) string, SQ_TSTRING);
}

static inline sq_value sq_value_new_form(struct sq_form *form) {
	assert(!SQ_VTAG((sq_value) form));
	return SQ_VMASK((sq_value) form, SQ_TFORM);
}

static inline sq_value sq_value_new_imitation(struct sq_imitation *imitation) {
	assert(!SQ_VTAG((sq_value) imitation));
	return SQ_VMASK((sq_value) imitation, SQ_TIMITATION);
}

static inline sq_value sq_value_new_function(struct sq_function *function) {
	assert(!SQ_VTAG((sq_value) function));
	return SQ_VMASK((sq_value) function, SQ_TFUNCTION);
}

static inline sq_value sq_value_new_array(struct sq_array *array) {
	assert(!SQ_VTAG((sq_value) array));
	return SQ_VMASK((sq_value) array, SQ_TARRAY);
}

static inline sq_value sq_value_new_codex(struct sq_codex *dict) {
	assert(!SQ_VTAG((sq_value) dict));
	return SQ_VMASK((sq_value) dict, SQ_TCODEX);
}

static inline bool sq_value_is_null(sq_value value) {
	return value == SQ_NULL;
}

static inline bool sq_value_is_number(sq_value value) {
	return SQ_VTAG(value) == SQ_TNUMBER;
}

static inline bool sq_value_is_boolean(sq_value value) {
	return value == SQ_TRUE || value == SQ_FALSE;
}

static inline bool sq_value_is_string(sq_value value) {
	return SQ_VTAG(value) == SQ_TSTRING;
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

static inline bool sq_value_is_array(sq_value value) {
	return SQ_VTAG(value) == SQ_TARRAY;
}

static inline bool sq_value_is_codex(sq_value value) {
	return SQ_VTAG(value) == SQ_TCODEX;
}

static inline sq_number sq_value_as_number(sq_value value) {
	assert(sq_value_is_number(value));
	return ((sq_number) value) >> SQ_VSHIFT;
}

static inline bool sq_value_as_boolean(sq_value value) {
	assert(sq_value_is_boolean(value));
	return value == SQ_TRUE;
}

static inline struct sq_string *sq_value_as_string(sq_value value) {
	assert(sq_value_is_string(value));
	return (struct sq_string *) SQ_VUNMASK(value);
}

static inline struct sq_form *sq_value_as_form(sq_value value) {
	assert(sq_value_is_form(value));
	return (struct sq_form *) SQ_VUNMASK(value);
}

static inline struct sq_imitation *sq_value_as_imitation(sq_value value) {
	assert(sq_value_is_imitation(value));
	return (struct sq_imitation *) SQ_VUNMASK(value);
}

static inline struct sq_function *sq_value_as_function(sq_value value) {
	assert(sq_value_is_function(value));
	return (struct sq_function *) SQ_VUNMASK(value);
}

static inline struct sq_array *sq_value_as_array(sq_value value) {
	assert(sq_value_is_array(value));
	return (struct sq_array *) SQ_VUNMASK(value);
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
sq_number sq_value_cmp(sq_value lhs, sq_value rhs);
sq_value sq_value_neg(sq_value arg);
sq_value sq_value_add(sq_value lhs, sq_value rhs);
sq_value sq_value_sub(sq_value lhs, sq_value rhs);
sq_value sq_value_mul(sq_value lhs, sq_value rhs);
sq_value sq_value_div(sq_value lhs, sq_value rhs);
sq_value sq_value_mod(sq_value lhs, sq_value rhs);
sq_value sq_value_index(sq_value value, sq_value key);
void sq_value_index_assign(sq_value value, sq_value key, sq_value val);

size_t sq_value_length(sq_value value);
struct sq_string *sq_value_to_string(sq_value value);
sq_number sq_value_to_number(sq_value value);
bool sq_value_to_boolean(sq_value value);
struct sq_array *sq_value_to_array(sq_value value);
struct sq_codex *sq_value_to_codex(sq_value value);

#endif /* !SQ_VALUE_H */
