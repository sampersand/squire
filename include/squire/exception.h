#ifndef SQ_EXCEPTIONS
#define SQ_EXCEPTIONS

#ifndef SQ_NUM_EXCEPTION_HANDLERS
# define SQ_NUM_EXCEPTION_HANDLERS 2048
#endif

#include <setjmp.h>
#include <squire/value.h>
#include <squire/shared.h>
#include <stdio.h>
#include <errno.h>

// jmp_buf redo_location;
extern jmp_buf exception_handlers[SQ_NUM_EXCEPTION_HANDLERS];
extern sq_value exception;
extern unsigned current_exception_handler;
extern struct sq_form sq_exception_form, sq_io_exception_form;

struct sq_program;
void sq_exception_init(struct sq_program *program);

void sq_throw_value(sq_value) SQ_ATTR_NORETURN_COLD;
void sq_throw2(struct sq_form *form, const char *fmt, ...)
	SQ_ATTR_NORETURN_COLD
	SQ_ATTR_PRINTF(2, 3);

#define sq_throw(...) sq_throw2(&sq_exception_form, __VA_ARGS__)

void sq_throw_io(const char *fmt, ...)
	SQ_ATTR_NORETURN_COLD
	SQ_ATTR_PRINTF(1, 2);


#define sq_throw_io(...) sq_throw_io_(__VA_ARGS__, strerror(errno))
#define sq_throw_io_(fmt, ...) sq_throw2(&sq_io_exception_form, "io error: " fmt ": %s", __VA_ARGS__)

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
