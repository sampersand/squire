#include "program.h"
#include "value.h"
#include "function.h"
#include "shared.h"
#include <string.h>
#include <stdlib.h>

void sq_program_run(struct sq_program *program) {
	int x;
	assert(program->main->argc == 0); // todo: allow for passing cmdline args
	srand((int) &x);

	sq_value_free(sq_function_run(program->main, 0, NULL));
}

void sq_program_free(struct sq_program *program) {
	for (unsigned i = 0; i < program->nglobals; ++i) {
		sq_value_free(program->globals[i]);
	}

	free(program->globals);

	for (unsigned i = 0; i < program->nfuncs; ++i)
		sq_function_free(program->funcs[i]);

	free(program->funcs);
	free(program);
}
