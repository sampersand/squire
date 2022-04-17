#include <squire/other/other.h>
#include <squire/journey.h>
#include <squire/text.h>
#include <squire/program.h>
#include <squire/shared.h>
#include <squire/form.h>
#include <squire/book.h>
#include <squire/codex.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

static void deallocate_pattern(struct sq_journey_pattern *pattern) {
	for (unsigned i = 0; i < pattern->pargc; ++i)
		free(pattern->pargv[i].name);

	for (unsigned i = 0; i < pattern->kwargc; ++i)
		free(pattern->kwargv[i].name);

	for (unsigned i = 0; i < pattern->code.nconsts; ++i)
		sq_value_free(pattern->code.consts[i]);

	free(pattern->pargv);
	free(pattern->kwargv);
	free(pattern->code.consts);
	free(pattern->code.bytecode);
}

void sq_journey_deallocate(struct sq_journey *journey) {
	assert(!journey->refcount);

	for (unsigned i = 0; i < journey->npatterns; ++i)
		deallocate_pattern(&journey->patterns[i]);

	free(journey->name);
	free(journey->patterns);
	free(journey);
}

void sq_journey_dump(FILE *out, const struct sq_journey *journey) {
	fprintf(out, "Journey(%s, %d patterns)", journey->name, journey->npatterns);
}

struct sq_stackframe {
	unsigned ip;
	const struct sq_journey *journey;
	const struct sq_journey_pattern *pattern;
	sq_value *locals;
};

static sq_value run_stackframe(struct sq_stackframe *stackframe);

static int assign_positional_arguments(
	struct sq_stackframe *sf,
	const struct sq_journey_pattern *pattern,
	struct sq_args *args
) {
	unsigned i = 0;
	struct sq_book *splat = NULL;
	// first, assign all positional arguments that we can.
	for (i = 0; i < args->pargc && i < pattern->pargc; ++i)
		sf->locals[i] = sq_value_clone(args->pargv[i]);

	if (pattern->pargc == args->pargc) {
		// do nothing, all argument counts worked out.
	} else if (i == pattern->pargc) {
		// if we have extra arguments, then either stick them into splat or return error.
		if (!pattern->splat) 
			return -1; // too many arguments given, and no splat provided.

		splat = sq_book_allocate(args->pargc - i);

		for (unsigned j = i; j < args->pargc; ++j)
			splat->pages[splat->length++] = sq_value_clone(args->pargv[j]);
	} else {
		// we have fewer arguments than total argument count, so either fill out defaults, or return -1.

		// if the first argument we didn't supply doesn't have a default, we don tmatch
		if (pattern->pargv[i].default_start < 0)
			return -1;

		for (; i < pattern->pargc; ++i) {
			assert(0 <= pattern->pargv[i].default_start);

			sf->ip = pattern->pargv[i].default_start;
			sf->locals[i] = run_stackframe(sf);
		}
	}

	// make sure all the non-splat parameters match
	assert(i == pattern->pargc);

	for (unsigned j = 0; j < i; ++j) {
		if (pattern->pargv[j].genus_start < 0)
			continue;

		sf->ip = pattern->pargv[j].genus_start;
		sq_value genus = run_stackframe(sf);

		bool matches = sq_value_matches(genus, sf->locals[j]);
		sq_value_free(genus);
		if (!matches)
			return -1;
	}

	if (pattern->splat && splat == NULL)
		splat = sq_book_allocate(0);

	if (splat != NULL)	
		sf->locals[i++] = sq_value_new(splat);

	// todo, check for genuses.

	return i;
}

static sq_value try_run_pattern(
	const struct sq_journey *journey,
	const struct sq_journey_pattern *pattern,
	struct sq_args *args
) {
	struct sq_stackframe sf = {
		.journey = journey,
		.pattern = pattern,
		.locals = calloc(sizeof(sq_value), pattern->code.nlocals)
	};

	sq_value result = SQ_UNDEFINED;

	int positional_argument_stop_index = assign_positional_arguments(&sf, pattern, args);

	if (positional_argument_stop_index < 0)
		goto free_and_return;

	// todo: handle keyword arguments

	// ie we have a condition
	if (0 <= pattern->condition_start) {
		sf.ip = pattern->condition_start;
		sq_value condition = run_stackframe(&sf);
		bool is_valid = sq_value_to_numeral(condition);
		sq_value_free(condition);
		if (!is_valid) goto free_and_return;
	}

	sf.ip = pattern->start_index;
	result = run_stackframe(&sf);

free_and_return:

	for (unsigned i = 0; i < pattern->code.nlocals; ++i)
		sq_value_free(sf.locals[i]);
	free(sf.locals);

	return result;
}

