#include "program.h"
#include "value.h"
#include "function.h"
#include "shared.h"
#include <string.h>

// struct sq_variable {
// 	char *name;
// 	sq_value value;
// };

// struct sq_program {
// 	unsigned nglobals;
// 	struct sq_variable *globals;

// 	unsigned nfuncs;
// 	struct sq_function **funcs;
// };

void sq_program_run(struct sq_program *program) {
	struct sq_function *main;

	for (unsigned i = 0; i < program->nfuncs; ++i) {
		main = program->funcs[i];

		if (strcmp("main", main->name))
			continue;

		// todo: allow for passing command line args?
		assert(main->argc == 0);
		sq_value_free(sq_function_run(main, NULL));
		return;
	}

	die("no 'main' function found.\n");
}

void sq_program_free(struct sq_program *program) {
	for (unsigned i = 0; i < program->nglobals; ++i) {
		free(program->globals[i].name);
		sq_value_free(program->globals[i].value);
	}

	free(program->globals);

	for (unsigned i = 0; i < program->nfuncs; ++i)
		sq_function_free(program->funcs[i]);

	free(program->funcs);
	free(program);
}
