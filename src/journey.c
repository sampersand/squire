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

struct sq_stackframe {
	unsigned ip;
	const struct sq_journey *journey;
	sq_value *locals;
};

static void setup_stackframe(struct sq_stackframe *stackframe, struct sq_args args);
static sq_value run_stackframe(struct sq_stackframe *stackframe);
static void teardown_stackframe(struct sq_stackframe *stackframe);

sq_value sq_journey_run(const struct sq_journey *journey, struct sq_args args) {
	sq_value locals[journey->nlocals];
	struct sq_stackframe stackframe = { .ip = 0, .journey = journey, .locals = locals };

	setup_stackframe(&stackframe, args);
	sq_value result = run_stackframe(&stackframe);
	teardown_stackframe(&stackframe);

	return result;
}

static void setup_stackframe(struct sq_stackframe *stackframe, struct sq_args args) {
	if (stackframe->journey->argc != args.pargc)
		sq_throw("argument mismatch: expected %d, given %d", stackframe->journey->argc, args.pargc);

	unsigned i = 0;

	for (; i < args.pargc; ++i)
		stackframe->locals[i] = args.pargv[i];

	for (; i < stackframe->journey->nlocals; ++i)
		stackframe->locals[i] = SQ_NI;
}

static void teardown_stackframe(struct sq_stackframe *stackframe) {
	for (unsigned i = 0; i < stackframe->journey->nlocals; ++i)
		sq_value_free(stackframe->locals[i]);
}

static inline union sq_bytecode next_bytecode(struct sq_stackframe *sf) {
	return sf->journey->bytecode[sf->ip++];
}

static inline unsigned next_index(struct sq_stackframe *sf) {
	return next_bytecode(sf).index;
}

static inline unsigned next_count(struct sq_stackframe *sf) {
	return next_bytecode(sf).count;
}

static inline sq_value *next_local(struct sq_stackframe *sf) {
	return &sf->locals[next_index(sf)];
}

static void set_local(struct sq_stackframe *sf, unsigned index, sq_value value) {
	assert(index <= sf->journey->nlocals);

	sq_value_free(sf->locals[index]);
	sf->locals[index] = value;
}

static void set_next_local(struct sq_stackframe *sf, sq_value value) {
	set_local(sf, next_index(sf), value);
}

static inline unsigned next_relative_index(struct sq_stackframe *sf) {
	return next_index(sf) + sf->ip;
}

#define ABS_INDEX(idx) (function->bytecode[idx].index)
#define RELATIVE_INDE(idx) ABS_INDEX((idx)+ip)
#define NEXT_INDEX() ABS_INDEX((ip)++)
#define NEXT_LOCAL() locals[NEXT_INDEX()]

#define MAX_INTERRUPT_OPERAND_COUNT 3
static unsigned interrupt_operands(enum sq_interrupt interrupt) {
	switch (interrupt) {
	case SQ_INT_TONUMERAL:
	case SQ_INT_TOTEXT:
	case SQ_INT_TOVERACITY:
	case SQ_INT_KINDOF:
	case SQ_INT_PRINT:
	case SQ_INT_PRINTLN:
	case SQ_INT_DUMP:
	case SQ_INT_SYSTEM:
	case SQ_INT_EXIT:
	case SQ_INT_LENGTH:
	case SQ_INT_ARABIC:
	case SQ_INT_ROMAN:
		return 1;

	case SQ_INT_PROMPT:
	case SQ_INT_RANDOM:
		return 0;

	case SQ_INT_SUBSTR:
	case SQ_INT_ARRAY_INSERT:
		return 3;

	case SQ_INT_ARRAY_DELETE:
		return 2;

	case SQ_INT_CODEX_NEW:
	case SQ_INT_BOOK_NEW:
		return 0;
	}
}

