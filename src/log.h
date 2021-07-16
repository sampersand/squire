#ifndef SQ_LOG_H
#define SQ_LOG_H

// we need to `#define` them because we need them in macros.
#define SQ_LOG_LEVEL_TRACE 0
#define SQ_LOG_LEVEL_DEBUG 1
#define SQ_LOG_LEVEL_INFO 2
#define SQ_LOG_LEVEL_WARN 3
#define SQ_LOG_LEVEL_ERROR 4
#define SQ_LOG_LEVEL_FATAL 5

#ifndef SQ_LOG_LEVEL
# define SQ_LOG_LEVEL SQ_LOG_LEVEL_WARN
#endif /* !SQ_LOG_LEVEL */

#define SQ_LOG_PREFIX(level) do { printf("[%5s] ", level); } while(0)
#define SQ_LOG(level, ...)         \
	do {                            \
		SQ_LOG_PREFIX(level);        \
		printf(__VA_ARGS__);         \
		putchar('\n');               \
	} while (0);

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_TRACE
# define SQ_LOG_TRACE(...) SQ_LOG("TRACE", __VA_ARGS__)
#else
# define SQ_LOG_TRACE(...)
#endif /* SQ_LOG_LEVEL_TRACE */

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_DEBUG
# define SQ_LOG_DEBUG(...) SQ_LOG("DEBUG", __VA_ARGS__)
#else
# define SQ_LOG_DEBUG(...)
#endif /* SQ_LOG_LEVEL_DEBUG */

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_INFO
# define SQ_LOG_INFO(...) SQ_LOG("INFO", __VA_ARGS__)
#else
# define SQ_LOG_INFO(...)
#endif /* SQ_LOG_LEVEL_INFO */

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_WARN
# define SQ_LOG_WARN(...) SQ_LOG("WARN", __VA_ARGS__)
#else
# define SQ_LOG_WARN(...)
#endif /* SQ_LOG_LEVEL_WARN */

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_ERROR
# define SQ_LOG_ERROR(...) SQ_LOG("ERROR", __VA_ARGS__)
#else
# define SQ_LOG_ERROR(...)
#endif /* SQ_LOG_LEVEL_ERROR */

#if SQ_LOG_LEVEL <= SQ_LOG_LEVEL_FATAL
# define SQ_LOG_FATAL(...) SQ_LOG("FATAL", __VA_ARGS__)
#else
# define SQ_LOG_FATAL(...)
#endif /* SQ_LOG_LEVEL_FATAL */

#endif /* SQ_LOG_H */
