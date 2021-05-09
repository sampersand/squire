#include "shared.h"
#include <string.h>

void *xmalloc(size_t length) {
	void *ptr = malloc(length);
	if (!ptr) abort();
	return ptr;
}

void *xrealloc(void *ptr, size_t length) {
	ptr = realloc(ptr, length);
	if (!ptr && length) abort();
	return ptr;
}

void *memdup(void *ptr, size_t length) {
	return memcpy(xmalloc(length), ptr, length);
}