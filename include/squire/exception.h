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
extern struct sq_form sq_exception_form;

struct sq_program;
void sq_exception_init(struct sq_program *program);

void sq_throw_value(sq_value) SQ_ATTR(cold,noreturn);
void sq_throw2(struct sq_form *form, const char *fmt, ...) SQ_ATTR(cold,noreturn);

#define sq_throw(...) sq_throw2(&sq_exception_form, __VA_ARGS__)

void sq_throw_io(const char *fmt, ...) SQ_ATTR(cold,noreturn);

/*
  catapult "lol" # you catapult a "projectile"
} victory { # nothing was thrown
  proclaim("haha!");
} alas err { # catch
  proclaim("looks like we got roasted: \(err)");
} verily { # finally
  proclaim("...");
}

*/

static inline void sq_exception_pop(void) {
	--current_exception_handler;
}

#endif /* !SQ_EXCEPTIONS */
