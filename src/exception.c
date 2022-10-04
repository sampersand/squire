#include <squire/exception.h>
#include <squire/text.h>
#include <squire/value.h>
#include <squire/form.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// jmp_buf redo_location;
jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
sq_value exception;
unsigned current_exception_handler;

// very basics of an exception with a form. todo: that
struct sq_form sq_exception_form;

void sq_exception_init(struct sq_program *program) {
	sq_exception_form.name = "Rock";
	sq_exception_form.refcount = 1;

	sq_exception_form.nmatter = 1;
	sq_exception_form.matter = xmalloc(sizeof(struct sq_form_matter));
	sq_exception_form.matter[0].name = "msg";
	sq_exception_form.matter[0].genus = SQ_UNDEFINED; // todo: make it text.

	sq_exception_form.nchanges = 1;
	sq_exception_form.changes = xmalloc(sizeof(struct sq_journey *));
	struct sq_journey *to_text = sq_exception_form.changes[0] = xmalloc(sizeof(struct sq_journey));

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
	// to_text->bytecode = xmalloc(sizeof_array(union sq_bytecode, 4));
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

void sq_throw2(struct sq_form *form, const char *fmt, ...) {
	(void) form;

	char *message;
	va_list args;
	va_start(args, fmt);
	vasprintf(&message, fmt, args);
	va_end(args);

	sq_throw_value(sq_value_new_text(sq_text_new(message)));
}

void sq_throw_value(sq_value value)  {
	if (!current_exception_handler) {
		fprintf(stderr, "uncaught exception encountered: ");
		sq_value_dump(stderr, value);
		putc('\n', stderr);
		exit(1);
	}

	sq_value_free(exception);
	exception = value;

	longjmp(exception_handlers[--current_exception_handler], 1);
}

void sq_throw_io(const char *fmt, ...) {
	const char *error = strerror(errno);
	char *message;
	va_list args;
	va_start(args, fmt);
	vasprintf(&message, fmt, args);
	va_end(args);

	char *msg2;
	asprintf(&msg2, "io error %s: %s", message, error);

	sq_throw_value(sq_value_new_text(sq_text_new(msg2)));
}
