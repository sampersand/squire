#include "exception.h"
#include "text.h"
#include "value.h"
#include <stdarg.h>
#include <stdlib.h>

// jmp_buf redo_location;
jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
sq_value exception;
unsigned current_exception_handler;

struct sq_form sq_exception_form = {

};

void sq_throw2(struct sq_form *form, const char *fmt, ...) {
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
		sq_value_dump_to(stderr, value);
		putc('\n', stderr);
		exit(1);
	}

	sq_value_free(exception);
	exception = value;

	longjmp(exception_handlers[--current_exception_handler], 1);
}