static void handle_interrupt(struct sq_stackframe *sf) {
	enum sq_interrupt interrupt = next_bytecode(sf).interrupt;
	sq_value operands[MAX_INTERRUPT_OPERAND_COUNT];
	struct sq_text *text;

	for (unsigned i = 0; i < interrupt_operands(interrupt); ++i)
		operands[i] = *next_local(sf);

	switch (interrupt) {
	// [A,DST] DST <- A.to_numeral()
	case SQ_INT_TONUMERAL:
		set_next_local(sf, sq_value_new(sq_value_to_numeral(operands[0])));
		return;

	// [A,DST] DST <- A.to_text()
	case SQ_INT_TOTEXT:
		set_next_local(sf, sq_value_new(sq_value_to_text(operands[0])));
		return;

	// [A,DST] DST <- A.to_veracity()
	case SQ_INT_TOVERACITY:
		set_next_local(sf, sq_value_new(sq_value_to_veracity(operands[0])));
		return;

	// [A,DST] DST <- A.genus
	case SQ_INT_KINDOF:
		set_next_local(sf, sq_value_clone(sq_value_genus(operands[0])));
		return;

	// [A,DST] Print `A`, DST <- ni
	case SQ_INT_PRINT:
		text = sq_value_to_text(operands[0]);
		if (!fputs(text->ptr, stdout))
			sq_io_error("proclaimnl");

		sq_text_free(text);
		set_next_local(sf, SQ_NI);
		return;

	// [A,DST] Print `A` with a newline, DST <- ni
	case SQ_INT_PRINTLN:
		text = sq_value_to_text(operands[0]);
		if (!puts(text->ptr))
			sq_io_error("proclaim");

		sq_text_free(text);
		set_next_local(sf, SQ_NI);
		return;

	// [A,DST] Dumps out `A`, DST <- A
	case SQ_INT_DUMP:
		sq_value_dump(operands[0]);
		putchar('\n');
		set_next_local(sf, sq_value_clone(operands[0]));
		return;

	// [DST] DST <- next line from stdin
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

		set_next_local(sf, sq_value_new(sq_text_new(line)));
		break;
	}


	// [CMD,DST] DST <- stdout of running `cmd`.
	case SQ_INT_SYSTEM: {
		text = sq_value_to_text(operands[0]);
		char *str = text->ptr;
		FILE *stream = popen(str, "r");

		if (stream == NULL)
			sq_io_error("opening `hex` stream");

		sq_text_free(text);

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
		if (ferror(stream)) sq_io_error("reading `hex` result stream");

		result = xrealloc(result, length + 1);
		result[length] = '\0';

		// Abort if we cant close stream.
		if (pclose(stream) == -1)
			sq_io_error("closing `hex` output stream");

		set_next_local(sf, sq_value_new(sq_text_new(result)));
		return;
	}

	// [CODE] Exits with the given code.
	case SQ_INT_EXIT:
		exit(sq_value_to_numeral(operands[0]));

	// [DST] DST <- random numeral
	case SQ_INT_RANDOM:
		// TODO: better random numbers
		set_next_local(sf, sq_value_new((sq_numeral) rand()));
		return;


	// [A,B,C,DST] DST <- A[B..B+C]
	case SQ_INT_SUBSTR: {
		text = sq_value_to_text(operands[0]);
		sq_numeral start = sq_value_to_numeral(operands[1]);
		if (!start--)
			sq_throw("cannot index by N.");
		sq_numeral count = sq_value_to_numeral(operands[2]);
		struct sq_text *result;

		if (!*text->ptr) 
			result = sq_text_new2(strdup(""), 0);
		else
			result = sq_text_new2(strndup(text->ptr + start, count), count);

		set_next_local(sf, sq_value_new(result));
		return;
	}

	// [A,DST] DST <- length A: book/codex/text
	case SQ_INT_LENGTH:
		set_next_local(sf, sq_value_new((sq_numeral) sq_value_length(operands[0])));


	// [N,...,DST] DST <- N key-value pairs.
	case SQ_INT_CODEX_NEW: {
		unsigned amnt = next_count(sf);
		struct sq_codex *codex = sq_codex_allocate(amnt);

		for (; codex->length < amnt; ++codex->length) {
			codex->pages[codex->length].key = sq_value_clone(*next_local(sf));
			codex->pages[codex->length].value = sq_value_clone(*next_local(sf));
		}

		set_next_local(sf, sq_value_new(codex));
		return;
	}


	// [N,...,DST] DST <- N-length array.
	case SQ_INT_BOOK_NEW: {
		unsigned amnt = next_count(sf);
		struct sq_book *book = sq_book_allocate(amnt);

		for (; book->length < amnt; ++book->length)
			book->pages[book->length] = sq_value_clone(*next_local(sf));

		set_next_local(sf, sq_value_new(book));
		return;
	}

	// [A,B,C,DST] A.insert(len=B,pos=C); (Stores in DST, though this is not intended)
	case SQ_INT_ARRAY_INSERT: {
		if (!sq_value_is_book(operands[0]))
			sq_throw("can only insert into books");

		struct sq_book *book = sq_value_as_book(operands[0]);
		unsigned index = sq_value_to_numeral(operands[1]);

		sq_book_insert2(book, index, sq_value_clone(operands[2]));
		set_next_local(sf, sq_value_clone(operands[2]));
		return;
	}

	// [A,B,DST] DST <- A.delete(B)
	case SQ_INT_ARRAY_DELETE: {
		if (sq_value_is_book(operands[0]))
			set_next_local(sf, sq_book_delete2(sq_value_as_book(operands[0]), sq_value_to_numeral(operands[1])));
		else if (sq_value_is_codex(operands[0]))
			set_next_local(sf, sq_codex_delete(sq_value_as_codex(operands[0]), operands[1]));
		else
			die("can only delete from books and codices");

		return;
	}


	// [A,DST] DST <- A.to_numeral().arabic()
	case SQ_INT_ARABIC:
		set_next_local(sf, sq_value_new(sq_numeral_to_arabic(sq_value_to_numeral(operands[0]))));
		return;

	// [A,DST] DST <- A.to_numeral().roman()
	case SQ_INT_ROMAN:
		set_next_local(sf, sq_value_new(sq_numeral_to_roman(sq_value_to_numeral(operands[0]))));
		return;
	}
}



