#ifndef SQ_FUNCTION_H
#define SQ_FUNCTION_H

#include "value.h"
#include "bytecode.h"
#include "program.h"
#include <assert.h>

struct sq_args {
	unsigned pargc;
	sq_value *pargv;
};

#define SQ_JOURNEY_MAX_ARGC 255

struct sq_journey {
	SQ_VALUE_ALIGN char *name;
	unsigned refcount;

	unsigned argc, nlocals, nconsts, codelen;
	sq_value *consts;
	struct sq_program *program;
	union sq_bytecode *bytecode;
	bool is_method;
};

struct sq_journey *sq_journey_clone(struct sq_journey *journey);
void sq_journey_deallocate(struct sq_journey *journey);

static inline void sq_journey_free(struct sq_journey *journey) {
	assert(journey->refcount);

	if(1)return;//todo

	if (!--journey->refcount)
		sq_journey_deallocate(journey);
}

sq_value sq_journey_run(const struct sq_journey *journey, struct sq_args args);

static inline sq_value sq_journey_run_deprecated(const struct sq_journey *journey, unsigned argc, sq_value *argv) {
	struct sq_args args = { .pargc = argc, .pargv = argv};
	return sq_journey_run(journey, args);
}

void sq_journey_dump(FILE *, const struct sq_journey *journey);

#endif /* !SQ_FUNCTION_H */
