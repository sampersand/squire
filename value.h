#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

struct sq_string;
struct sq_instance;
struct sq_function;

typedef intptr_t sq_number;
typedef uintptr_t sq_value;

typedef enum {
	SQ_VK_BOOLEAN,
	SQ_VK_NULL,
	SQ_VK_NUMBER,
	SQ_VK_STRING,
	SQ_VK_INSTANCE,
	SQ_VK_FUNCTION,
} sq_vtag;

#define SQ_VSHIFT 3
#define SQ_VMASK_BITS ((1<<SQ_VSHIFT)-1)
#define SQ_VMASK(value, kind) ((value) | (kind))
#define SQ_VTAG(value) ((value) & SQ_VMASK_BITS)
#define SQ_VUNMASK(value) ((value) & ~SQ_VMASK_BITS)
#define SQ_TRUE SQ_VMASK((true << SQ_VSHIFT), SQ_VK_BOOLEAN)
#define SQ_FALSE SQ_VMASK(false, SQ_VK_BOOLEAN)
#define SQ_NULL SQ_VMASK(0, SQ_VK_NULL)

static inline sq_value sq_value_new_number(sq_number number) {
	assert((((number) << SQ_VSHIFT) >> SQ_VSHIFT) == number);
	return SQ_VMASK(((sq_value) number) << SQ_VSHIFT, SQ_VK_NUMBER);
}

static inline sq_value sq_value_new_boolean(bool boolean) {
	return boolean ? SQ_TRUE : SQ_FALSE;
}

static inline sq_value sq_value_new_string(struct sq_string *string) {
	assert(!SQ_VTAG((sq_value) string));
	return SQ_VMASK((sq_value) string, SQ_VK_STRING);
}

static inline sq_value sq_value_new_instance(struct sq_instance *instance) {
	assert(!SQ_VTAG((sq_value) instance));
	return SQ_VMASK((sq_value) instance, SQ_VK_INSTANCE);
}

static inline sq_value sq_value_new_function(struct sq_function *function) {
	assert(!SQ_VTAG((sq_value) function));
	return SQ_VMASK((sq_value) function, SQ_VK_FUNCTION);
}

static inline bool sq_value_is_null(sq_value value) {
	return value == SQ_NULL;
}

static inline bool sq_value_is_number(sq_value value) {
	return SQ_VTAG(value) == SQ_VK_NUMBER;
}

static inline bool sq_value_is_boolean(sq_value value) {
	return value == SQ_TRUE || value == SQ_FALSE;
}

static inline bool sq_value_is_string(sq_value value) {
	return SQ_VTAG(value) == SQ_VK_STRING;
}

static inline bool sq_value_is_instance(sq_value value) {
	return SQ_VTAG(value) == SQ_VK_INSTANCE;
}

static inline bool sq_value_is_function(sq_value value) {
	return SQ_VTAG(value) == SQ_VK_FUNCTION;
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

static inline struct sq_instance *sq_value_as_instance(sq_value value) {
	assert(sq_value_is_instance(value));
	return (struct sq_instance *) SQ_VUNMASK(value);
}

static inline struct sq_function *sq_value_as_function(sq_value value) {
	assert(sq_value_is_function(value));
	return (struct sq_function *) SQ_VUNMASK(value);
}

void sq_value_clone(sq_value value);
void sq_value_free(sq_value value);
