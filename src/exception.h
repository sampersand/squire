#ifndef SQ_EXCEPTIONS
#define SQ_EXCEPTIONS

#ifndef SQ_NUM_EXCEPTION_HANDLERS
# define SQ_NUM_EXCEPTION_HANDLERS 2048
#endif

#include <setjmp.h>
#include "value.h"
#include "shared.h"

// jmp_buf redo_location;
extern jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
extern sq_value exception;
extern unsigned current_exception_handler;

void sq_throw(const char *fmt, ...) SQ_ATTR(cold,noreturn);
void sq_throw_value(sq_value) SQ_ATTR(cold,noreturn);

static inline void sq_exception_pop(void) {
	--current_exception_handler;
}

#endif /* !SQ_EXCEPTIONS */
