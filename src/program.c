#include "book.h"
#include "program.h"
#include "value.h"
#include "journey.h"
#include "shared.h"
#include "exception.h"
#include "text.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>


sq_value create_argv(unsigned argc, const char **argv) {
	sq_value *args = xmalloc(sizeof_array(sq_value, argc));

	for (unsigned i = 0; i < argc; ++i)
		args[i] = sq_value_new(sq_text_new(strdup(argv[i])));

	return sq_value_new_book(sq_book_new2(argc, args));
}

void sq_program_run(struct sq_program *program, unsigned argc, const char **argv) {
	assert(program->main->argc == 0);
	srand(time(NULL));

	program->globals[0] = create_argv(argc, argv);
	sq_exception_init(program);

	sq_value args[argc];

	for (unsigned i = 0; i < argc; ++i)
		args[i] = sq_value_new(sq_text_new(strdup(argv[i])));

	sq_value_free(sq_journey_run_deprecated(program->main, 0, NULL));

	for (unsigned i = 0; i < argc; ++i)
		sq_value_free(args[i]);
}

void sq_program_free(struct sq_program *program) {
	for (unsigned i = 0; i < program->nglobals; ++i)
		sq_value_free(program->globals[i]);

	free(program->globals);
	sq_journey_free(program->main);

	free(program);
}
