#ifndef SQ_SHARED_H
#define SQ_SHARED_H

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(SQ_USE_ATTRIBUTES)
# define SQ_ATTR(...) __attribute__((__VA_ARGS__))
#else
# define SQ_ATTR(...)
#endif /* __GNUC__ || SQ_USE_ATTRIBUTES */

#define SQ_NONNULL SQ_ATTR(nonnull)
#define sizeof_array(kind, length) (sizeof(kind) * (length))

#include <squire/exception.h>

#define die sq_throw
#define todo(...) (fprintf(stderr, __VA_ARGS__), exit(1))
#define bug(...) (\
	fprintf(stderr, "bug at " __FILE__ ":%s:%d: ", __func__, __LINE__),\
	fprintf(stderr, __VA_ARGS__),\
	fputc('\n', stderr),\
	abort())

void *xcalloc(size_t count, size_t size) SQ_ATTR(malloc);
void *xmalloc(size_t size) SQ_ATTR(malloc);
void *xrealloc(void *ptr, size_t size);
void *memdup(void *ptr, size_t size);

void sq_memory_error(char *msg, ...) SQ_ATTR(cold,noreturn);

#endif /* !SQ_SHARED_H */
