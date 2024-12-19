#include <squire/other/other.h>
#include <squire/journey.h>
#include <squire/text.h>
#include <squire/program.h>
#include <squire/parse.h>
#include <squire/shared.h>
#include <squire/form.h>
#include <squire/book.h>
#include <squire/codex.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#define SQ_USE_COMPUTED_GOTOS // todo: make this programatically enabled
#ifdef SQ_USE_COMPUTED_GOTOS
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wgnu-label-as-value"
# endif /* defined(__clang__) */
# define VM_SWITCH(oc, labels) goto *(labels)[oc];
# define VM_CASE_NAME(oc) vm_case_##oc
# define VM_CASE(oc) SQ_UNREACHABLE /* no fallthroughs */; VM_CASE_FT(oc)
# define VM_CASE_FT(oc) VM_CASE_NAME(oc):
# define VM_SWITCH_END
# define VM_DEFAULT if (0) 
#else
# define VM_SWITCH(oc, _labels) switch (oc) {
# define VM_CASE(oc) SQ_UNREACHABLE /* no fallthroughs */; VM_CASE_FT(oc)
# define VM_CASE_FT(oc) case oc:
# define VM_SWITCH_END }
# define VM_DEFAULT default:
#endif /* defined(SQ_USE_COMPUTED_GOTOS) */

static void deallocate_pattern(struct sq_journey_pattern *pattern) {
	for (unsigned i = 0; i < pattern->pargc; ++i)
		free(pattern->pargv[i].name);

	for (unsigned i = 0; i < pattern->kwargc; ++i)
		free(pattern->kwargv[i].name);

	free(pattern->pargv);
	free(pattern->kwargv);
	free(pattern->code.consts);
	free(pattern->code.bytecode);
}

void sq_stackframe_mark(struct sq_stackframe *stackframe) {
#ifndef NDEBUG
	for (unsigned i = 0; i < stackframe->journey->npatterns; ++i)
		if (&stackframe->journey->patterns[i] == stackframe->pattern)
			goto found;
	SQ_UNREACHABLE;
found:
#endif
	
	sq_journey_mark((struct sq_journey *)  stackframe->journey); // lol we just removed constness..
	for (unsigned i = 0; i < stackframe->pattern->code.nlocals;	++i)
		sq_value_mark(stackframe->locals[i]);
}

void sq_journey_mark(struct sq_journey *journey) {
	SQ_GUARD_MARK(journey);

	for (unsigned i = 0; i < journey->npatterns; ++i) {
		struct sq_journey_pattern pattern = journey->patterns[i];

		for (unsigned j = 0; j < pattern.code.nconsts; ++j)
			sq_value_mark(pattern.code.consts[j]);
	}
}

void sq_journey_deallocate(struct sq_journey *journey) {
	for (unsigned i = 0; i < journey->npatterns; ++i)
		deallocate_pattern(&journey->patterns[i]);

	free(journey->name);
	free(journey->patterns);
}

void sq_journey_dump(FILE *out, const struct sq_journey *journey) {
	fprintf(out, "Journey(%s, %d patterns)", journey->name, journey->npatterns);
}

static sq_value sq_run_stackframe(struct sq_stackframe *stackframe);

static int assign_positional_arguments(
	struct sq_stackframe *sf,
	const struct sq_journey_pattern *pattern,
	struct sq_args *args
) {
	unsigned i = 0;
	struct sq_book *splat = NULL;
	// first, assign all positional arguments that we can.
	for (i = 0; i < args->pargc && i < pattern->pargc; ++i)
		sf->locals[i] = args->pargv[i];

	if (pattern->pargc == args->pargc) {
		// do nothing, all argument counts worked out.
	} else if (i == pattern->pargc) {
		// if we have extra arguments, then either stick them into splat or return error.
		if (!pattern->splat) 
			return -1; // too many arguments given, and no splat provided.

		splat = sq_book_allocate(args->pargc - i);

		for (unsigned j = i; j < args->pargc; ++j)
			splat->pages[splat->length++] = args->pargv[j];
	} else {
		// we have fewer arguments than total argument count, so either fill out defaults, or return -1.

		// if the first argument we didn't supply doesn't have a default, we don tmatch
		if (pattern->pargv[i].default_start < 0)
			return -1;

		for (; i < pattern->pargc; ++i) {
			sq_assert_le(0, pattern->pargv[i].default_start);

			sf->ip = pattern->pargv[i].default_start;
			sf->locals[i] = sq_run_stackframe(sf);
		}
	}

	// make sure all the non-splat parameters match
	sq_assert_eq(i, pattern->pargc);

	for (unsigned j = 0; j < i; ++j) {
		if (pattern->pargv[j].genus_start < 0)
			continue;

		sf->ip = pattern->pargv[j].genus_start;
		sq_value genus = sq_run_stackframe(sf);

		bool matches = sq_value_matches(genus, sf->locals[j]);
		if (!matches)
			return -1;
	}

	if (pattern->splat && splat == NULL)
		splat = sq_book_allocate(0);

	if (splat != NULL)	
		sf->locals[i++] = sq_value_new_book(splat);

	// todo, check for genuses.

	return i;
}


