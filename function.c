#include "function.h"
#include "string.h"
#include "program.h"
#include "shared.h"
#include "struct.h"
#include <assert.h>
#include <stdlib.h>

void sq_function_clone(struct sq_function *function) {
	assert(function->refcount);

	if (0 < function->refcount)
		++function->refcount;
}

void sq_function_free(struct sq_function *function) {
	assert(function->refcount);

	if (function->refcount < 0 || !--function->refcount)
		return;

	for (unsigned i = 0; i < function->nconsts; ++i)
		sq_value_free(function->consts[i]);

	free(function->name);
	free(function->consts);
	free(function->code);
	free(function);
}


/*
typedef enum {
	SQ_OC_SWAP,

	SQ_OC_JMP,
	SQ_OC_JMP_FALSE,
	SQ_OC_CALL,
	SQ_OC_RETURN,

	SQ_OC_EQL,
	SQ_OC_NEQ,
	SQ_OC_LTH,
	SQ_OC_GTH,
	SQ_OC_ADD,
	SQ_OC_SUB,
	SQ_OC_MUL,
	SQ_OC_DIV,
	SQ_OC_MOD,
	SQ_OC_NOT,

	SQ_OC_INEW, // create a struct instance

	SQ_OC_CLOAD,  // load a constant
	SQ_OC_GLOAD,  // load a global
	SQ_OC_ILOAD,  // load an instance field
	SQ_OC_ALOAD,  // load an array index

	SQ_OC_GSTORE, // store a global
	SQ_OC_ISTORE, // store an instance field
	SQ_OC_ASTORE, // store an array index
*/

#define NEXT_INDEX() (function->code[ip++].index)

#define LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__);

sq_value sq_function_run(struct sq_function *function, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value;
	sq_opcode opcode;

	LOG("starting function '%s'", function->name);
	for (unsigned i = 0; i < function->nlocals; ++i)
		locals[i] = SQ_NULL;

	unsigned ip = 0;

	for (;; locals[NEXT_INDEX()] = value) {
	top:
		opcode = function->code[ip++].opcode;
		
		LOG("loaded opcode '%d'", opcode);


		switch (opcode) {
		case SQ_OC_CLOAD:
			value = function->consts[NEXT_INDEX()];
			sq_value_clone(value);
			continue;

		case SQ_OC_GLOAD:
			value = function->program->globals[NEXT_INDEX()].value;
			sq_value_clone(value);
			continue;

		case SQ_OC_ILOAD: {
			value = locals[NEXT_INDEX()];

			if (!sq_value_is_instance(value))
				die("can only access fields on instances.");

			struct sq_instance *instance = sq_value_as_instance(value);
			const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
			sq_value *valueptr = sq_instance_field(instance, field);

			if (!valueptr)
				die("unknown field '%s' for type '%s'", field, instance->kind->name);

			value = *valueptr;
			sq_value_clone(value);
			continue;
		}

		case SQ_OC_RETURN:
			value = locals[NEXT_INDEX()];
			sq_value_clone(value);
			goto done;

		default:
			die("unknown opcode '%d'", opcode);
		}
	}

	done:

	for (unsigned i = 0; i < function->nlocals; ++i)
		sq_value_free(locals[i]);

	return value;
}