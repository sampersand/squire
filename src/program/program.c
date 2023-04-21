#include <squire/book.h>
#include <squire/program.h>
#include <squire/value.h>
#include <squire/journey.h>
#include <squire/shared.h>
#include <squire/exception.h>
#include <squire/text.h>

#include <stdlib.h>
#include <time.h>
#include <string.h>

extern void sq_io_startup(struct sq_program *program);
void sq_program_initialize(struct sq_program *program) {
	sq_exception_init(program);
	sq_io_startup(program);	
}

void sq_program_mark(struct sq_program *program) {
	for (unsigned i = 0; i < program->nglobals; ++i) {
		if (program->globals[i] != SQ_UNDEFINED)
			sq_value_mark(program->globals[i]);
	}

	sq_journey_mark(program->main);

	for (unsigned i = 0; i < sq_current_stackframe; ++i)
		sq_stackframe_mark(&sq_stackframes[i]);
}


static sq_value create_argv(unsigned argc, const char **argv) {
	sq_value *args = sq_malloc_vec(sq_value, argc);

	for (unsigned i = 0; i < argc; ++i)
		args[i] = sq_value_new_text(sq_text_new(strdup(argv[i])));

	return sq_value_new_book(sq_book_new2(argc, args));
}

void sq_program_run(struct sq_program *program, unsigned argc, const char **argv) {
	sq_assert_eq(1, program->main->npatterns);
	sq_assert_z(program->main->patterns[0].pargc);
	srand(time(NULL));

	program->globals[0] = create_argv(argc, argv);
	sq_exception_init(program);
	sq_io_startup(program);

	SQ_ALLOCA(sq_value, args, argc);

	for (unsigned i = 0; i < argc; ++i)
		args[i] = sq_value_new_text(sq_text_new(strdup(argv[i])));

	sq_journey_run_deprecated(program->main, 0, NULL);
	SQ_ALLOCA_FREE(args);
}

void sq_program_finish(struct sq_program *program) {
	free(program->globals);
}
