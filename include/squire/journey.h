#ifndef SQ_FUNCTION_H
#define SQ_FUNCTION_H

#include <squire/value.h>
#include <squire/bytecode.h>
#include <squire/program.h>
#include <squire/shared.h>

#include <assert.h>

struct sq_args {
	unsigned pargc, kwargc;
	sq_value *pargv;
	struct sq_arg_kw { const char *name; sq_value value; } *kwargv;
};

#define SQ_JOURNEY_MAX_ARGC 32 // seems like a reasonable maximum

struct sq_codeblock {
	unsigned nlocals, nconsts, codelen;
	sq_value *consts;
	union sq_bytecode *bytecode;
};

struct sq_journey_argument {
	char *name;
	int default_start, genus_start; // will be `-1` if no default or genus is supplied.
};

struct sq_journey_pattern {
	unsigned pargc, kwargc, start_index;
	bool splat, splatsplat;
	int condition_start; // if `-1`, there is no condition.
	struct sq_journey_argument *pargv, *kwargv;
	struct sq_codeblock code;
};

struct sq_journey {
	SQ_VALUE_ALIGN char *name;
	unsigned refcount, npatterns;
	struct sq_program *program;
	bool is_method;

	struct sq_journey_pattern *patterns;
};

void sq_journey_deallocate(struct sq_journey *journey);

static inline struct sq_journey *sq_journey_clone(struct sq_journey *journey) {
	assert(journey->refcount);

	++journey->refcount;

	return journey;
}

static inline void sq_journey_free(struct sq_journey *journey) {
	if(1)return;//todo

	assert(journey->refcount);

	if (!--journey->refcount)
		sq_journey_deallocate(journey);
}

sq_value sq_journey_run(const struct sq_journey *journey, struct sq_args args);

static inline void sq_journey_assert_arglen(struct sq_args args, unsigned pargc, unsigned kwargc) {
	if (args.pargc != pargc || args.kwargc != kwargc)
		sq_throw("Argument mismatch: given (pos=%d, kw=%d) expected (pos=%d,kw=%d)",
			pargc, kwargc, args.pargc, args.kwargc);
}

static inline sq_value sq_journey_run_deprecated(const struct sq_journey *journey, unsigned argc, sq_value *argv) {
	struct sq_args args = { .pargc = argc, .pargv = argv};
	return sq_journey_run(journey, args);
}

void sq_journey_dump(FILE *, const struct sq_journey *journey);

#endif /* !SQ_FUNCTION_H */