static unsigned normal_operands(enum sq_opcode opcode) {
	switch (opcode) {
		case SQ_OC_MOV:
		case SQ_OC_JMP_TRUE:
		case SQ_OC_JMP_FALSE:
		case SQ_OC_NOT:
		case SQ_OC_NEG:
		case SQ_OC_CALL:
		case SQ_OC_GSTORE:
			return 1;

		case SQ_OC_EQL:
		case SQ_OC_NEQ:
		case SQ_OC_LTH:
		case SQ_OC_GTH:
		case SQ_OC_LEQ:
		case SQ_OC_GEQ:
		case SQ_OC_ADD:
		case SQ_OC_SUB:
		case SQ_OC_MUL:
		case SQ_OC_DIV:
		case SQ_OC_MOD:
		case SQ_OC_INDEX:
		case SQ_OC_ILOAD:
			return 2;

		case SQ_OC_INDEX_ASSIGN:
		case SQ_OC_ISTORE:
			return 3;

		default:
			return 0;
	}
}

#define MAX_OPERAND_COUNT 3 // the max amount of operands (3) is from INDEX_ASSIGN
sq_value run_stackframe(struct sq_stackframe *sf) {
	enum sq_opcode opcode;
	sq_value operands[MAX_OPERAND_COUNT];
	unsigned arity, index;

	while (sf->ip < sf->journey->codelen) {
		opcode = next_bytecode(sf).opcode;
		arity = normal_operands(opcode);

		for (unsigned i = 0; i < arity; ++i)
			operands[i] = *next_local(sf); // note we do not clone it!

		switch (opcode) {

	/*** Misc ***/

		case SQ_OC_NOOP:
			continue;

		case SQ_OC_MOV:
			set_next_local(sf, sq_value_clone(operands[0]));
			continue;

		case SQ_OC_INT:
			handle_interrupt(sf);
			continue;

		case SQ_OC_UNDEFINED:
			bug("encountered SQ_OC_UNDEFINED %s", "");

	/*** Control Flow ***/
		case SQ_OC_JMP:
			sf->ip = next_relative_index(sf);
			continue;

		case SQ_OC_JMP_TRUE:
		case SQ_OC_JMP_FALSE:
			index = next_relative_index(sf);

			if (sq_value_to_veracity(operands[0]) == (opcode == SQ_OC_JMP_TRUE))
				sf->ip = index;

			continue;

		case SQ_OC_COMEFROM: {
			int i;
			int amnt = (int) next_index(sf);
			for (i = 0; i < amnt - 1; ++i)
				if (!fork()) break;
			sf->ip += i;
			continue;
			// todo: this should probably be fixed.
		}

		case SQ_OC_CALL: {
			unsigned pargc = next_count(sf);
			sq_value pargv[pargc];
			struct sq_args args = { .pargc = pargc, .pargv = pargv };

			for (unsigned i = 0; i < pargc; ++i)
				args.pargv[i] = sq_value_clone(*next_local(sf));

			set_next_local(sf, sq_value_call(operands[0], args));
			continue;
		}

		case SQ_OC_RETURN:
			return sq_value_clone(operands[0]);

		case SQ_OC_THROW:
			// TODO: catch thrown values and free memory in the current function.
			sq_throw_value(sq_value_clone(operands[0]));

		case SQ_OC_POPTRYCATCH:
			sq_exception_pop();
			continue;

		case SQ_OC_TRYCATCH: {
			// todo: maybe have this be within the `stackframe`?
			unsigned catch_index = next_index(sf);
			unsigned exception_index = next_index(sf);

			if (!setjmp(exception_handlers[current_exception_handler++]))
				continue;

			sf->locals[exception_index] = exception;
			exception = SQ_NI;
			sf->ip = catch_index;
			continue;
		}

	/** Logic **/
		case SQ_OC_NOT:
			set_next_local(sf, sq_value_new(sq_value_not(operands[0])));
			continue;

		case SQ_OC_EQL:
			set_next_local(sf, sq_value_new(sq_value_eql(operands[0], operands[1])));
			continue;

		case SQ_OC_NEQ:
			set_next_local(sf, sq_value_new(sq_value_neq(operands[0], operands[1])));
			continue;

		case SQ_OC_LTH:
			set_next_local(sf, sq_value_new(sq_value_lth(operands[0], operands[1])));
			continue;

		case SQ_OC_GTH:
			set_next_local(sf, sq_value_new(sq_value_gth(operands[0], operands[1])));
			continue;

		case SQ_OC_LEQ:
			set_next_local(sf, sq_value_new(sq_value_leq(operands[0], operands[1])));
			continue;

		case SQ_OC_GEQ:
			set_next_local(sf, sq_value_new(sq_value_geq(operands[0], operands[1])));
			continue;

	/** Math **/
		case SQ_OC_NEG:
			set_next_local(sf, sq_value_neg(operands[0]));
			continue;

		case SQ_OC_ADD:
			set_next_local(sf, sq_value_add(operands[0], operands[1]));
			continue;

		case SQ_OC_SUB:
			set_next_local(sf, sq_value_sub(operands[0], operands[1]));
			continue;

		case SQ_OC_MUL:
			set_next_local(sf, sq_value_mul(operands[0], operands[1]));
			continue;

		case SQ_OC_DIV:
			set_next_local(sf, sq_value_div(operands[0], operands[1]));
			continue;

		case SQ_OC_MOD:
			set_next_local(sf, sq_value_mod(operands[0], operands[1]));
			continue;

		case SQ_OC_INDEX:
			set_next_local(sf, sq_value_index(operands[0], operands[1]));
			continue;

		case SQ_OC_INDEX_ASSIGN:
			sq_value_index_assign(operands[0], sq_value_clone(operands[1]), sq_value_clone(operands[2]));
			continue;

	/*** Interpreter Stuff ***/
		case SQ_OC_CLOAD:
			index = next_index(sf);
			assert(index <= sf->journey->nconsts);

			set_next_local(sf, sq_value_clone(sf->journey->consts[index]));
			continue;

		case SQ_OC_GLOAD:
			index = next_index(sf);
			assert(index <= sf->journey->program->nglobals);

			set_next_local(sf, sq_value_clone(sf->journey->program->globals[index]));
			continue;

		case SQ_OC_GSTORE:
			index = next_index(sf);
			assert(index <= sf->journey->program->nglobals);

			sq_value_free(sf->journey->program->globals[index]);
			sf->journey->program->globals[index] = sq_value_clone(operands[0]);
			continue;

		case SQ_OC_ILOAD:
			assert(sq_value_is_text(operands[1]));
			set_next_local(sf, sq_value_get_attr(operands[0], sq_value_as_text(operands[1])->ptr));
			continue;

		case SQ_OC_ISTORE:
			assert(sq_value_is_text(operands[1]));
			sq_value_set_attr(operands[0], sq_value_as_text(operands[1])->ptr, operands[2]);
			continue;

		default:
			todo("!");
		}
	}

	return SQ_NI;
}

#define SET_NEXT_LOCAL() NEXT_LOCAL() = value

#if 0
sq_value sq_journey_run_deprecated(const struct sq_journey *function, unsigned argc, sq_value *args) {
	sq_value locals[function->nlocals];
	sq_value value = SQ_NI;
	enum sq_opcode opcode;


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
				sq_value *pages = xmalloc(sizeof_array(sq_value , amnt));

				for (unsigned i = 0; i < amnt; ++i)
					pages[i] = NEXT_LOCAL();

				SET_NEXT_LOCAL() = sq_value_new_book(sq_book_new2(amnt, pages));
				break;
			}

			case SQ_INT_CODEX_NEW: {
				unsigned amnt = NEXT_INDEX();
				struct sq_codex_page *pages = xmalloc(sizeof_array(struct sq_codex_page , amnt));

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

				SET_NEXT_LOCAL() = sq_journey_run_deprecated(fn, argc, newargs);
			} else if (sq_value_is_form(imitation_value)) {
				struct sq_form *form = sq_value_as_form(imitation_value);
				struct sq_args args = { .pargc = argc, .pargv = newargs };
				SET_NEXT_LOCAL() = sq_value_new(sq_form_imitate(form, args));
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
#endif
