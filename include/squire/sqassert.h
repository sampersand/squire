#ifndef SQ_ASSERT_H
#define SQ_ASSERT_H

#include <squire/attributes.h>
#include <squire/utils.h>

void SQ_NORETURN sq_assert_failed(
	const char *file, 
	const char *function,
	unsigned long line,
	const char *assertion,
	const char *fmt,
	...
) SQ_ATTR_PRINTF(5, 6) SQ_COLD SQ_NONNULL;


#define SQ_STATIC_ASSERT _Static_assert

#ifdef SQ_NDEBUG
# define sq_assert_internal(cond, ...)
#else
# define sq_assert_internal(cond, ...) \
	(SQ_LIKELY(cond) \
		? ((void) 0) \
		: sq_assert_failed(SQ_FILENAME, __func__, __LINE__, SQ_TO_STRING(cond), __VA_ARGS__))
#endif

#define sq_assert(...) sq_assert_internal(SQ_CAR(__VA_ARGS__, _ignored), SQ_CDR(__VA_ARGS__, ""))
#define sq_assert_ne(l, ...) sq_assert((l) != (SQ_CAR(__VA_ARGS__, _ignored)), SQ_CDR(__VA_ARGS__, ""))
#define sq_assert_eq(l, ...) sq_assert((l) == (SQ_CAR(__VA_ARGS__, _ignored)), SQ_CDR(__VA_ARGS__, ""))
#define sq_assert_nn(...) sq_assert_ne(NULL, __VA_ARGS__)
#define sq_assert_n(...) sq_assert_eq(NULL, __VA_ARGS__)
#define sq_assert_nz(...) sq_assert_ne(0, __VA_ARGS__)
#define sq_assert_z(...) sq_assert_eq(0, __VA_ARGS__)
#define sq_assert_le(l, ...) sq_assert((l) <= (SQ_CAR(__VA_ARGS__, _ignored)), SQ_CDR(__VA_ARGS__, ""))
#define sq_assert_lt(l, ...) sq_assert((l) < (SQ_CAR(__VA_ARGS__, _ignored)), SQ_CDR(__VA_ARGS__, ""))
#define sq_assert_nundefined(...) sq_assert_ne(SQ_UNDEFINED, __VA_ARGS__)

#endif
