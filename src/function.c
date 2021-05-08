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
	}
*/

#define REL_INDEX(ip) (function->code[ip].index)
#define NEXT_INDEX() (REL_INDEX(ip++))

#define LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__);

sq_value sq_function_run(struct sq_function *function, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value;
	enum sq_opcode opcode;

	LOG("starting function '%s'", function->name);
	for (unsigned i = 0; i < function->nlocals; ++i)
		locals[i] = SQ_NULL;

	unsigned ip = 0;

	while (true) {
		opcode = function->code[ip++].opcode;
		LOG("loaded opcode '%d'", opcode);

		switch (opcode) {

	/*** Loading Values ***/

		case SQ_OC_CLOAD:
			value = function->consts[NEXT_INDEX()];
			sq_value_clone(value);
			locals[NEXT_INDEX()] = value;
			continue;

		case SQ_OC_GLOAD:
			value = function->program->globals[NEXT_INDEX()].value;
			sq_value_clone(value);
			locals[NEXT_INDEX()] = value;
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
			locals[NEXT_INDEX()] = value;
			continue;

		}

		case SQ_OC_ALOAD:
			die("todo: SQ_OC_ALOAD");

	/*** Storing Values ***/

		case SQ_OC_GSTORE: {
			sq_value *global = &function->program->globals[NEXT_INDEX()].value;
			value = locals[NEXT_INDEX()];
			*global = value;
			sq_value_clone(value);
			continue;
		}

		case SQ_OC_ISTORE: {
			sq_value instance_value = locals[NEXT_INDEX()];

			if (!sq_value_is_instance(instance_value))
				die("can only access fields on instances.");

			struct sq_instance *instance = sq_value_as_instance(instance_value);
			const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
			sq_value *valueptr = sq_instance_field(instance, field);
			if (!valueptr)
				die("unknown field '%s' for type '%s'", field, instance->kind->name);

			value = locals[NEXT_INDEX()];
			*valueptr = value;
			sq_value_clone(value);
			continue;
		}

		case SQ_OC_ASTORE:
			die("todo: SQ_OC_ASTORE");


	/*** Misc ***/

		case SQ_OC_SWAP: {
			unsigned idx1 = NEXT_INDEX();
			unsigned idx2 = NEXT_INDEX();
			value = locals[idx1];
			locals[idx1] = locals[idx2];
			locals[idx2] = value;
			continue;
		}

		case SQ_OC_INEW: // create a struct instance
			die("todo: SQ_OC_INEW");


	/*** Control Flow ***/

		case SQ_OC_RETURN:
			value = locals[NEXT_INDEX()];
			sq_value_clone(value);
			break;

		case SQ_OC_JMP_FALSE: {
			value = locals[NEXT_INDEX()];
			unsigned dst = NEXT_INDEX();

			if (!sq_value_is_boolean(value))
				die("can only jump on booleans");

			if (sq_value_as_boolean(value))
				ip = dst;

			continue;
		}

		case SQ_OC_JMP:
			ip = REL_INDEX(0);
			continue;

		case SQ_OC_CALL:
			die("todo: SQ_OC_CALL");

	/*** Math ***/
		case SQ_OC_EQL:
			value = sq_value_new_boolean(sq_value_eql(REL_INDEX(0), REL_INDEX(1)));
			ip += 2;
			continue;

		case SQ_OC_LTH:
			value = sq_value_new_boolean(sq_value_lth(REL_INDEX(0), REL_INDEX(1)));
			ip += 2;
			continue;

		case SQ_OC_GTH:
			value = sq_value_new_boolean(sq_value_gth(REL_INDEX(0), REL_INDEX(1)));
			ip += 2;
			continue;

		case SQ_OC_ADD:
			value = sq_value_add(REL_INDEX(0), REL_INDEX(1));
			ip += 2;
			continue;

		case SQ_OC_SUB:
			value = sq_value_sub(REL_INDEX(0), REL_INDEX(1));
			ip += 2;
			continue;

		case SQ_OC_MUL:
			value = sq_value_mul(REL_INDEX(0), REL_INDEX(1));
			ip += 2;
			continue;

		case SQ_OC_DIV:
			value = sq_value_div(REL_INDEX(0), REL_INDEX(1));
			ip += 2;
			continue;

		case SQ_OC_MOD:
			value = sq_value_mod(REL_INDEX(0), REL_INDEX(1));
			ip += 2;
			continue;

		case SQ_OC_NOT:
			value = sq_value_not(NEXT_INDEX());
			continue;

		default:
			die("unknown opcode '%d'", opcode);
		}
	}

	for (unsigned i = 0; i < function->nlocals; ++i)
		sq_value_free(locals[i]);

	return value;
}