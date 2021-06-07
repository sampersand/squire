#ifndef SQ_VALUE_H
#define SQ_VALUE_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "number.h"

struct sq_string;
struct sq_class;
struct sq_instance;
struct sq_function;
struct sq_array;

typedef uint64_t sq_value;

typedef enum {
	SQ_TCONST,
	SQ_TNUMBER,
	SQ_TSTRING,
	SQ_TCLASS,
	SQ_TINSTANCE,
	SQ_TFUNCTION,
	SQ_TARRAY,
} sq_vtag;

#define SQ_VSHIFT 3
#define SQ_VMASK_BITS ((1<<SQ_VSHIFT)-1)
#define SQ_VMASK(value, kind) ((value) | (kind))
#define SQ_VTAG(value) ((value) & SQ_VMASK_BITS)
#define SQ_VUNMASK(value) ((value) & ~SQ_VMASK_BITS)
#define SQ_TRUE SQ_VMASK((1 << SQ_VSHIFT), SQ_TCONST)
#define SQ_FALSE SQ_VMASK(0, SQ_TCONST)
#define SQ_NULL SQ_VMASK((2 << SQ_VSHIFT), SQ_TCONST)
#define SQ_UNDEFINED SQ_VMASK((3 << SQ_VSHIFT), SQ_TCONST)

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

static inline sq_value sq_value_new_class(struct sq_class *class) {
	assert(!SQ_VTAG((sq_value) class));
	return SQ_VMASK((sq_value) class, SQ_TCLASS);
}

static inline sq_value sq_value_new_instance(struct sq_instance *instance) {
	assert(!SQ_VTAG((sq_value) instance));
	return SQ_VMASK((sq_value) instance, SQ_TINSTANCE);
}

static inline sq_value sq_value_new_function(struct sq_function *function) {
	assert(!SQ_VTAG((sq_value) function));
	return SQ_VMASK((sq_value) function, SQ_TFUNCTION);
}

static inline sq_value sq_value_new_array(struct sq_array *array) {
	assert(!SQ_VTAG((sq_value) array));
	return SQ_VMASK((sq_value) array, SQ_TARRAY);
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

static inline bool sq_value_is_class(sq_value value) {
	return SQ_VTAG(value) == SQ_TCLASS;
}

static inline bool sq_value_is_instance(sq_value value) {
	return SQ_VTAG(value) == SQ_TINSTANCE;
}

static inline bool sq_value_is_function(sq_value value) {
	return SQ_VTAG(value) == SQ_TFUNCTION;
}

static inline bool sq_value_is_array(sq_value value) {
	return SQ_VTAG(value) == SQ_TARRAY;
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

static inline struct sq_class *sq_value_as_class(sq_value value) {
	assert(sq_value_is_class(value));
	return (struct sq_class *) SQ_VUNMASK(value);
}

static inline struct sq_instance *sq_value_as_instance(sq_value value) {
	assert(sq_value_is_instance(value));
	return (struct sq_instance *) SQ_VUNMASK(value);
}

static inline struct sq_function *sq_value_as_function(sq_value value) {
	assert(sq_value_is_function(value));
	return (struct sq_function *) SQ_VUNMASK(value);
}

static inline struct sq_array *sq_value_as_array(sq_value value) {
	assert(sq_value_is_array(value));
	return (struct sq_array *) SQ_VUNMASK(value);
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

struct sq_string *sq_value_to_string(sq_value value);
sq_number sq_value_to_number(sq_value value);
bool sq_value_to_boolean(sq_value value);

#endif /* !SQ_VALUE_H */
