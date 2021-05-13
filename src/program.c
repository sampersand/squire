#include "program.h"
#include "value.h"
#include "function.h"
#include "shared.h"
#include "string.h"
#include <stdlib.h>
#include <time.h>

void sq_program_run(struct sq_program *program, unsigned argc, const char **argv) {
	assert(program->main->argc == 0); // todo: allow for passing cmdline args
	srand(time(NULL));

	sq_value args[argc];

	for (unsigned i = 0; i < argc; ++i)
		args[i] = sq_value_new_string(sq_string_borrowed((char *) argv[i]));

	sq_value_free(sq_function_run(program->main, argc, args));

	for (unsigned i = 0; i < argc; ++i)
		sq_value_free(args[i]);
}

void sq_program_free(struct sq_program *program) {
	for (unsigned i = 0; i < program->nglobals; ++i)
		sq_value_free(program->globals[i]);

	free(program->globals);
	sq_function_free(program->main);

	free(program);
}
