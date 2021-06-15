#ifndef SQ_SHARED_H
#define SQ_SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include "exception.h"

#define die sq_throw
#define todo(...) (fprintf(stderr, __VA_ARGS__), exit(1))
#define bug(msg, ...) (fprintf(stderr, "bug at " __FILE__ ":%s:%d: " msg, __func__, __LINE__, __VA_ARGS__),abort())

void *xmalloc(size_t length);
void *xrealloc(void *ptr, size_t length);
void *memdup(void *ptr, size_t length);

#endif /* !SQ_SHARED_H */
