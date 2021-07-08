#ifndef SQ_FUNCTION_H
#define SQ_FUNCTION_H

#include "value.h"
#include "bytecode.h"
#include "program.h"
#include <assert.h>

#define MAX_ARGC 255

struct sq_journey {
	SQ_VALUE_ALIGN char *name;
	unsigned refcount; // negative indicates a global function.

	unsigned argc, nlocals, nconsts, codelen;
	sq_value *consts;
	struct sq_program *program;
	union sq_bytecode *bytecode;
	bool is_method;
};

struct sq_journey *sq_journey_clone(struct sq_journey *function);
void sq_journey_deallocate(struct sq_journey *function);

static inline void sq_journey_free(struct sq_journey *function) {
	assert(function->refcount);

	if(1)return;//todo

	if (!--function->refcount)
		sq_journey_deallocate(function);
}

sq_value sq_journey_run(const struct sq_journey *function, unsigned argc, sq_value *args);
void sq_journey_dump(FILE *, const struct sq_journey *function);

#endif /* !SQ_FUNCTION_H */
