#ifndef SQ_ATTRIBUTES_H
#define SQ_ATTRIBUTES_H

#ifdef __has_builtin
# define SQ_HAS_BUILTIN(x) __has_builtin(x)
#else
# define SQ_HAS_BUILTIN(x) 0
#endif /* defined(__has_builtin) */

#ifdef __has_attribute
# define SQ_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
# define SQ_HAS_ATTRIBUTE(x) 0
#endif /* defined(__has_attribute) */

#if defined(__GNUC__) || defined(__clang__)
# define SQ_ATTR(...) __attribute__((__VA_ARGS__))
#else
# define SQ_ATTR(...)
#endif /* __GNUC__ || __clang__ */

#define SQ_NORETURN _Noreturn
#define SQ_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)

#if SQ_HAS_ATTRIBUTE(cold)
# define SQ_COLD SQ_ATTR(cold)
#else
# define SQ_COLD
#endif

#if SQ_HAS_ATTRIBUTE(enum_extensibility)
# define SQ_CLOSED_ENUM SQ_ATTR(enum_extensibility(closed))
#else
# define SQ_CLOSED_ENUM
#endif /* SQ_HAS_ATTRIBUTE(enum_extensibility) */

#if SQ_HAS_ATTRIBUTE(flag_enum)
# define SQ_FLAG_ENUM SQ_ATTR(flag_enum)
#else
# define SQ_FLAG_ENUM
#endif /* SQ_HAS_ATTRIBUTE(enum_extensibility) */

#if SQ_HAS_ATTRIBUTE(printf)
# define SQ_ATTR_PRINTF(fmt_index, va_start) SQ_ATTR(format(__printf__, fmt_index, va_start))
#else
# define SQ_ATTR_PRINTF(fmt_index, va_start)
#endif /* SQ_HAS_ATTRIBUTE(printf) */

#if SQ_HAS_ATTRIBUTE(nodiscard)
# define SQ_NODISCARD SQ_ATTR(nodiscard)
#elif SQ_HAS_ATTRIBUTE(warn_unused_result)
# define SQ_NODISCARD SQ_ATTR(warn_unused_result)
#else
# define SQ_NODISCARD
#endif /* SQ_HAS_ATTRIBUTE(nodiscard) */

#if SQ_HAS_ATTRIBUTE(noalias)
# define SQ_ATTR_NOALIAS SQ_ATTR(noalias)
#else
# define SQ_ATTR_NOALIAS
#endif /* SQ_HAS_ATTRIBUTE(noalias) */

#if SQ_HAS_ATTRIBUTE(noescape)
# define SQ_ATTR_NOESCAPE SQ_ATTR(noescape)
#else
# define SQ_ATTR_NOESCAPE
#endif /* SQ_HAS_ATTRIBUTE(noescape) */

#if SQ_HAS_ATTRIBUTE(fallthrough)
# define SQ_FALLTHROUGH SQ_ATTR(fallthrough)
#else
# define SQ_FALLTHROUGH
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

#if SQ_HAS_ATTRIBUTE(nonnull)
# define SQ_NONNULL SQ_ATTR(nonnull)
#else
# define SQ_NONNULL
#endif /* SQ_HAS_ATTRIBUTE(nonnull) */

#if SQ_HAS_ATTRIBUTE(returns_nonnull)
# define SQ_RETURNS_NONNULL SQ_ATTR(returns_nonnull)
#else
# define SQ_RETURNS_NONNULL
#endif /* SQ_HAS_ATTRIBUTE(nonnull) */

#if SQ_HAS_ATTRIBUTE(nullable)
# define SQ_NULLABLE SQ_ATTR(nullable)
#else
# define SQ_NULLABLE 
#endif /* SQ_HAS_ATTRIBUTE(nonnull) */

#endif /* !defined(SQ_ATTRIBUTES_H) */
