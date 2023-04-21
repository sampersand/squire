#ifndef SQ_PROGRAM_H
#define SQ_PROGRAM_H

#include <squire/value.h>

struct sq_program {
	unsigned nglobals;
	sq_value *globals;
	struct sq_journey *main;
};

void sq_program_initialize(struct sq_program *);
void sq_program_mark(struct sq_program *);
void sq_program_compile(struct sq_program *program, const char *stream);
void sq_program_run(struct sq_program *program, unsigned argc, const char **argv);
void sq_program_finish(struct sq_program *program);

#endif /* !SQ_PROGRAM_H */