sq_value sq_journey_run(const struct sq_journey *journey, struct sq_args args) {
	sq_value result;

	for (unsigned i = 0; i < journey->npatterns; ++i)
		if ((result = try_run_pattern(journey, &journey->patterns[i], &args)) != SQ_UNDEFINED)
			return result;

	// whelp, no pattern matched. exception time!
	sq_throw("no patterns match for '%s'", journey->name);
}

// static void setup_stackframe(struct sq_stackframe *stackframe, struct sq_args args) {
// 	unsigned index = 0;
// 	for (unsigned i = 0; i < stackframe->pattern->pargc; ++i) {
// 		if (i < args.pargc)
// 			stackframe->locals[index++] = args.pargv[i];
// 		else
// 			stackframe->locals[]
// 	}
// 	if (stackframe->pattern->code.argc != args.pargc)
// 		sq_throw("argument mismatch: expected %d, given %d", stackframe->pattern->argc, args.pargc);

// 	unsigned i = 0;

// 	for (; i < args.pargc; ++i)
// 		stackframe->locals[i] = args.pargv[i];

// 	for (; i < stackframe->pattern->nlocals; ++i)
// 		stackframe->locals[i] = SQ_NI;
// }

#ifndef SQ_NMOON_JOKE

#include <time.h>
extern double moon_phase2(int year,int month,int day, double hour);

bool sq_moon_joke_does_were_flip() {
	time_t t = time(NULL);
	struct tm *time = localtime(&t);

	double mf = moon_phase2(
		time->tm_year + 1900,
		time->tm_mon + 1,
		time->tm_mday,
		time->tm_hour
	);

	return (0.80 - mf) < 0 && !(rand() % 100);
}
#endif /* !SQ_NMOON_JOKE */

static inline union sq_bytecode next_bytecode(struct sq_stackframe *sf) {
	return sf->pattern->code.bytecode[sf->ip++];
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
	assert(index <= sf->pattern->code.nlocals);

	sq_value_free(sf->locals[index]);
	sf->locals[index] = value;
}

static void set_next_local(struct sq_stackframe *sf, sq_value value) {
	set_local(sf, next_index(sf), value);
}

#define MAX_INTERRUPT_OPERAND_COUNT 3
static unsigned interrupt_operands(enum sq_interrupt interrupt) {
	switch (interrupt) {
	case SQ_INT_TONUMERAL:
	case SQ_INT_TOTEXT:
	case SQ_INT_TOVERACITY:
	case SQ_INT_TOBOOK:
	case SQ_INT_TOCODEX:
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

	// temporary hacks until we get kingdoms working.
	case SQ_INT_FOPEN: return 2;
	case SQ_INT_FCLOSE: return 1;
	case SQ_INT_FREAD: return 2;
	case SQ_INT_FREADALL: return 1;
	case SQ_INT_FWRITE: return 2;
	case SQ_INT_FTELL: return 1;
	case SQ_INT_FSEEK: return 3;

	// ASCII
	case SQ_INT_ASCII: return 1;
	case SQ_INT_UNDEFINED: bug("undefined encountered");
	}
}

