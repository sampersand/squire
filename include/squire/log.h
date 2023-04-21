#ifndef SQ_LOG_H
#define SQ_LOG_H

#include <squire/attributes.h>

#ifdef SQ_LOG_ALL
# define SQ_LOG_GC 1
# define SQ_LOG_TOKEN 1
#endif

#ifndef SQ_LOG_GC
# define SQ_LOG_GC 0
#endif

#ifndef SQ_LOG_TOKEN
# define SQ_LOG_TOKEN 0
#endif



void sq_log_fn(const char *category, const char *fmt, ...) SQ_NONNULL SQ_ATTR_PRINTF(2, 3);

#define SQ_EXPAND(x) x
#define sq_log(category, verbosity, ...) SQ_EXPAND(sq_log_ ## category ## _ ## verbosity)(__VA_ARGS__)

#if SQ_LOG_GC >= 2
# define sq_log_gc_2(...) sq_log_fn("GC[2]", __VA_ARGS__)
#else
# define sq_log_gc_2(...)
#endif

#if SQ_LOG_GC >= 1
# define sq_log_gc_1(...) sq_log_fn("GC[1]", __VA_ARGS__)
#else
# define sq_log_gc_1(...)
#endif

#if SQ_LOG_TOKEN >= 1
# define sq_log_token_1(...) sq_log_fn("TOKEN[1]", __VA_ARGS__)
#else
# define sq_log_token_1(...)
#endif

// #if SQ_LOG_TOKEN >= 1
// # define sq_log_parse(...) sq_log_fn("PARSE", __VA_ARGS__)
// #else
// # define sq_log_parse(...)
// #endif

#endif
