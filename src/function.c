#include "function.h"
#include "string.h"
#include "program.h"
#include "shared.h"
#include "struct.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct sq_function *sq_function_clone(struct sq_function *function) {
	assert(function->refcount);

	if (0 < function->refcount)
		++function->refcount;

	return function;
}

void sq_function_free(struct sq_function *function) {
	return;
	assert(function->refcount);

	if (function->refcount < 0 || !--function->refcount)
		return;

	for (unsigned i = 0; i < function->nconsts; ++i)
		sq_value_free(function->consts[i]);

	free(function->name);
	free(function->consts);
	free(function->bytecode);
	free(function);
}

void sq_function_dump(const struct sq_function *function) {
	printf("Function(%s, %d arg", function->name, function->argc);

	if (function->argc != 1)
		putchar('s');

	putchar(')');
}

#define ABS_INDEX(idx) (function->bytecode[idx].index)
#define REL_INDEX(idx) ABS_INDEX((idx)+ip)
#define NEXT_INDEX() ABS_INDEX((ip)++)
#define NEXT_LOCAL() locals[NEXT_INDEX()]

#ifdef NDEBUG
// #define LOG(fmt, ...)
#else
// #define LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__);
#endif

sq_value sq_function_run(struct sq_function *function, unsigned argc, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value;
	enum sq_opcode opcode;

	for (unsigned i = 0; i < argc; ++i) {
		locals[i] = args[i];
	}

	// printf("starting function '%s'\n", function->name);
	for (unsigned i = argc; i < function->nlocals; ++i)
		locals[i] = SQ_NULL;

	unsigned ip = 0;

	while (true) {
		opcode = function->bytecode[ip++].opcode;
		// LOG("loaded opcode '%02x'", opcode);

		switch (opcode) {


	/*** Misc ***/

		case SQ_OC_SWAP: {
			unsigned idx1 = NEXT_INDEX();
			unsigned idx2 = NEXT_INDEX();
			value = locals[idx1];
			locals[idx1] = locals[idx2];
			locals[idx2] = value;
			continue;
		}

		case SQ_OC_MOV: {
			unsigned idx1 = NEXT_INDEX();
			unsigned idx2 = NEXT_INDEX();
			locals[idx2] = locals[idx1];
			continue;
		}

		case SQ_OC_INT: {
			unsigned idx;
			struct sq_string *string;

			switch ((idx = NEXT_INDEX())) {
			case SQ_INT_PRINT: {
				string = sq_value_to_string(NEXT_LOCAL());
				printf("%s", string->ptr);
				sq_string_free(string);
				NEXT_LOCAL() = SQ_NULL;
				break;
			}
			case SQ_INT_DUMP: {
				value = NEXT_LOCAL();
				sq_value_dump(value);
				putchar('\n');
				NEXT_LOCAL() = value;
				break;
			}

			case SQ_INT_TOSTRING: {
				string = sq_value_to_string(NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_new_string(string);
				break;
			}

			case SQ_INT_TONUMBER: {
				sq_number number = sq_value_to_number(NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_new_number(number);
				break;
			}
			
			case SQ_INT_TOBOOLEAN: {
				bool boolean = sq_value_to_boolean(NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_new_boolean(boolean);
				break;
			}

			case SQ_INT_SUBSTR: {
				string = sq_value_to_string(NEXT_LOCAL());
				sq_number start = sq_value_to_number(NEXT_LOCAL());
				sq_number count = sq_value_to_number(NEXT_LOCAL());
				struct sq_string *result;

				if (!*string->ptr) 
					result = sq_string_new(strdup(""));
				else
					result = sq_string_new(strndup(string->ptr + start, count));

				NEXT_LOCAL() = sq_value_new_string(result);
				break;
			}

			case SQ_INT_LENGTH: {
				string = sq_value_to_string(NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_new_number(string->length);
				break;
			}

			case SQ_INT_KINDOF: {
				value = NEXT_LOCAL();
				if (sq_value_is_instance(value))
					NEXT_LOCAL() = sq_value_new_struct(sq_value_as_instance(value)->kind);
				else
					NEXT_LOCAL() = sq_value_new_string(sq_string_new(strdup(sq_value_typename(value))));
				break;
			}

			case SQ_INT_EXIT:
				exit(sq_value_to_number(NEXT_LOCAL()));

			case SQ_INT_SYSTEM: {
				string = sq_value_to_string(NEXT_LOCAL());
				char *str = string->ptr;
				FILE *stream = popen(str, "r");

				if (stream == NULL) die("unable to execute command '%s'.", str);

				// sq_string_free(string);

				size_t tmp;
				size_t capacity = 2048;
				size_t length = 0;
				char *result = xmalloc(capacity);

				// try to read the entire stream's stdout to `result`.
				while (0 != (tmp = fread(result + length, 1, capacity - length, stream))) {
					length += tmp;

					if (length == capacity) {
						capacity *= 2;
						result = xrealloc(result, capacity);
					}
				}

				// Abort if `stream` had an error.
				if (ferror(stream)) die("unable to read command stream");

				result = xrealloc(result, length + 1);
				result[length] = '\0';

				// Abort if we cant close stream.
				if (pclose(stream) == -1)
					die("unable to close command stream");

				NEXT_LOCAL() = sq_value_new_string(sq_string_new(result));
				break;
			}

			case SQ_INT_PROMPT: {
				char *line = NULL;
				size_t cap, length;

				if ((length = getline(&line, &cap, stdin)) == (size_t) -1) {
					line = strdup("");
					cap = 0;
				}

				if (length && line[length-1] == '\n') {
					--length;
					if (length && line[length-1] == '\r')
						--length;
					line[length] = '\0';
				}

				NEXT_LOCAL() = sq_value_new_string(sq_string_new(line));
				break;
			}

			case SQ_INT_RANDOM:
				NEXT_LOCAL() = sq_value_new_number(rand());
				break;

			default:
				bug("unknown index: %d", idx);
			}

			continue;
		}

		case SQ_OC_NOOP:
			continue;


	/*** Control Flow ***/

		case SQ_OC_JMP:
			ip = REL_INDEX(0);
			continue;

		case SQ_OC_JMP_FALSE: {
			value = NEXT_LOCAL();
			unsigned dst = NEXT_INDEX();

			if (!sq_value_to_boolean(value))
				ip = dst;

			continue;
		}

		case SQ_OC_JMP_TRUE: {
			value = NEXT_LOCAL();
			unsigned dst = NEXT_INDEX();

			if (sq_value_to_boolean(value))
				ip = dst;

			continue;
		}

		case SQ_OC_CALL: {
			sq_value instance_value = NEXT_LOCAL();
			unsigned argc = NEXT_INDEX();

			sq_value newargs[argc];

			for (unsigned i = 0; i < argc; ++i)
				newargs[i] = NEXT_LOCAL();

			if (sq_value_is_function(instance_value)) {
				struct sq_function *fn = sq_value_as_function(instance_value);
				if (argc != fn->argc)
					die("argc mismatch (given %d, expected %d) for func '%s'", argc, fn->argc, fn->name);
				NEXT_LOCAL() = sq_function_run(fn, argc, newargs);
			} else if (sq_value_is_struct(instance_value)) {
				struct sq_struct *kind = sq_value_as_struct(instance_value);
				if (argc != kind->nfields)
					die("fields mismatch (given %d, expected %d) for struct '%s'", argc, kind->nfields, kind->name);
				NEXT_LOCAL() = sq_value_new_instance(
					sq_instance_new(kind, memdup(newargs, sizeof(sq_value[argc]))));
			} else {
				die("can only call funcs, not '%s'", sq_value_typename(instance_value));
			}
			continue;
		}

		case SQ_OC_RETURN:
			value = NEXT_LOCAL();
			sq_value_clone(value);
			goto done;

	/*** Math ***/
		case SQ_OC_EQL:
			value = sq_value_new_boolean(sq_value_eql(locals[REL_INDEX(0)], locals[REL_INDEX(1)]));
			ip += 2;
			NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_NEQ:
			value = sq_value_new_boolean(!sq_value_eql(locals[REL_INDEX(0)], locals[REL_INDEX(1)]));
			ip += 2;
			NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_LTH:
			value = sq_value_new_boolean(sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) < 0);
			ip += 2;
			NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_GTH:
			value = sq_value_new_boolean(sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) > 0);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;
		case SQ_OC_LEQ:
			value = sq_value_new_boolean(sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) <= 0);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;
		case SQ_OC_GEQ:
			value = sq_value_new_boolean(sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) >= 0);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_ADD:
			value = sq_value_add(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_SUB:
			value = sq_value_sub(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_MUL:
			value = sq_value_mul(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_DIV:
			value = sq_value_div(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_MOD:
			value = sq_value_mod(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_NEG:
			value = sq_value_neg(NEXT_LOCAL());
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_NOT:
			value = sq_value_new_boolean(sq_value_not(NEXT_LOCAL()));
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;


	/*** Constants ***/

		case SQ_OC_CLOAD:
			value = function->consts[NEXT_INDEX()];
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			// LOG("loaded local '%llu'", value);
			continue;

	/*** Globals ***/

		case SQ_OC_GLOAD:
			value = function->program->globals[NEXT_INDEX()];
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		case SQ_OC_GSTORE: {
			value = NEXT_LOCAL();
			function->program->globals[NEXT_INDEX()] = value;
			NEXT_LOCAL() = value;
			sq_value_clone(value);
			sq_value_clone(value);
			continue;
		}

	/*** Struct & Instances ***/

		case SQ_OC_ILOAD: {
			value = NEXT_LOCAL();

			if (!sq_value_is_instance(value))
				die("can only access fields on instances.");

			struct sq_instance *instance = sq_value_as_instance(value);
			const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
			sq_value *valueptr = sq_instance_field(instance, field);

			if (!valueptr)
				die("unknown field '%s' for type '%s'", field, instance->kind->name);

			value = *valueptr;
			sq_value_clone(value);
			NEXT_LOCAL() = value;
			continue;

		}

		case SQ_OC_ISTORE: {
			sq_value instance_value = NEXT_LOCAL();

			if (!sq_value_is_instance(instance_value))
				die("can only access fields on instances.");

			struct sq_instance *instance = sq_value_as_instance(instance_value);
			const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
			sq_value *valueptr = sq_instance_field(instance, field);
			if (!valueptr)
				die("unknown field '%s' for type '%s'", field, instance->kind->name);

			value = NEXT_LOCAL();
			sq_value_clone(value);
			NEXT_LOCAL() = *valueptr = value;
			continue;
		}

		case SQ_OC_INEW: // create a struct instance
			die("todo: SQ_OC_INEW");

		default:
			die("unknown opcode '%d'", opcode);
		}
	}

	done:

	// for (unsigned i = 0; i < function->nlocals; ++i)
	// 	sq_value_free(locals[i]);

	return value;
}
