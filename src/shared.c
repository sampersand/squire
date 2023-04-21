#include <squire/shared.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void SQ_NORETURN memory_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "memory error: ");
	vfprintf(stderr, fmt, args);
	putc('\n', stderr);

	abort();
}

void *sq_malloc_heap(size_t size) {
	void *ptr = malloc(size);

	if (ptr == NULL && size)
		memory_error("unable to allocate %zu bytes of memory", size);

	return ptr;
}

void *sq_calloc(size_t count, size_t size) {
	void *ptr = calloc(count, size);

	if (ptr == NULL && size)
		memory_error("unable to zero-allocate %zu bytes of memory", size);

	return ptr;
}

void *sq_realloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);

	if (ptr == NULL && size != 0)
		memory_error("unable to reallocate %zu bytes of memory", size);

	return ptr;
}

void *sq_memdup(void *ptr, size_t size) {
	ptr = memcpy(sq_malloc_heap(size), ptr, size);

	if (ptr == NULL && size != 0)
		memory_error("sq_memdup failed for size %zu", size);

	return ptr;
}

void sq_internal_bug_fn(const char *file, const char *function, size_t line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "bug at %s:%zu (%s): ", file, line, function);
	vfprintf(stderr, fmt, args);
	putc('\n', stderr);

#if SQ_HAS_BUILTIN(__builtin_debugtrap)
	__builtin_debugtrap();
#endif

	abort();
}

char *sq_read_file(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (file == NULL)
		sq_throw("unable to read file '%s': %s", filename, strerror(errno));

	size_t length = 0;
	size_t capacity = 2048;
	char *contents = sq_malloc_heap(capacity);

	while (!feof(file)) {
		size_t amntread = fread(&contents[length], 1, capacity - length, file);

		if (amntread == 0) {
			if (!feof(file))
				sq_throw("unable to read file '%s': %s'", filename, strerror(errno));
			break;
		}

		length += amntread;

		if (length == capacity) {
			capacity *= 2;
			contents = sq_realloc(contents, capacity);
		}
	}

	if (fclose(file) == EOF)
		perror("couldn't close input file");

	contents = sq_realloc(contents, length + 1);
	contents[length] = '\0';
	return contents;
}
