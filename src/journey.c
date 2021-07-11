#include "journey.h"
#include "text.h"
#include "program.h"
#include "shared.h"
#include "form.h"
#include "book.h"
#include "codex.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

struct sq_journey *sq_journey_clone(struct sq_journey *function) {
	assert(function->refcount);

	if (0 < function->refcount)
		++function->refcount;

	return function;
}

void sq_journey_deallocate(struct sq_journey *function) {
	assert(function->refcount);

	for (unsigned i = 0; i < function->nconsts; ++i)
		sq_value_free(function->consts[i]);

	free(function->name);
	free(function->consts);
	free(function->bytecode);
	free(function);
}

void sq_journey_dump(FILE *out, const struct sq_journey *function) {
	fprintf(out, "Journey(%s, %d arg", function->name, function->argc);

	if (function->argc != 1)
		putc('s', out);

	putc(')', out);
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

static sq_value create_form_imitation(struct sq_form *form, unsigned argc, sq_value *args) {
	// todo: this will fail with functions with an arity not the same as their field count.
	if (form->imitate == NULL) {
		if (argc != form->nmatter)
			die("matter count mismatch (given %d, expected %d) for struct '%s'", argc, form->nmatter, form->name);

		return sq_value_new_imitation(sq_imitation_new(form, args));
	}

	sq_value imitation = sq_value_new_imitation(sq_imitation_new(form, xmalloc(sizeof(sq_value[form->nmatter]))));
	for (unsigned i = 0; i < form->nmatter; ++i)
		sq_value_as_imitation(imitation)->matter[i] = SQ_NI;

	sq_value fn_args[argc + 1];
	fn_args[0] = sq_value_clone(imitation);
	for (unsigned i = 0; i < argc; ++i)
		fn_args[i + 1] = sq_value_clone(args[i]);

	sq_value_free(sq_journey_run(form->imitate, argc + 1, fn_args));

	return imitation;
}

// jmp_buf redo_location;
// jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
// sq_value exception;
// unsigned current_exception_handler;

// struct function_vm_state {
// 	unsigned ip;

// };
#define SET_NEXT_LOCAL() NEXT_LOCAL() = value

sq_value sq_journey_run(const struct sq_journey *function, unsigned argc, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value = SQ_NI;
	enum sq_opcode opcode;

	for (unsigned i = 0; i < argc; ++i)
		locals[i] = args[i];

	for (unsigned i = argc; i < function->nlocals; ++i)
		locals[i] = SQ_NI;

	unsigned ip = 0;

	while (ip < function->codelen) {
		switch ((opcode = function->bytecode[ip++].opcode)) {

	/*** Misc ***/

		case SQ_OC_MOV: {
			unsigned idx1 = NEXT_INDEX();
			unsigned idx2 = NEXT_INDEX();
			locals[idx2] = locals[idx1];
			continue;
		}

		case SQ_OC_INT: {
			unsigned idx;
			struct sq_text *text;

			switch ((idx = NEXT_INDEX())) {
			case SQ_INT_PRINT:
			case SQ_INT_PRINTLN:
				text = sq_value_to_text(NEXT_LOCAL());
				if (idx == SQ_INT_PRINTLN) puts(text->ptr);
				else printf("%s", text->ptr);
				fflush(stdout);
				sq_text_free(text);
				SET_NEXT_LOCAL() = SQ_NI;
				break;

			case SQ_INT_DUMP: {
				value = NEXT_LOCAL();
				sq_value_dump(value);
				putchar('\n');
				SET_NEXT_LOCAL() = value;
				break;
			}

			case SQ_INT_TOTEXT: {
				text = sq_value_to_text(NEXT_LOCAL());
				SET_NEXT_LOCAL() = sq_value_new(text);
				break;
			}

			case SQ_INT_TONUMERAL: {
				sq_numeral numeral = sq_value_to_numeral(NEXT_LOCAL());
				SET_NEXT_LOCAL() = sq_value_new(numeral);
				break;
			}
			
			case SQ_INT_TOVERACITY: {
				sq_veracity veracity = sq_value_to_veracity(NEXT_LOCAL());
				SET_NEXT_LOCAL() = sq_value_new(veracity);
				break;
			}

			case SQ_INT_SUBSTR: {
				text = sq_value_to_text(NEXT_LOCAL());
				sq_numeral start = sq_value_to_numeral(NEXT_LOCAL());
				if (!start--) sq_throw("cannot index by N.");
				sq_numeral count = sq_value_to_numeral(NEXT_LOCAL());
				struct sq_text *result;

				if (!*text->ptr) 
					result = sq_text_new(strdup(""));
				else
					result = sq_text_new(strndup(text->ptr + start, count));

				SET_NEXT_LOCAL() = sq_value_new(result);
				break;
			}

			case SQ_INT_LENGTH:
				value = NEXT_LOCAL();

				SET_NEXT_LOCAL() = sq_value_new((sq_numeral) sq_value_length(value));
				break;

			case SQ_INT_KINDOF: {
				value = NEXT_LOCAL();
			genus_kindof:
				if (sq_value_is_imitation(value))
					SET_NEXT_LOCAL() = sq_value_new_form(sq_value_as_imitation(value)->form);
				else
					SET_NEXT_LOCAL() = sq_value_kindof(value);
				break;
			}

			case SQ_INT_EXIT:
				exit(sq_value_to_numeral(NEXT_LOCAL()));

			case SQ_INT_SYSTEM: {
				text = sq_value_to_text(NEXT_LOCAL());
				char *str = text->ptr;
				FILE *stream = popen(str, "r");

				if (stream == NULL) die("unable to execute command '%s'.", str);

				// sq_text_free(text);

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

				SET_NEXT_LOCAL() = sq_value_new(sq_text_new(result));
				break;
			}

			case SQ_INT_PROMPT: {
				char *line = NULL;
				size_t cap, length;

				if ((length = getline(&line, &cap, stdin)) == (size_t) -1) {
					free(line);
					line = strdup("");
					cap = 0;
					length = 0;
				}

				if (length && line[length-1] == '\n') {
					--length;
					if (length && line[length-1] == '\r')
						--length;
					line[length] = '\0';
				}

				SET_NEXT_LOCAL() = sq_value_new(sq_text_new(line));
				break;
			}

			case SQ_INT_RANDOM:
				SET_NEXT_LOCAL() = sq_value_new((sq_numeral) rand());
				break;

			case SQ_INT_BOOK_NEW: {
				unsigned amnt = NEXT_INDEX();
				sq_value *pages = xmalloc(sizeof(sq_value [amnt]));

				for (unsigned i = 0; i < amnt; ++i)
					pages[i] = NEXT_LOCAL();

				SET_NEXT_LOCAL() = sq_value_new_book(sq_book_new2(amnt, pages));
				break;
			}

			case SQ_INT_CODEX_NEW: {
				unsigned amnt = NEXT_INDEX();
				struct sq_codex_page *pages = xmalloc(sizeof(struct sq_codex_page [amnt]));

				for (unsigned i = 0; i < amnt; ++i) {
					pages[i].key = NEXT_LOCAL();
					pages[i].value = NEXT_LOCAL();
				}

				SET_NEXT_LOCAL() = sq_value_new_codex(sq_codex_new2(amnt, pages));
				break;
			}


			case SQ_INT_ARRAY_INSERT: {
				value = NEXT_LOCAL();
				if (!sq_value_is_book(value)) die("can only insert into books");
				struct sq_book *book = sq_value_as_book(value);

				unsigned index = sq_value_to_numeral(NEXT_LOCAL());
				sq_book_insert2(book, index, NEXT_LOCAL());
				SET_NEXT_LOCAL() = sq_value_clone(value);
				break;
			}

			case SQ_INT_ARRAY_DELETE: {
				value = NEXT_LOCAL();
				sq_value index = NEXT_LOCAL();
				if (sq_value_is_book(value))
					SET_NEXT_LOCAL() = sq_book_delete2(sq_value_as_book(value), sq_value_to_numeral(index));
				else if (sq_value_is_codex(value))
					SET_NEXT_LOCAL() = sq_codex_delete(sq_value_as_codex(value), index);
				else 
					die("can only delete from books and codices");

				break;
			}

			/** ARABIC **/
			case SQ_INT_ARABIC:
				value = NEXT_LOCAL();
				SET_NEXT_LOCAL() = sq_value_new(sq_numeral_to_arabic(sq_value_to_numeral(value)));
				break;

			case SQ_INT_ROMAN:
				value = NEXT_LOCAL();
				SET_NEXT_LOCAL() = sq_value_new(sq_numeral_to_roman(sq_value_to_numeral(value)));
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

		case SQ_OC_COMEFROM: {
			int i, amnt = (int) NEXT_INDEX();

			for (i = 0; i < amnt - 1; ++i)
				if (!fork()) break;

			ip = REL_INDEX(i);
			continue;
		}

		case SQ_OC_JMP_FALSE: {
			value = NEXT_LOCAL();
			unsigned dst = NEXT_INDEX();

			if (!sq_value_to_veracity(value))
				ip = dst;

			continue;
		}

		case SQ_OC_JMP_TRUE: {
			value = NEXT_LOCAL();
			unsigned dst = NEXT_INDEX();

			if (sq_value_to_veracity(value))
				ip = dst;

			continue;
		}

		case SQ_OC_CALL: {
			sq_value imitation_value = NEXT_LOCAL();
			unsigned argc = NEXT_INDEX();

			sq_value newargs[argc];

			for (unsigned i = 0; i < argc; ++i)
				newargs[i] = NEXT_LOCAL();

			if (sq_value_is_function(imitation_value)) {
				struct sq_journey *fn = sq_value_as_function(imitation_value);

				if (argc != fn->argc)
					die("argc mismatch (given %d, expected %d) for func '%s'", argc, fn->argc, fn->name);

				SET_NEXT_LOCAL() = sq_journey_run(fn, argc, newargs);
			} else if (sq_value_is_form(imitation_value)) {
				struct sq_form *form = sq_value_as_form(imitation_value);
				SET_NEXT_LOCAL() = create_form_imitation(
					form,
					argc,
					memdup(newargs, sizeof(sq_value[argc]))
				);
			} else {
				die("can only call funcs, not '%s'", sq_value_typename(imitation_value));
			}

			continue;
		}

		case SQ_OC_RETURN:
			value = NEXT_LOCAL();
			sq_value_clone(value);
			sq_value_clone(value);
			goto done;

		case SQ_OC_THROW:
			sq_throw_value(NEXT_LOCAL());

		case SQ_OC_POPTRYCATCH:
			sq_exception_pop();
			continue;

		case SQ_OC_TRYCATCH: {
			unsigned catch_index = NEXT_INDEX();
			unsigned exception_index = NEXT_INDEX();

			if (!setjmp(exception_handlers[current_exception_handler++]))
				continue;

			locals[exception_index] = exception;
			exception = SQ_NI;
			ip = catch_index;
			continue;
		}

	/*** Operators ***/
		case SQ_OC_EQL:
			value = sq_value_new(sq_value_eql(locals[REL_INDEX(0)], locals[REL_INDEX(1)]));
			ip += 2;
			SET_NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_NEQ:
			value = sq_value_new((sq_veracity) !sq_value_eql(locals[REL_INDEX(0)], locals[REL_INDEX(1)]));
			ip += 2;
			SET_NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_LTH:
			value = sq_value_new((sq_veracity) (sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) < 0));
			ip += 2;
			SET_NEXT_LOCAL() = value;
			sq_value_clone(value);
			continue;

		case SQ_OC_GTH:
			value = sq_value_new((sq_veracity) (sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) > 0));
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;
		case SQ_OC_LEQ:
			value = sq_value_new((sq_veracity) (sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) <= 0));
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;
		case SQ_OC_GEQ:
			value = sq_value_new((sq_veracity) (sq_value_cmp(locals[REL_INDEX(0)], locals[REL_INDEX(1)]) >= 0));
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_ADD:
			value = sq_value_add(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_SUB:
			value = sq_value_sub(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_MUL:
			value = sq_value_mul(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_DIV:
			value = sq_value_div(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_MOD:
			value = sq_value_mod(locals[REL_INDEX(0)], locals[REL_INDEX(1)]);
			ip += 2;
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_NEG:
			value = sq_value_neg(NEXT_LOCAL());
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_NOT:
			value = sq_value_new(sq_value_not(NEXT_LOCAL()));
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_INDEX:
			value = NEXT_LOCAL();
			value = sq_value_index(value, NEXT_LOCAL());
			SET_NEXT_LOCAL() = value;
			break;

		case SQ_OC_INDEX_ASSIGN: {
			value = NEXT_LOCAL();
			sq_value key = NEXT_LOCAL();
			value = NEXT_LOCAL();
			sq_value_index_assign(value, key, value);
			break;
		}
	/*** Constants ***/

		case SQ_OC_CLOAD:
	
			value = function->consts[NEXT_INDEX()];
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			// LOG("loaded local '%llu'", value);
			continue;

	/*** Globals ***/

		case SQ_OC_GLOAD:
			value = function->program->globals[NEXT_INDEX()];
			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		case SQ_OC_GSTORE: {
			unsigned index = NEXT_INDEX();
			// todo: free old value.
			value = function->program->globals[index] = NEXT_LOCAL();
			SET_NEXT_LOCAL() = value;
			sq_value_clone(value);
			sq_value_clone(value);
			continue;
		}

	/*** Struct & imitations ***/

		case SQ_OC_ILOAD: {
			value = NEXT_LOCAL();
			const char *field = sq_value_as_text(function->consts[NEXT_INDEX()])->ptr;

			if (!strcmp(field, "genus"))
				goto genus_kindof;

			if (!strcmp(field, "length"))
				value = sq_value_new((sq_numeral) sq_value_length(value));
			else if (sq_value_is_form(value)) {
				struct sq_form *form = sq_value_as_form(value);
				value = sq_form_lookup(form, field);

				if (value == SQ_UNDEFINED)
					sq_throw("unknown static field '%s' for type '%s'", field, form->name);
			} else if (sq_value_is_imitation(value)) {
				struct sq_imitation *imitation = sq_value_as_imitation(value);
				value = sq_imitation_lookup(imitation, field);

				if (value == SQ_UNDEFINED)
					sq_throw("unknown field '%s' for type '%s'", field, imitation->form->name);
			} else if (sq_value_is_book(value)) {
				if (!strcmp(field, "verso"))
					value = sq_book_index(sq_value_as_book(value), 1);
				else if (!strcmp(field, "recto"))
					value = sq_book_index2(sq_value_as_book(value), -1);
				else
					sq_throw("unknown field '%s' on a book", field);
			} else {
				sq_throw("can only access fields on imitations.");
			}

			sq_value_clone(value);
			SET_NEXT_LOCAL() = value;
			continue;

		}

		case SQ_OC_ISTORE: {
			sq_value target = NEXT_LOCAL();
			sq_value *valueptr;
			const char *field = sq_value_as_text(function->consts[NEXT_INDEX()])->ptr;

			if (sq_value_is_imitation(target)) {
				struct sq_imitation *imitation = sq_value_as_imitation(target);
				valueptr = sq_imitation_lookup_matter(imitation, field);

				if (!valueptr)
					sq_throw("unknown matter '%s' for type '%s'", field, imitation->form->name);
			} else if (sq_value_is_form(target)) {
				struct sq_form *form = sq_value_as_form(target);
				valueptr = sq_form_lookup_essence(form, field);

				if (!valueptr)
					sq_throw("unknown essence '%s' for form '%s'", field, form->name);
			} else if (sq_value_is_book(target)) {
				value = NEXT_LOCAL();
				sq_value_clone(value);
				SET_NEXT_LOCAL() = value;

				if (!strcmp(field, "verso"))
					sq_book_index_assign(sq_value_as_book(target), 1, value);
				else if (!strcmp(field, "recto"))
					sq_book_index_assign2(sq_value_as_book(target), -1, value);
				else
					sq_throw("unknown field '%s' on a book", field);
				continue;
			} else {
				sq_throw("can only access fields on imitations.");
			}

			value = NEXT_LOCAL();
			sq_value_clone(value);
			SET_NEXT_LOCAL() = *valueptr = value;
			continue;
		}

		default:
			bug("unknown opcode '%d'", opcode);
		}
	}

	done:

	// for (unsigned i = 0; i < function->nlocals; ++i)
	// 	sq_value_free(locals[i]);

	return value;
}