struct sq_stackframe sq_stackframes[SQ_MAX_STACKFRAME_COUNT];
unsigned sq_current_stackframe;

static sq_value try_run_pattern(
	const struct sq_journey *journey,
	const struct sq_journey_pattern *pattern,
	struct sq_args *args
) {
	if (sq_current_stackframe == SQ_MAX_STACKFRAME_COUNT)
		sq_throw("too many stackframes encountered");

	struct sq_stackframe *sf = &sq_stackframes[sq_current_stackframe++];
	*sf = (struct sq_stackframe) {
		.journey = journey,
		.pattern = pattern,
		.locals = calloc(sizeof(sq_value), pattern->code.nlocals)
	};

	sq_value result = SQ_UNDEFINED;

	int positional_argument_stop_index = assign_positional_arguments(sf, pattern, args);

	if (positional_argument_stop_index < 0)
		goto free_and_return;

	// todo: handle keyword arguments

	// ie we have a condition
	if (0 <= pattern->condition_start) {
		sf->ip = pattern->condition_start;
		sq_value condition = sq_run_stackframe(sf);
		bool is_valid = sq_value_to_numeral(condition);
		if (!is_valid) goto free_and_return;
	}

	sf->ip = pattern->start_index;
	result = sq_run_stackframe(sf);

free_and_return:
	free(sf->locals);
	--sq_current_stackframe;
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

static bool sq_moon_joke_does_were_flip() {
	extern double moon_phase2(int year,int month,int day, double hour);
	static time_t last_check; // cache it so we don't always recalculate
	static bool should_flip;
	const int HOUR_IN_SECONDS = 3600;

	time_t t = time(NULL);

	if (last_check <= t + HOUR_IN_SECONDS)  {
		struct tm *time = localtime(&t);

		double mf = moon_phase2(
			time->tm_year + 1900,
			time->tm_mon + 1,
			time->tm_mday,
			time->tm_hour
		);
		should_flip = 0.95 <= mf;
	}

	return should_flip && !(rand() % 100);
}
#endif /* !SQ_NMOON_JOKE */

static inline union sq_bytecode next_bytecode(struct sq_stackframe *sf) {
	union sq_bytecode bc = sf->pattern->code.bytecode[sf->ip++];
	sq_log_old("runtime[%d]=%u\n", sf->ip-1, bc.index);
	return bc;
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
	sq_assert_le(index, sf->pattern->code.nlocals);

	sf->locals[index] = value;
}

static void set_next_local(struct sq_stackframe *sf, sq_value value) {
	set_local(sf, next_index(sf), value);
}

// todo: does this exist already?
static int write_all(int fd, const void *buf, size_t nbytes) {
	ssize_t tmp = 0;
	while ((tmp = write(fd, buf, nbytes))) {
		if (tmp < 0) return -1;
		nbytes -= tmp;
	} 
	return 0;
}
/*
static char *read_all(int fd) {
	size_t buf_len = 0, buf_cap = 1024;
	char *buf = sq_malloc_vec(char, buf_cap), *curr = buf;

	ssize_t tmp = 0;
	while ((tmp = write(fd, buf, nbytes))) {
		if (tmp < 0) return free(buf), 0;
		nbytes -= tmp;
	} 
	return 0;
}
struct sq_text *do_babel2(
	const struct sq_text *executable,
	const struct sq_text *executable_stdin,
	unsigned nargs,
	const struct sq_text **args
) {
	int in_fds[2], out_fds[2], status;
	pid_t child_pid;
	char *c_args[nargs + 2];
	c_args[0] = sq_text_to_c_str(executable);
	for (unsigned i = 0; i < nargs; ++i)
		c_args[i + 1] = sq_text_to_c_str(args[i]);
	c_args[nargs + 1] = 0;


	// TODO: check these for errors.
	pipe(in_fds);
	pipe(out_fds);
	write_all(in_fds[1], executable_stdin->ptr, executable_stdin->length);
	close(in_fds[1]);

	if (!(child_pid = fork())) {
		dup2(in_fds[0], STDIN_FILENO);
		dup2(out_fds[1], STDOUT_FILENO);
		dup2(out_fds[1], STDERR_FILENO);
		execvp(c_args[0], c_args);
		perror("cant exec");
		_Exit(1);
	}

	char *result = read_all(out_fds[0]);
	close(out_fds[0]);
	waitpid(child_pid, &status, 0);

	for (unsigned i = 0; i <= nargs; ++i) free(c_args[i]);
	return sq_text_new(result);
}*/

static struct sq_text *do_babel(
	const struct sq_text *executable,
	const struct sq_text *executable_stdin,
	unsigned nargs,
	const struct sq_text **args
) {
	int in_fds[2], out_fds[2], status;
	pid_t child_pid;

	if (pipe(in_fds)) sq_throw_io("unable to create pipes");
	if (pipe(out_fds)) {
		sq_throw_io("unable to create pipes");
	}
	if (write_all(in_fds[1], executable_stdin->ptr, executable_stdin->length)) {
		sq_throw_io("unable to write stdin");
	}
	close(in_fds[1]);

	SQ_ALLOCA(char *, c_args, nargs + 2);
	c_args[0] = sq_text_to_c_str(executable);	
	for (unsigned i = 0; i < nargs; ++i)
		c_args[i + 1] = sq_text_to_c_str(args[i]);
	c_args[nargs + 1] = 0;

	if (!(child_pid = fork())) {
		dup2(in_fds[0], STDIN_FILENO);
		dup2(out_fds[1], STDOUT_FILENO);
		dup2(out_fds[1], STDERR_FILENO);
		execvp(c_args[0], c_args);
		perror("cant exec");
		_Exit(1);
	}

	char *result = malloc(1000); // todo: read more than 1000
	read(out_fds[0], result, 1000);
	close(out_fds[0]);
	waitpid(child_pid, &status, 0);

	for (unsigned i = 0; i <= nargs; ++i)
		free(c_args[i]);

	SQ_ALLOCA_FREE(c_args);
	return sq_text_new(result);
}

static void handle_interrupt(struct sq_stackframe *sf) {
#ifdef SQ_USE_COMPUTED_GOTOS
	static const void *labels[] = {
		[SQ_INT_TONUMERAL] = &&VM_CASE_NAME(SQ_INT_TONUMERAL),
		[SQ_INT_TOTEXT] = &&VM_CASE_NAME(SQ_INT_TOTEXT),
		[SQ_INT_TOVERACITY] = &&VM_CASE_NAME(SQ_INT_TOVERACITY),
		[SQ_INT_TOBOOK] = &&VM_CASE_NAME(SQ_INT_TOBOOK),
		[SQ_INT_TOCODEX] = &&VM_CASE_NAME(SQ_INT_TOCODEX),
		[SQ_INT_KINDOF] = &&VM_CASE_NAME(SQ_INT_KINDOF),
		[SQ_INT_PRINT] = &&VM_CASE_NAME(SQ_INT_PRINT),
		[SQ_INT_PRINTLN] = &&VM_CASE_NAME(SQ_INT_PRINTLN),
		[SQ_INT_DUMP] = &&VM_CASE_NAME(SQ_INT_DUMP),
		[SQ_INT_PROMPT] = &&VM_CASE_NAME(SQ_INT_PROMPT),
		[SQ_INT_SYSTEM] = &&VM_CASE_NAME(SQ_INT_SYSTEM),
		[SQ_INT_EXIT] = &&VM_CASE_NAME(SQ_INT_EXIT),
		[SQ_INT_RANDOM] = &&VM_CASE_NAME(SQ_INT_RANDOM),
		[SQ_INT_PTR_GET] = &&VM_CASE_NAME(SQ_INT_PTR_GET),
		[SQ_INT_PTR_SET] = &&VM_CASE_NAME(SQ_INT_PTR_SET),
		[SQ_INT_SUBSTR] = &&VM_CASE_NAME(SQ_INT_SUBSTR),
		[SQ_INT_LENGTH] = &&VM_CASE_NAME(SQ_INT_LENGTH),
		[SQ_INT_CODEX_NEW] = &&VM_CASE_NAME(SQ_INT_CODEX_NEW),
		[SQ_INT_BOOK_NEW] = &&VM_CASE_NAME(SQ_INT_BOOK_NEW),
		[SQ_INT_ARRAY_INSERT] = &&VM_CASE_NAME(SQ_INT_ARRAY_INSERT),
		[SQ_INT_ARRAY_DELETE] = &&VM_CASE_NAME(SQ_INT_ARRAY_DELETE),
		[SQ_INT_BABEL] = &&VM_CASE_NAME(SQ_INT_BABEL),
		[SQ_INT_ARABIC] = &&VM_CASE_NAME(SQ_INT_ARABIC),
		[SQ_INT_ROMAN] = &&VM_CASE_NAME(SQ_INT_ROMAN),
		[SQ_INT_FOPEN] = &&VM_CASE_NAME(SQ_INT_FOPEN),
		[SQ_INT_ASCII] = &&VM_CASE_NAME(SQ_INT_ASCII),
# ifndef NDEBUG
		[SQ_INT_UNDEFINED] = &&VM_CASE_NAME(SQ_INT_UNDEFINED),
# endif /* !defined(NDEBUG) */
	};
#endif /* defined(SQ_USE_COMPUTED_GOTOS) */

	enum sq_interrupt interrupt = next_bytecode(sf).interrupt;
	sq_value operands[SQ_INTERRUPT_MAX_ARITY];
	struct sq_text *text;
	struct sq_other *other;
	unsigned arity = sq_interrupt_arity(interrupt);

#ifdef __clang__
# pragma clang loop unroll_count(SQ_INTERRUPT_MAX_ARITY)
#endif
	for (unsigned i = 0; i < arity; ++i)
		operands[i] = *next_local(sf);

	VM_SWITCH(interrupt, labels)
	VM_DEFAULT {
		sq_bug("unknown interrupt: %d", interrupt);
	}

#ifndef NDEBUG
	VM_CASE(SQ_INT_UNDEFINED)
		sq_bug("undefined encountered");
#endif /* !defined(NDEBUG) */

	// [A,DST] DST <- A.to_numeral()
	VM_CASE(SQ_INT_TONUMERAL)
		set_next_local(sf, sq_value_new_numeral(sq_value_to_numeral(operands[0])));
		return;

	// [A,DST] DST <- A.to_text()
	VM_CASE(SQ_INT_TOTEXT)
		set_next_local(sf, sq_value_new_text(sq_value_to_text(operands[0])));
		return;

	// [A,DST] DST <- A.to_veracity()
	VM_CASE(SQ_INT_TOVERACITY)
		set_next_local(sf, sq_value_new_veracity(sq_value_to_veracity(operands[0])));
		return;

	// [A,DST] DST <- A.to_book()
	VM_CASE(SQ_INT_TOBOOK)
		set_next_local(sf, sq_value_new_book(sq_value_to_book(operands[0])));
		return;

	// [A,DST] DST <- A.to_codex()
	VM_CASE(SQ_INT_TOCODEX)
		set_next_local(sf, sq_value_new_codex(sq_value_to_codex(operands[0])));
		return;

	// [A,DST] DST <- A.genus
	VM_CASE(SQ_INT_KINDOF)
		set_next_local(sf, sq_value_genus(operands[0]));
		return;

	// [A,DST] Print `A`, DST <- ni
	VM_CASE(SQ_INT_PRINT)
		text = sq_value_to_text(operands[0]);
		if (!fputs(text->ptr, stdout))
			sq_throw_io("proclaimnl");
		fflush(stdout);

		set_next_local(sf, SQ_NI);
		return;

	// [A,DST] Print `A` with a newline, DST <- ni
	VM_CASE(SQ_INT_PRINTLN)
		text = sq_value_to_text(operands[0]);
		if (!puts(text->ptr))
			sq_throw_io("proclaim");
		fflush(stdout);

		set_next_local(sf, SQ_NI);
		return;

	// [A,DST] Dumps out `A`, DST <- A
	VM_CASE(SQ_INT_DUMP)
		sq_value_dump(stdout, operands[0]);
		set_next_local(sf, operands[0]);
		return;

	// [DST] DST <- next line from stdin
	VM_CASE(SQ_INT_PROMPT) {
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

		set_next_local(sf, sq_value_new_text(sq_text_new(line)));
		return;
	}


	// [CMD,DST] DST <- stdout of running `cmd`.
	VM_CASE(SQ_INT_SYSTEM) {
		text = sq_value_to_text(operands[0]);
		char *str = text->ptr;
		FILE *stream = popen(str, "r");

		if (stream == NULL)
			sq_throw_io("opening `hex` stream");

		size_t tmp;
		size_t capacity = 2048;
		size_t length = 0;
		char *result = sq_malloc_heap(capacity);

		// try to read the entire stream's stdout to `result`.
		while (0 != (tmp = fread(result + length, 1, capacity - length, stream))) {
			length += tmp;

			if (length == capacity) {
				capacity *= 2;
				result = sq_realloc(result, capacity);
			}
		}

		// Abort if `stream` had an error.
		if (ferror(stream)) sq_throw_io("reading `hex` result stream");

		result = sq_realloc(result, length + 1);
		result[length] = '\0';

		// Abort if we cant close stream.
		if (pclose(stream) == -1)
			sq_throw_io("closing `hex` output stream");

		set_next_local(sf, sq_value_new_text(sq_text_new(result)));
		return;
	}

	// [CODE] Exits with the given code.
	VM_CASE(SQ_INT_EXIT)
		exit(sq_value_to_numeral(operands[0]));

	// [DST] DST <- random numeral
	VM_CASE(SQ_INT_RANDOM)
		// TODO: better random numbers
		set_next_local(sf, sq_value_new_numeral(rand()));
		return;

	// [A,DST] DST <- *A
	VM_CASE(SQ_INT_PTR_GET)
		if (!sq_value_is_other(operands[0]) || SQ_OK_CITATION != (other = sq_value_as_other(operands[0]))->kind)
			sq_throw("can only read citations");

		set_next_local(sf, *other->citation);
		return;


	// [A,B,DST] DST <- *A = B
	VM_CASE(SQ_INT_PTR_SET)
		if (!sq_value_is_other(operands[0]) || SQ_OK_CITATION != (other = sq_value_as_other(operands[0]))->kind)
			sq_throw("can only addend citations");

		set_next_local(sf, *other->citation = operands[1]);
		return;

	// [A,B,C,DST] DST <- A[B..B+C]
	VM_CASE(SQ_INT_SUBSTR) {
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

		set_next_local(sf, sq_value_new_text(result));
		return;
	}

	// [A,DST] DST <- length A: book/codex/text
	VM_CASE(SQ_INT_LENGTH)
		set_next_local(sf, sq_value_new_numeral(sq_value_length(operands[0])));
		return;

	// [N,...,DST] DST <- N key-value pairs.
	VM_CASE(SQ_INT_CODEX_NEW) {
		unsigned amnt = next_count(sf);
		struct sq_codex *codex = sq_codex_allocate(amnt);

		for (; codex->length < amnt; ++codex->length) {
			codex->pages[codex->length].key = *next_local(sf);
			codex->pages[codex->length].value = *next_local(sf);
		}

		set_next_local(sf, sq_value_new_codex(codex));
		return;
	}

	// [N,...,DST] DST <- N-length array.
	VM_CASE(SQ_INT_BOOK_NEW) {
		unsigned amnt = next_count(sf);
		struct sq_book *book = sq_book_allocate(amnt);

		for (; book->length < amnt; ++book->length)
			book->pages[book->length] = *next_local(sf);

		set_next_local(sf, sq_value_new_book(book));
		return;
	}

	// [A,B,C,DST] A.insert(len=B,pos=C); (Stores in DST, though this is not intended)
	VM_CASE(SQ_INT_ARRAY_INSERT) {
		if (!sq_value_is_book(operands[0]))
			sq_throw("can only insert into books");

		struct sq_book *book = sq_value_as_book(operands[0]);
		unsigned index = sq_value_to_numeral(operands[1]);

		sq_book_insert2(book, index, operands[2]);
		set_next_local(sf, operands[2]);
		return;
	}

	// [A,B,DST] DST <- A.delete(B)
	VM_CASE(SQ_INT_ARRAY_DELETE) {
		if (sq_value_is_book(operands[0]))
			set_next_local(sf, sq_book_delete2(sq_value_as_book(operands[0]), sq_value_to_numeral(operands[1])));
		else if (sq_value_is_codex(operands[0]))
			set_next_local(sf, sq_codex_delete(sq_value_as_codex(operands[0]), operands[1]));
		else
			sq_throw("can only delete from books and codices");

		return;
	}

	// [A,...,B,DST] DST <- babel(exec=A,stdin=B,args=...)c
	VM_CASE(SQ_INT_BABEL) {
		unsigned amnt = next_count(sf);
		struct sq_text *args[SQ_BABEL_MAX_ARGC], *executable, *stdin;

		executable = sq_value_to_text(operands[0]);
		for (unsigned i = 0; i < amnt; ++i)
			args[i] = sq_value_to_text(*next_local(sf));
		stdin = sq_value_to_text(operands[1]);

		set_next_local(sf, 
			sq_value_new_text(do_babel(executable, stdin, amnt, (const struct sq_text **) &*args))
		);
		return;
	}

	// [A,DST] DST <- A.to_numeral().arabic()
	VM_CASE(SQ_INT_ARABIC)
		set_next_local(sf, sq_value_new_text(sq_numeral_to_arabic(sq_value_to_numeral(operands[0]))));
		return;

	// [A,DST] DST <- A.to_numeral().roman()
	VM_CASE(SQ_INT_ROMAN)
		set_next_local(sf, sq_value_new_text(sq_numeral_to_roman(sq_value_to_numeral(operands[0]))));
		return;

	// temporary hacks until we get kingdoms working.
	VM_CASE(SQ_INT_FOPEN) {
		other = sq_mallocv(struct sq_other);
		other->kind = SQ_OK_SCROLL;
		struct sq_text *filename = sq_value_to_text(operands[0]);
		struct sq_text *mode = sq_value_to_text(operands[1]);
		sq_scroll_init(&other->scroll, filename->ptr, mode->ptr);

		set_next_local(sf, sq_value_new_other(other));
		return;
	}

	VM_CASE(SQ_INT_ASCII)
		if (sq_value_is_numeral(operands[0])) {
			char *data = sq_malloc_heap(2);
			data[0] = sq_value_as_numeral(operands[0]) & 0xff;
			data[1] = '\0';
			set_next_local(sf, sq_value_new_text(sq_text_new(data)));
		} else if (sq_value_is_text(operands[0])) {
			set_next_local(sf, sq_value_new_numeral(sq_value_as_text(operands[0])->ptr[0]));
		} else {
			sq_throw("can only ascii numerals and text, not '%s'", sq_value_typename(operands[0]));
		}

		return;

	VM_SWITCH_END
}

static sq_value sq_run_stackframe(struct sq_stackframe *sf) {
#ifdef SQ_USE_COMPUTED_GOTOS
	static const void *labels[] = {
# ifndef NDEBUG
		[SQ_OC_UNDEFINED] = &&VM_CASE_NAME(SQ_OC_UNDEFINED),
# endif /* !defined(NDEBUG) */
		[SQ_OC_NOOP] = &&VM_CASE_NAME(SQ_OC_NOOP),
		[SQ_OC_MOV] = &&VM_CASE_NAME(SQ_OC_MOV),
		[SQ_OC_INT] = &&VM_CASE_NAME(SQ_OC_INT),
		[SQ_OC_JMP] = &&VM_CASE_NAME(SQ_OC_JMP),
		[SQ_OC_JMP_FALSE] = &&VM_CASE_NAME(SQ_OC_JMP_FALSE),
		[SQ_OC_JMP_TRUE] = &&VM_CASE_NAME(SQ_OC_JMP_TRUE),
#ifndef SQ_NMOON_JOKE
		[SQ_OC_WERE_JMP] = &&VM_CASE_NAME(SQ_OC_WERE_JMP),
#endif
		[SQ_OC_COMEFROM] = &&VM_CASE_NAME(SQ_OC_COMEFROM),
		[SQ_OC_CALL] = &&VM_CASE_NAME(SQ_OC_CALL),
		[SQ_OC_RETURN] = &&VM_CASE_NAME(SQ_OC_RETURN),
		[SQ_OC_THROW] = &&VM_CASE_NAME(SQ_OC_THROW),
		[SQ_OC_POPTRYCATCH] = &&VM_CASE_NAME(SQ_OC_POPTRYCATCH),
		[SQ_OC_TRYCATCH] = &&VM_CASE_NAME(SQ_OC_TRYCATCH),
		[SQ_OC_CITE] = &&VM_CASE_NAME(SQ_OC_CITE),
		[SQ_OC_NOT] = &&VM_CASE_NAME(SQ_OC_NOT),
		[SQ_OC_EQL] = &&VM_CASE_NAME(SQ_OC_EQL),
		[SQ_OC_NEQ] = &&VM_CASE_NAME(SQ_OC_NEQ),
		[SQ_OC_LTH] = &&VM_CASE_NAME(SQ_OC_LTH),
		[SQ_OC_GTH] = &&VM_CASE_NAME(SQ_OC_GTH),
		[SQ_OC_LEQ] = &&VM_CASE_NAME(SQ_OC_LEQ),
		[SQ_OC_GEQ] = &&VM_CASE_NAME(SQ_OC_GEQ),
		[SQ_OC_CMP] = &&VM_CASE_NAME(SQ_OC_CMP),
		[SQ_OC_NEG] = &&VM_CASE_NAME(SQ_OC_NEG),
		[SQ_OC_ADD] = &&VM_CASE_NAME(SQ_OC_ADD),
		[SQ_OC_SUB] = &&VM_CASE_NAME(SQ_OC_SUB),
		[SQ_OC_MUL] = &&VM_CASE_NAME(SQ_OC_MUL),
		[SQ_OC_DIV] = &&VM_CASE_NAME(SQ_OC_DIV),
		[SQ_OC_MOD] = &&VM_CASE_NAME(SQ_OC_MOD),
		[SQ_OC_POW] = &&VM_CASE_NAME(SQ_OC_POW),
		[SQ_OC_INDEX] = &&VM_CASE_NAME(SQ_OC_INDEX),
		[SQ_OC_INDEX_ASSIGN] = &&VM_CASE_NAME(SQ_OC_INDEX_ASSIGN),
		[SQ_OC_MATCHES] = &&VM_CASE_NAME(SQ_OC_MATCHES),
		[SQ_OC_PAT_NOT] = &&VM_CASE_NAME(SQ_OC_PAT_NOT),
		[SQ_OC_PAT_OR] = &&VM_CASE_NAME(SQ_OC_PAT_OR),
		[SQ_OC_PAT_AND] = &&VM_CASE_NAME(SQ_OC_PAT_AND),
		[SQ_OC_CLOAD] = &&VM_CASE_NAME(SQ_OC_CLOAD),
		[SQ_OC_GLOAD] = &&VM_CASE_NAME(SQ_OC_GLOAD),
		[SQ_OC_GSTORE] = &&VM_CASE_NAME(SQ_OC_GSTORE),
		[SQ_OC_ILOAD] = &&VM_CASE_NAME(SQ_OC_ILOAD),
		[SQ_OC_ISTORE] = &&VM_CASE_NAME(SQ_OC_ISTORE),
		[SQ_OC_FEGENUS_STORE] = &&VM_CASE_NAME(SQ_OC_FEGENUS_STORE),
		[SQ_OC_FMGENUS_STORE] = &&VM_CASE_NAME(SQ_OC_FMGENUS_STORE),
	};
#endif /* defined(SQ_USE_COMPUTED_GOTOS) */

	enum sq_opcode opcode;
	sq_value operands[SQ_OPCODE_MAX_ARITY], result;
	unsigned arity, index;
	const struct sq_codeblock *code = &sf->pattern->code;


	while (sf->ip < code->codelen) {
#ifndef NDEBUG
		result = SQ_UNDEFINED;
#endif /* !defined(NDEBUG) */

		opcode = next_bytecode(sf).opcode;
		arity = sq_opcode_arity(opcode);

#ifdef __clang__
# pragma clang loop unroll_count(SQ_OPCODE_MAX_ARITY)
#endif
		for (unsigned i = 0; i < arity; ++i)
			operands[i] = *next_local(sf); // note we do not clone it!

#define SET_RESULT(val) do { result = val; goto push_result; } while(0)

		VM_SWITCH(opcode, labels)
		VM_DEFAULT {
			sq_bug("unknown opcode: %d", opcode);
		}
	/*** Misc ***/
# ifndef NDEBUG
		VM_CASE(SQ_OC_UNDEFINED)
			sq_bug("encountered SQ_OC_UNDEFINED");
# endif /* NDEBUG */

		VM_CASE(SQ_OC_NOOP)
			continue;

		VM_CASE(SQ_OC_MOV)
			SET_RESULT(operands[0]);

		VM_CASE(SQ_OC_INT)
			handle_interrupt(sf);
			continue;

	/*** Control Flow ***/
		VM_CASE(SQ_OC_JMP)
			sf->ip = next_index(sf);
			continue;

		VM_CASE(SQ_OC_JMP_FALSE)
		VM_CASE_FT(SQ_OC_JMP_TRUE)
#ifndef SQ_NMOON_JOKE
		VM_CASE_FT(SQ_OC_WERE_JMP)
#endif /* SQ_NMOON_JOKE */
			index = next_index(sf);
			bool should_jump = sq_value_to_veracity(operands[0]) == (opcode == SQ_OC_JMP_TRUE);

#ifndef SQ_NMOON_JOKE
			if (opcode == SQ_OC_WERE_JMP && sq_moon_joke_does_were_flip())
				should_jump = !should_jump;
#endif /* SQ_NMOON_JOKE */

			if (should_jump)
				sf->ip = index;

			continue;

		VM_CASE(SQ_OC_COMEFROM) {
			int amnt = next_index(sf);
			for (int i = 0; i < amnt - 1; ++i)
				if (!fork()) break;
				else next_index(sf);

			sf->ip = next_index(sf);
			continue;
		}

		VM_CASE(SQ_OC_CALL) {
			unsigned pargc = next_count(sf);
			SQ_ALLOCA(sq_value, pargv, pargc);
			struct sq_args args = { .pargc = pargc, .pargv = pargv };

			for (unsigned i = 0; i < pargc; ++i)
				args.pargv[i] = *next_local(sf);

			result = sq_value_call(operands[0], args);
			SQ_ALLOCA_FREE(pargv);
			goto push_result;
		}

		VM_CASE(SQ_OC_RETURN)
			return operands[0];

		VM_CASE(SQ_OC_THROW)
			// TODO: catch thrown values and free memory in the current journey.
			sq_throw_value(operands[0]);

		VM_CASE(SQ_OC_POPTRYCATCH)
			sq_exception_pop();
			continue;

		VM_CASE(SQ_OC_TRYCATCH) {
			// todo: maybe have this be within the `stackframe`?
			unsigned catch_index = next_index(sf);
			unsigned exception_index = next_index(sf);

			if (!setjmp(exception_handlers[current_exception_handler++]))
				continue;

			sf->locals[exception_index] = sq_current_exception;
			sq_current_exception = SQ_NI;
			sf->ip = catch_index;
			continue;
		}

	/** Misc **/
		VM_CASE(SQ_OC_CITE) {
			struct sq_other *ptr = sq_mallocv(struct sq_other);
			ptr->kind = SQ_OK_CITATION;
			ptr->citation = next_local(sf);
			SET_RESULT(sq_value_new_other(ptr));
		}

	/** Logic **/
		VM_CASE(SQ_OC_NOT) SET_RESULT(sq_value_new_veracity(sq_value_not(operands[0])));
		VM_CASE(SQ_OC_EQL) SET_RESULT(sq_value_new_veracity(sq_value_eql(operands[0], operands[1])));
		VM_CASE(SQ_OC_NEQ) SET_RESULT(sq_value_new_veracity(sq_value_neq(operands[0], operands[1])));
		VM_CASE(SQ_OC_LTH) SET_RESULT(sq_value_new_veracity(sq_value_lth(operands[0], operands[1])));
		VM_CASE(SQ_OC_GTH) SET_RESULT(sq_value_new_veracity(sq_value_gth(operands[0], operands[1])));
		VM_CASE(SQ_OC_LEQ) SET_RESULT(sq_value_new_veracity(sq_value_leq(operands[0], operands[1])));
		VM_CASE(SQ_OC_GEQ) SET_RESULT(sq_value_new_veracity(sq_value_geq(operands[0], operands[1])));
		VM_CASE(SQ_OC_CMP) SET_RESULT(sq_value_new_numeral(sq_value_cmp(operands[0], operands[1])));

	/** Math **/
		VM_CASE(SQ_OC_NEG) SET_RESULT(sq_value_neg(operands[0]));
		VM_CASE(SQ_OC_ADD) SET_RESULT(sq_value_add(operands[0], operands[1]));
		VM_CASE(SQ_OC_SUB) SET_RESULT(sq_value_sub(operands[0], operands[1]));
		VM_CASE(SQ_OC_MUL) SET_RESULT(sq_value_mul(operands[0], operands[1]));
		VM_CASE(SQ_OC_DIV) SET_RESULT(sq_value_div(operands[0], operands[1]));
		VM_CASE(SQ_OC_MOD) SET_RESULT(sq_value_mod(operands[0], operands[1]));
		VM_CASE(SQ_OC_POW) SET_RESULT(sq_value_pow(operands[0], operands[1]));
		VM_CASE(SQ_OC_INDEX) SET_RESULT(sq_value_index(operands[0], operands[1]));
		VM_CASE(SQ_OC_INDEX_ASSIGN)
			sq_value_index_assign(operands[0], operands[1], operands[2]);
			continue;

		VM_CASE(SQ_OC_MATCHES)
			SET_RESULT(sq_value_new_veracity(sq_value_matches(operands[0], operands[1])));

		VM_CASE(SQ_OC_PAT_NOT)
		VM_CASE_FT(SQ_OC_PAT_OR)
		VM_CASE_FT(SQ_OC_PAT_AND) {
			struct sq_other *helper = sq_mallocv(struct sq_other);
			helper->kind = SQ_OK_PAT_HELPER;
			helper->helper.left = operands[0];

			if (opcode == SQ_OC_PAT_NOT)
				helper->helper.kind = SQ_PH_NOT;
			else {
				helper->helper.right = operands[1];
				helper->helper.kind = opcode == SQ_OC_PAT_AND ? SQ_PH_AND : SQ_PH_OR;
			}

			SET_RESULT(sq_value_new_other(helper));
		}

	/*** Interpreter Stuff ***/
		VM_CASE(SQ_OC_CLOAD)
			index = next_index(sf);
			sq_assert_lt(index, code->nconsts);

			SET_RESULT(code->consts[index]);

		VM_CASE(SQ_OC_GLOAD)
			index = next_index(sf);
			sq_assert_lt(index, sf->journey->program->nglobals);

			SET_RESULT(sf->journey->program->globals[index]);

		VM_CASE(SQ_OC_GSTORE)
			index = next_index(sf);
			sq_assert_lt(index, sf->journey->program->nglobals);

			sf->journey->program->globals[index] = operands[0];
			continue;

		VM_CASE(SQ_OC_ILOAD)
			index = next_index(sf);
			sq_assert_lt(index, code->nconsts);

			operands[1] = code->consts[index];
			sq_assert(sq_value_is_text(operands[1]));

			SET_RESULT(sq_value_get_attr(operands[0], sq_value_as_text(operands[1])->ptr));

		VM_CASE(SQ_OC_ISTORE)
			index = next_index(sf);
			sq_assert_lt(index, code->nconsts);

			operands[2] = code->consts[index];
			sq_assert(sq_value_is_text(operands[2]));

			sq_value_set_attr(operands[0], sq_value_as_text(operands[2])->ptr, operands[1]);
			continue;

		VM_CASE(SQ_OC_FEGENUS_STORE)
			index = next_index(sf);
			sq_assert(sq_value_is_form(operands[0]));
			sq_assert_lt(index, sq_value_as_form(operands[0])->vt->nessences);
			sq_assert(sq_value_as_form(operands[0])->vt->essences[index].genus == SQ_UNDEFINED);
			sq_value_as_form(operands[0])->vt->essences[index].genus = operands[1];
			continue;

		VM_CASE(SQ_OC_FMGENUS_STORE)
			index = next_index(sf);
			sq_assert(sq_value_is_form(operands[0]));
			sq_assert_lt(index, sq_value_as_form(operands[0])->vt->nmatter);
			sq_assert(sq_value_as_form(operands[0])->vt->matter[index].genus == SQ_UNDEFINED);
			sq_value_as_form(operands[0])->vt->matter[index].genus = operands[1];
			continue;
		VM_SWITCH_END

		SQ_UNREACHABLE;

	push_result:

		sq_assert_nundefined(result);
		set_next_local(sf, result);
	}

	return SQ_NI;
}
