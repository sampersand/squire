#include <squire/other/io.h>
#include <squire/other/kingdom.h>
#include <squire/other/other.h>
#include <squire/program.h>

static struct sq_other sq_io_kingdom_other = {
	.basic = SQ_BASIC_DEFAULT,
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

	// todo: set `Kingdom` in `globals`.
	(void) program->globals;
}
