#ifndef SQ_SHARED_H
#define SQ_SHARED_H

#include <squire/attributes.h>
#include <squire/gc.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SQ_LOG
#define sq_log_old printf
#else
#define sq_log_old(...) ((void) 0)
#endif /* SQ_LOG */
// #pragma unroll 16

#ifdef SQ_USE_ALLOCA
# pragma clang diagnostic ignored "-Wvla"
# define SQ_ALLOCA(type, name, amnt) type name[amnt];
# define SQ_ALLOCA_FREE(name)
#else
# define SQ_ALLOCA(type, name, amnt) type *name = sq_malloc_vec(type, amnt);
# define SQ_ALLOCA_FREE(name) free(name)
#endif


#include <squire/exception.h>
#define sq_todo(...) (fprintf(stderr, __VA_ARGS__), exit(1))

#define sq_sizeof_array(kind, length) (sizeof(kind) * (length))
#define sq_mallocz(kind) ((kind *) sq_calloc(1, sizeof(kind)))
#define sq_mallocv(kind) (sq_assert(sizeof(kind) <= SQ_VALUE_SIZE),\
	(kind *) sq_gc_malloc( \
	SQ_TAG_FOR(kind) \
))
#define sq_malloc_single(kind) ((kind *) sq_malloc_heap(sizeof(kind)))
#define sq_malloc_vec(kind, length) ((kind *) sq_malloc_heap(sq_sizeof_array(kind, (length))))
#define sq_realloc_vec(kind, ptr, length) ((kind *) sq_realloc((ptr), sq_sizeof_array(kind, (length))))

void *sq_malloc_heap(size_t size)
	SQ_NODISCARD
#if SQ_HAS_ATTRIBUTE(malloc)
	SQ_ATTR(malloc)
#endif
#if SQ_HAS_ATTRIBUTE(alloc_size)
	SQ_ATTR(alloc_size(1))
#endif
#if SQ_HAS_ATTRIBUTE(assume_aligned)
	SQ_ATTR(assume_aligned(SQ_VALUE_ALIGNMENT))
#endif
	;

void *sq_calloc(size_t count, size_t size)
	SQ_NODISCARD
#if SQ_HAS_ATTRIBUTE(malloc)
	SQ_ATTR(malloc)
#endif
#if SQ_HAS_ATTRIBUTE(alloc_size)
	SQ_ATTR(alloc_size(1, 2))
#endif
#if SQ_HAS_ATTRIBUTE(assume_aligned)
	SQ_ATTR(assume_aligned(SQ_VALUE_ALIGNMENT))
#endif
	;

void *sq_realloc(void *ptr, size_t size) SQ_NODISCARD;
void *sq_memdup(void *ptr, size_t size) SQ_NODISCARD;

SQ_NORETURN void sq_internal_bug_fn(const char *file, const char *fn, size_t line, const char *fmt, ...)
	SQ_COLD SQ_ATTR_PRINTF(4, 5) SQ_NONNULL;

#ifdef SQ_RELEASE_FAST
# define sq_bug(...) SQ_UNREACHABLE
#else
# define sq_bug(...) (sq_internal_bug_fn(__FILE__, __func__, __LINE__, __VA_ARGS__),SQ_UNREACHABLE)
#endif /* defined(SQ_RELEASE_FAST) */

char *sq_read_file(const char *filename) SQ_NONNULL SQ_RETURNS_NONNULL ;

#endif /* !SQ_SHARED_H */
