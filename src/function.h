#pragma once
#include "value.h"
#include "bytecode.h"

struct sq_program;

struct sq_function {
	char *name;
	int refcount; // negative indicates a global function.

	unsigned argc, nlocals, nconsts;
	struct sq_program *program;

	sq_value *consts;
	union sq_bytecode *code;
};

void sq_function_clone(struct sq_function *function);
void sq_function_free(struct sq_function *function);
sq_value sq_function_run(struct sq_function *function, sq_value *args);