static void handle_interrupt(struct sq_stackframe *sf) {
	enum sq_interrupt interrupt = next_bytecode(sf).interrupt;
	sq_value operands[MAX_INTERRUPT_OPERAND_COUNT];
	struct sq_text *text;
	struct sq_other *other;

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

	// [A,DST] DST <- A.to_book()
	case SQ_INT_TOBOOK:
		set_next_local(sf, sq_value_new(sq_value_to_book(operands[0])));
		return;

	// [A,DST] DST <- A.to_codex()
	case SQ_INT_TOCODEX:
		set_next_local(sf, sq_value_new(sq_value_to_codex(operands[0])));
		return;

	// [A,DST] DST <- A.genus
	case SQ_INT_KINDOF:
		set_next_local(sf, sq_value_clone(sq_value_genus(operands[0])));
		return;

	// [A,DST] Print `A`, DST <- ni
	case SQ_INT_PRINT:
		text = sq_value_to_text(operands[0]);
		if (!fputs(text->ptr, stdout))
			sq_throw_io("proclaimnl");
		fflush(stdout);

		sq_text_free(text);
		set_next_local(sf, SQ_NI);
		return;

	// [A,DST] Print `A` with a newline, DST <- ni
	case SQ_INT_PRINTLN:
		text = sq_value_to_text(operands[0]);
		if (!puts(text->ptr))
			sq_throw_io("proclaim");
		fflush(stdout);

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
			sq_throw_io("opening `hex` stream");

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
		if (ferror(stream)) sq_throw_io("reading `hex` result stream");

		result = xrealloc(result, length + 1);
		result[length] = '\0';

		// Abort if we cant close stream.
		if (pclose(stream) == -1)
			sq_throw_io("closing `hex` output stream");

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

		if (!*text->ptr || start >= text->length) 
			result = sq_text_new2(strdup(""), 0);
		else if (start + count < text->length)
			result = sq_text_new2(strndup(text->ptr + start, count), count);
		else
			result = sq_text_new(strdup(text->ptr + start)); // todo: we know the length

		set_next_local(sf, sq_value_new(result));
		return;
	}

	// [A,DST] DST <- length A: book/codex/text
	case SQ_INT_LENGTH:
		set_next_local(sf, sq_value_new((sq_numeral) sq_value_length(operands[0])));
		return;


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

	// temporary hacks until we get kingdoms working.
	case SQ_INT_FOPEN: {
		other = xmalloc(sizeof(struct sq_other));
		other->refcount = 1;
		other->kind = SQ_OK_SCROLL;
		struct sq_text *filename = sq_value_to_text(operands[0]);
		struct sq_text *mode = sq_value_to_text(operands[1]);
		sq_scroll_init(&other->scroll, filename->ptr, mode->ptr);
		sq_text_free(filename);
		sq_text_free(mode);

		set_next_local(sf, sq_value_new(other));
		return;
	}

	case SQ_INT_FCLOSE:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only close scrolls, not '%s'", sq_value_typename(operands[0]));
		sq_scroll_close(sq_other_as_scroll(other));
		set_next_local(sf, SQ_NI);
		return;

	case SQ_INT_FREAD:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only read scrolls, not '%s'", sq_value_typename(operands[0]));

		set_next_local(sf, sq_value_new(sq_scroll_read(sq_other_as_scroll(other), sq_value_to_numeral(operands[1]))));
		return;

	case SQ_INT_FREADALL:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only read scrolls, not '%s'", sq_value_typename(operands[0]));

		set_next_local(sf, sq_value_new(sq_scroll_read_all(sq_other_as_scroll(other))));
		return;

	case SQ_INT_FWRITE:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only write scrolls, not '%s'", sq_value_typename(operands[0]));

		text = sq_value_to_text(operands[1]);
		sq_scroll_write(sq_other_as_scroll(other), text->ptr, text->length);
		sq_text_free(text);

		set_next_local(sf, SQ_NI);
		return;

	case SQ_INT_FTELL:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only tell scrolls, not '%s'", sq_value_typename(operands[0]));

		set_next_local(sf, sq_value_new((sq_numeral) sq_scroll_tell(sq_other_as_scroll(other))));
		return;

	case SQ_INT_FSEEK:
		if (!sq_value_is_other(operands[0]) || (other = sq_value_as_other(operands[0]))->kind != SQ_OK_SCROLL)
			sq_throw("can only seek scrolls, not '%s'", sq_value_typename(operands[0]));

		sq_scroll_seek(sq_other_as_scroll(other), sq_value_to_numeral(operands[1]), sq_value_to_numeral(operands[2]));
		set_next_local(sf, SQ_NI);
		return;

	case SQ_INT_ASCII:
		if (sq_value_is_numeral(operands[0])) {
			char *data = xmalloc(2);
			data[0] = sq_value_as_numeral(operands[0]) & 0xff;
			data[1] = '\0';
			set_next_local(sf, sq_value_new(sq_text_new(data)));
		} else if (sq_value_is_text(operands[0])) {
			set_next_local(sf, sq_value_new((sq_numeral) sq_value_as_text(operands[0])->ptr[0]));
		} else {
			sq_throw("can only ascii numerals and text, not '%s'", sq_value_typename(operands[0]));
		}

		return;

	case SQ_INT_UNDEFINED:
		bug("undefined encountered");
	}
}

