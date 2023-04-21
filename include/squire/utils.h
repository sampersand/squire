#ifndef SQ_UTILS_H
#define SQ_UTILS_H

#define SQ_CAR(x, ...) x
#define SQ_CDR(x, ...) __VA_ARGS__

#define SQ_TO_STRING_(x) #x
#define SQ_TO_STRING(x) SQ_TO_STRING_(x)

#ifdef __FILE_NAME__
# define SQ_FILENAME __FILE_NAME__
#else
# define SQ_FILENAME __FILE__
#endif

#endif
