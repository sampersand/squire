#ifndef SQ_PROGRAM_H
#define SQ_PROGRAM_H

#include "value.h"

struct sq_program {
	unsigned nglobals, nfuncs;

	sq_value *globals;
	struct sq_function *main, **funcs;
};

struct sq_program *sq_program_compile(const char *stream, unsigned argc, char **argv);
void sq_program_run(struct sq_program *program);
void sq_program_free(struct sq_program *program);

#endif /* !SQ_PROGRAM_H */
