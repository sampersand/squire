#include <squire/log.h>
#include <stdarg.h>
#include <stdio.h>

void sq_log_fn(const char *category, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	printf("LOG [%s] ", category);
	vprintf(fmt, args);
	putchar('\n');
}
