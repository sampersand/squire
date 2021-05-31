#ifndef SQ_FUNCTION_H
#define SQ_FUNCTION_H

#include "value.h"
#include "bytecode.h"
#include "program.h"
#define MAX_ARGC 255

struct sq_function {
	char *name;
	int refcount; // negative indicates a global function.

	unsigned argc, nlocals, nconsts, codelen;
	sq_value *consts;
	struct sq_program *program;
	union sq_bytecode *bytecode;
	bool is_method;
};

struct sq_function *sq_function_clone(struct sq_function *function);
void sq_function_free(struct sq_function *function);
sq_value sq_function_run(struct sq_function *function, unsigned argc, sq_value *args);
void sq_function_dump(const struct sq_function *function);

#endif /* !SQ_FUNCTION_H */
