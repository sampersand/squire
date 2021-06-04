#include "function.h"
#include "string.h"
#include "program.h"
#include "shared.h"
#include "class.h"
#include "array.h"
#include "roman.h"
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
	return; // todo: fixme?
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

static sq_value create_class_instance(struct sq_class *class, sq_value *args, unsigned argc) {
	// todo: this will fail with functions with an arity not the same as their field count.
	if (class->constructor == NULL) {
		if (argc != class->nfields)
			die("fields mismatch (given %d, expected %d) for struct '%s'", argc, class->nfields, class->name);
		return sq_value_new_instance(sq_instance_new(class, memdup(args, sizeof(sq_value[argc]))));
	}

	sq_value instance = sq_value_new_instance(sq_instance_new(class, xmalloc(sizeof(sq_value[class->nfields]))));
	for (unsigned i = 0; i < class->nfields; ++i)
		sq_value_as_instance(instance)->fields[i] = SQ_NULL;

	sq_value fn_args[argc+1];
	for (unsigned i = 0; i < argc; ++i)
		fn_args[i+1] = sq_value_clone(args[i]);

	fn_args[0] = sq_value_clone(instance);
	sq_value_free(sq_function_run(class->constructor, argc+1, fn_args));

	return instance;
}

sq_value sq_function_run(struct sq_function *function, unsigned argc, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value;
	enum sq_opcode opcode;

	for (unsigned i = 0; i < argc; ++i)
		locals[i] = args[i];

	for (unsigned i = argc; i < function->nlocals; ++i)
		locals[i] = SQ_NULL;

	unsigned ip = 0;

	while (true) {
		if (ip == function->codelen) {
			value = SQ_NULL;
			goto done;
		}

		opcode = function->bytecode[ip++].opcode;

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
				fflush(stdout);
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

			case SQ_INT_LENGTH:
				value = NEXT_LOCAL();

				if (sq_value_is_array(value)) {
					value = sq_value_new_number(sq_value_as_array(value)->len);
				} else {
					string = sq_value_to_string(value);
					value = sq_value_new_number(string->length);
				}

				NEXT_LOCAL() = value;
				break;

			case SQ_INT_KINDOF: {
				value = NEXT_LOCAL();
				if (sq_value_is_instance(value))
					NEXT_LOCAL() = sq_value_new_class(sq_value_as_instance(value)->class);
				else
					NEXT_LOCAL() = sq_value_kindof(value);
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

			case SQ_INT_ARRAY_NEW: {
				unsigned amnt = NEXT_INDEX();
				sq_value *eles = xmalloc(sizeof(sq_value [amnt]));

				for (unsigned i = 0; i < amnt; ++i) {
					eles[i] = NEXT_LOCAL();
				}

				NEXT_LOCAL() = sq_value_new_array(sq_array_new(amnt, eles));
				break;
			}


			case SQ_INT_ARRAY_INSERT: {
				value = NEXT_LOCAL();
				if (!sq_value_is_array(value)) die("can only insert into arrays");
				struct sq_array *array = sq_value_as_array(value);

				unsigned index = sq_value_to_number(NEXT_LOCAL());
				sq_array_insert(array, index, NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_clone(value);
				break;
			}

			case SQ_INT_ARRAY_DELETE: {
				value = NEXT_LOCAL();
				if (!sq_value_is_array(value)) die("can only delete from arrays");
				struct sq_array *array = sq_value_as_array(value);
				unsigned index = sq_value_to_number(NEXT_LOCAL());
				NEXT_LOCAL() = sq_array_delete(array, index);
				break;
			}

			case SQ_INT_ARRAY_INDEX: {
				value = NEXT_LOCAL();
				if (!sq_value_is_array(value)) die("can only index into arrays");
				struct sq_array *array = sq_value_as_array(value);

				unsigned index = sq_value_to_number(NEXT_LOCAL());
				NEXT_LOCAL() = sq_array_index(array, index);
				break;
			}

			case SQ_INT_ARRAY_INDEX_ASSIGN: {
				value = NEXT_LOCAL();
				if (!sq_value_is_array(value)) die("can only index assign into arrays");
				struct sq_array *array = sq_value_as_array(value);

				unsigned index = sq_value_to_number(NEXT_LOCAL());
				sq_array_index_assign(array, index, value = NEXT_LOCAL());
				NEXT_LOCAL() = sq_value_clone(value);
				break;
			}

			/** ARABIC **/
			case SQ_INT_ARABIC:
				value = NEXT_LOCAL();
				NEXT_LOCAL() = sq_value_new_string(sq_string_new(sq_number_to_arabic(sq_value_to_number(value))));
				break;

			case SQ_INT_ROMAN:
				value = NEXT_LOCAL();
				NEXT_LOCAL() = sq_value_new_string(sq_string_new(sq_number_to_roman(sq_value_to_number(value))));
				break;

			default:
				bug("unknown interrupt: %d", idx);
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
			} else if (sq_value_is_class(instance_value)) {
				struct sq_class *class = sq_value_as_class(instance_value);
				NEXT_LOCAL() = create_class_instance(class, newargs, argc);
			} else {
				die("can only call funcs, not '%s'", sq_value_typename(instance_value));
			}
			continue;
		}

		case SQ_OC_RETURN:
			value = NEXT_LOCAL();
			sq_value_clone(value);
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
			unsigned index = NEXT_INDEX();
			// todo: free old value.
			value = function->program->globals[index] = NEXT_LOCAL();
			NEXT_LOCAL() = value;
			sq_value_clone(value);
			sq_value_clone(value);
			continue;
		}

	/*** Struct & Instances ***/

		case SQ_OC_ILOAD: {
			value = NEXT_LOCAL();

			if (sq_value_is_class(value)) {
				struct sq_class *class = sq_value_as_class(value);
				const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
				value = sq_class_field(class, field);

				if (value == SQ_UNDEFINED)
					die("unknown static field '%s' for type '%s'", field, class->name);
			} else if (sq_value_is_instance(value)) {
				struct sq_instance *instance = sq_value_as_instance(value);
				const char *field = sq_value_as_string(function->consts[NEXT_INDEX()])->ptr;
				sq_value *valueptr = sq_instance_field(instance, field);
				struct sq_function *method;

				// i've given up on this function lol.

				if (valueptr)
					value = *valueptr;
				else if ((method = sq_instance_method(instance, field)))
					value =  sq_value_new_function(sq_function_clone(method));
				else
					die("unknown field '%s' for type '%s'", field, instance->class->name);
			} else  {
				die("can only access fields on instances.");
			}

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
				die("unknown field '%s' for type '%s'", field, instance->class->name);

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
