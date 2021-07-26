#include <squire/shared.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void *xmalloc(size_t size) {
	void *ptr = malloc(size);

	if (ptr == NULL)
		sq_memory_error("unable to allocate %zu bytes of memory", size);

	return ptr;
}

void *xcalloc(size_t count, size_t size) {
	void *ptr = calloc(count, size);

	if (ptr == NULL)
		sq_memory_error("unable to zero-allocate %zu bytes of memory", size);

	return ptr;
}

void *xrealloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);

	if (ptr == NULL && size != 0)
		sq_memory_error("unable to reallocate %zu bytes of memory", size);

	return ptr;
}

void *memdup(void *ptr, size_t size) {
	ptr = memcpy(xmalloc(size), ptr, size);

	if (ptr == NULL && size != 0)
		sq_memory_error("memdup failed for size %zu", size);

	return ptr;
}

void sq_memory_error(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "memory error: ");
	vfprintf(stderr, fmt, args);
	putc('\n', stderr);

	abort();
}