static unsigned normal_operands(enum sq_opcode opcode) {
	switch (opcode) {
		case SQ_OC_MOV:
		case SQ_OC_JMP_TRUE:
		case SQ_OC_JMP_FALSE:
#ifndef SQ_NMOON_JOKE
		case SQ_OC_WERE_JMP:
#endif /* SQ_NMOON_JOKE */
		case SQ_OC_NOT:
		case SQ_OC_NEG:
		case SQ_OC_CALL:
		case SQ_OC_GSTORE:
		case SQ_OC_ILOAD:
		case SQ_OC_RETURN:
		case SQ_OC_THROW:
			return 1;

		case SQ_OC_EQL:
		case SQ_OC_NEQ:
		case SQ_OC_LTH:
		case SQ_OC_GTH:
		case SQ_OC_LEQ:
		case SQ_OC_GEQ:
		case SQ_OC_CMP:
		case SQ_OC_ADD:
		case SQ_OC_SUB:
		case SQ_OC_MUL:
		case SQ_OC_DIV:
		case SQ_OC_MOD:
		case SQ_OC_POW:
		case SQ_OC_MATCHES:
		case SQ_OC_INDEX:
		case SQ_OC_ISTORE:
		case SQ_OC_FEGENUS_STORE:
		case SQ_OC_FMGENUS_STORE:
			return 2;

		case SQ_OC_INDEX_ASSIGN:
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
	const struct sq_codeblock *code = &sf->pattern->code;

	while (sf->ip < code->codelen) {
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
			sf->ip = next_index(sf);
			continue;

		case SQ_OC_JMP_FALSE:
		case SQ_OC_JMP_TRUE:
#ifndef SQ_NMOON_JOKE
		case SQ_OC_WERE_JMP:
#endif /* SQ_NMOON_JOKE */
			index = next_index(sf);
			bool should_jump =
				sq_value_to_veracity(operands[0]) == (opcode == SQ_OC_JMP_TRUE);

#ifndef SQ_NMOON_JOKE
			if (opcode == SQ_OC_WERE_JMP && sq_moon_joke_does_were_flip())
				should_jump = !should_jump;
#endif /* SQ_NMOON_JOKE */

			if (should_jump)
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
			// TODO: catch thrown values and free memory in the current journey.
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

		case SQ_OC_CMP:
			set_next_local(sf, sq_value_new(sq_value_cmp(operands[0], operands[1])));
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

		case SQ_OC_POW:
			set_next_local(sf, sq_value_pow(operands[0], operands[1]));
			continue;

		case SQ_OC_INDEX:
			set_next_local(sf, sq_value_index(operands[0], operands[1]));
			continue;

		case SQ_OC_INDEX_ASSIGN:
			sq_value_index_assign(operands[0], sq_value_clone(operands[1]), sq_value_clone(operands[2]));
			continue;

		case SQ_OC_MATCHES:
			set_next_local(sf, sq_value_new(sq_value_matches(operands[0], operands[1])));
			continue;

	/*** Interpreter Stuff ***/
		case SQ_OC_CLOAD:
			index = next_index(sf);
			assert(index < code->nconsts);

			set_next_local(sf, sq_value_clone(code->consts[index]));
			continue;

		case SQ_OC_GLOAD:
			index = next_index(sf);
			assert(index < sf->journey->program->nglobals);

			set_next_local(sf, sq_value_clone(sf->journey->program->globals[index]));
			continue;

		case SQ_OC_GSTORE:
			index = next_index(sf);
			assert(index < sf->journey->program->nglobals);

			sq_value_free(sf->journey->program->globals[index]);
			sf->journey->program->globals[index] = sq_value_clone(operands[0]);
			continue;

		case SQ_OC_ILOAD:
			index = next_index(sf);
			assert(index < code->nconsts);
			assert(sq_value_is_text(operands[1] = code->consts[index]));

			set_next_local(sf, sq_value_get_attr(operands[0], sq_value_as_text(operands[1])->ptr));
			continue;

		case SQ_OC_ISTORE:
			index = next_index(sf);

			assert(index < code->nconsts);
			assert(sq_value_is_text(operands[2] = code->consts[index]));

			sq_value_set_attr(operands[0], sq_value_as_text(operands[2])->ptr, operands[1]);
			continue;

		case SQ_OC_FEGENUS_STORE: {
			index = next_index(sf);

			assert(sq_value_is_form(operands[0]));
			assert(index < sq_value_as_form(operands[0])->nessences);
			assert(sq_value_as_form(operands[0])->essences[index].genus == SQ_UNDEFINED);
			sq_value_as_form(operands[0])->essences[index].genus = sq_value_clone(operands[1]);
			continue;
		}

		case SQ_OC_FMGENUS_STORE:
			index = next_index(sf);

			assert(sq_value_is_form(operands[0]));
			assert(index < sq_value_as_form(operands[0])->nmatter);
			assert(sq_value_as_form(operands[0])->matter[index].genus == SQ_UNDEFINED);
			sq_value_as_form(operands[0])->matter[index].genus = sq_value_clone(operands[1]);
			continue;
		}

		bug("unknown opcode: %d", opcode);
	}

	return SQ_NI;
}
