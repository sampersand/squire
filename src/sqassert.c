#include <squire/sqassert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void sq_assert_failed(
	const char *file, 
	const char *function,
	unsigned long line,
	const char *assertion,
	const char *fmt,
	...
) { 
	fprintf(stderr, "%s:%ld:%s: assertion (%s) failed", file, line, function, assertion);
	if (*fmt != 0) fputs(": ", stderr);

	va_list args;
	va_start(args, fmt);

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
	vfprintf(stderr, fmt, args);
#ifdef __clang__
# pragma clang diagnostic pop
#endif

	putc('\n', stderr);

	fflush(stderr);
#if SQ_HAS_BUILTIN(__builtin_debugtrap)
	__builtin_debugtrap();
#endif

	abort();
}
