#include "io.h"
#include "other/kingdom.h"
#include "other/other.h"
#include "program.h"

static struct sq_other sq_io_kingdom_other = {
	.refcount = -1,
	.kind = SQ_OK_KINGDOM,
	.kingdom = {
		.name = "Io"
	}
};
struct sq_kingdom *sq_io_kingdom = &sq_io_kingdom_other.kingdom;

extern void sq_io_startup(struct sq_program *program);

void sq_io_startup(struct sq_program *program) {
	sq_kingdom_initialize(sq_io_kingdom, 8);
	// sq_kingdom_set_attr(sq_io_kingdom, "Scroll", sq_scroll_form);

	program->globals;
}
