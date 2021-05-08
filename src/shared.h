#pragma once

#include <stdio.h>
#include <stdlib.h>

#define die(...) (fprintf(stderr,__VA_ARGS__),exit(1)) 
#define bug(msg, ...) (fprint("bug at " __FILE__ ":%s:%d: " msg, __func__, __LINE__, __VA_ARGS__),abort())
static inline void *xmalloc(size_t length) {
	void *ptr = malloc(length);
	if (!ptr) abort();
	return ptr;
}

static inline void *xrealloc(void *ptr, size_t length) {
	ptr = realloc(ptr, length);
	if (!ptr && length) abort();
	return ptr;
}