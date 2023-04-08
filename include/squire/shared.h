#ifndef SQ_SHARED_H
#define SQ_SHARED_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __has_builtin
# define SQ_HAS_BUILTIN(x) __has_builtin(x)
#else
# define SQ_HAS_BUILTIN(x) 0
#endif /* defined __has_builtin */

#ifdef __has_attribute
# define SQ_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
# define SQ_HAS_ATTRIBUTE(x) 0
#endif /* defined __has_attribute */

#if defined(__GNUC__) || defined(__clang__)
# define SQ_ATTR(...) __attribute__((__VA_ARGS__))
#else
# define SQ_ATTR(...)
#endif /* __GNUC__ || __clang__ */

#if SQ_HAS_ATTRIBUTE(nonnull)
# define SQ_NONNULL SQ_ATTR(nonnull)
#else
# define SQ_NONNULL
#endif /* SQ_HAS_ATTRIBUTE(nonnull) */

#if SQ_HAS_BUILTIN(__builtin_expect)
# define SQ_LIKELY(x) (__builtin_expect(1, !!(x)))
#else
# define SQ_LIKELY(x) (!!(x))
#endif /* SQ_HAS_BUILTIN(__builtin_expect) */
#define SQ_UNLIKELY(x) SQ_LIKELY(!(x))

#if SQ_HAS_BUILTIN(__builtin_unreachable)
# define SQ_UNREACHABLE do { __builtin_unreachable(); } while(0)
#else
# define SQ_UNREACHABLE do { abort(); } while(0)
#endif /* SQ_HAS_BUILTIN(__builtin_unreachable) */

#ifdef SQ_RELEASE_FAST
# define sq_bug(...) SQ_UNREACHABLE
#else
# define sq_bug(...) do {sq_bug_fn(__FILE__, __func__, __LINE__, __VA_ARGS__); } while(0)
#endif /* defined(SQ_RELEASE_FAST) */

#ifdef SQ_LOG
#define sq_log printf
#else
#define sq_log(...) ((void) 0)
#endif /* SQ_LOG */


#include <squire/exception.h>
#define die sq_throw
#define sq_todo(...) (fprintf(stderr, __VA_ARGS__), exit(1))

#define sq_sizeof_array(kind, length) (sizeof(kind) * (length))

#define sq_malloc_vec(kind, length) (sq_malloc(sizeof(kind) * (length)))
#define sq_realloc_vec(kind, ptr, length) (sq_realloc((ptr), sizeof(kind) * (length)))

void *sq_calloc(size_t count, size_t size) SQ_ATTR(malloc) ;
void *sq_malloc(size_t size) SQ_ATTR(malloc);
void *sq_realloc(void *ptr, size_t size);
void *sq_memdup(void *ptr, size_t size);

void sq_bug_fn(
	const char *file, const char *function, long long line, const char *fmt, ...
) SQ_ATTR(cold,noreturn);
void sq_memory_error(const char *fmt, ...) SQ_ATTR(cold,noreturn);
char *sq_read_file(const char *filename);

#endif /* !SQ_SHARED_H */
