#include <squire/exception.h>
#include <squire/text.h>
#include <squire/value.h>
#include <squire/form.h>
#include <squire/other/other.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// jmp_buf redo_location;
jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
sq_value sq_current_exception;
unsigned current_exception_handler;

// very basics of an exception with a form. todo: that
struct sq_form sq_exception_form, sq_io_exception_form;
struct sq_form_vtable sq_exception_form_vtable, sq_io_exception_form_vtable;

void sq_exception_init(struct sq_program *program) {
	sq_exception_form.basic = SQ_STATIC_BASIC(struct sq_form);
	sq_exception_form.vt = &sq_exception_form_vtable;
	sq_exception_form.vt->name = "Rock";

	sq_exception_form.vt->nmatter = 1;
	sq_exception_form.vt->matter = sq_malloc_single(struct sq_form_matter);
	sq_exception_form.vt->matter[0].name = "msg";
	sq_exception_form.vt->matter[0].genus = SQ_UNDEFINED; // todo: make it text.

	sq_exception_form.vt->nchanges = 1;
	sq_exception_form.vt->changes = sq_malloc_single(struct sq_journey *);
	struct sq_journey *to_text = sq_exception_form.vt->changes[0] = sq_mallocv(struct sq_journey);

	(void) to_text;
	(void) program;

	// todo: totext.
	// to_text->name = "to_text";
	// to_text->refcount = 1;
	// to_text->argc = 1;
	// to_text->nlocals = 1;
	// to_text->nconsts = 1;
	// to_text->codelen = 94;
	// to_text->consts = NULL;
	// to_text->program = program;
	// to_text->is_method = true;
	// to_text->bytecode = sq_malloc_vec(union sq_bytecode, 4);
	// to_text->bytecode[0].opcode = SQ_OC_ILOAD;
	// to_text->bytecode[1].index = 0;
	// to_text->bytecode[2].index = 0;
	// to_text->bytecode[3].index = 0;


// struct sq_journey {
// 	SQ_VALUE_ALIGN char *name;
// 	unsigned refcount; // negative indicates a global function.

// 	unsigned argc, nlocals, nconsts, codelen;
// 	sq_value *consts;
// 	struct sq_program *program;
// 	union sq_bytecode *bytecode;
// 	bool is_method;
// };

}

#undef sq_throw_value
void sq_throw_value(sq_value value)  {
	if (!current_exception_handler) {
		fprintf(stderr, "uncaught exception encountered: ");
		sq_value_dump(stderr, value);
		putc('\n', stderr);
		exit(1);
	}

	sq_current_exception = value;

	longjmp(exception_handlers[--current_exception_handler], 1);
}

#undef sq_throw2
void sq_throw2(struct sq_form *form, const char *fmt, ...) {
	(void) form;

	char *message;
	va_list args;
	va_start(args, fmt);
	vasprintf(&message, fmt, args);
	va_end(args);

	sq_throw_value(sq_value_new_text(sq_text_new(message)));
}
