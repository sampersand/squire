#ifndef SQ_IO_H
#define SQ_IO_H

#include "scroll.h"
#include "other/kingdom.h"

struct sq_program;
extern struct sq_kingdom *sq_io_kingdom;

void sq_io_startup(struct sq_program *program);

#endif /* !SQ_IO_H */
