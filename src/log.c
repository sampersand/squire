#include <squire/log.h>
#include <stdarg.h>
#include <stdio.h>

void sq_log_fn(const char *category, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	printf("LOG [%s] ", category);

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
	vprintf(fmt, args);
#ifdef __clang__
# pragma clang diagnostic pop
#endif

	putchar('\n');
}
