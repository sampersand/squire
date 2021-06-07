#include "exception.h"
#include "string.h"
#include "value.h"
#include <stdarg.h>
#include <stdlib.h>

// jmp_buf redo_location;
jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
sq_value exception;
unsigned current_exception_handler;

void sq_throw(const char *fmt, ...) {
	char *message;
	va_list args;
	va_start(args, fmt);
	vasprintf(&message, fmt, args);
	va_end(args);

	sq_throw_value(sq_value_new_string(sq_string_new(message)));
}

void sq_throw_value(sq_value value)  {
	if (!current_exception_handler) {
		fprintf(stderr, "uncaught exception encountered: ");
		sq_value_dump_to(stderr, value);
		exit(1);
	}

	sq_value_free(exception);
	exception = value;

	longjmp(exception_handlers[--current_exception_handler], 1);
}

