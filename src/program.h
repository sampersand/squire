#pragma once
#include "value.h"

struct sq_variable {
	char *name;
	sq_value value;
};

struct sq_program {
	unsigned nglobals;
	struct sq_variable *globals;

	unsigned nfuncs;
	struct sq_function **funcs;
};

void sq_program_run(struct sq_program *program);
void sq_program_free(struct sq_program *program);
