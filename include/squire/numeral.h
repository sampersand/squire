#ifndef sq_numeral_H
#define sq_numeral_H

#include <inttypes.h>
#include <stdbool.h>
#include <squire/attributes.h>

typedef int64_t sq_numeral;

struct sq_text;

sq_numeral sq_roman_to_numeral(const char *input, const char **output) SQ_NODISCARD;
struct sq_text *sq_numeral_to_roman(sq_numeral numeral) SQ_NODISCARD;
struct sq_text *sq_numeral_to_arabic(sq_numeral numeral) SQ_NODISCARD;

extern struct sq_text sq_text_zero_numeral, sq_text_zero_arabic;

#ifdef SQ_NUMERAL_TO_ARABIC
# define sq_text_zero sq_text_zero_arabic
# define sq_numeral_to_text sq_numeral_to_arabic
#else
# define sq_text_zero sq_text_numeral
# define sq_numeral_to_text sq_numeral_to_roman
#endif /* SQ_NUMERAL_TO_ARABIC */

bool sq_numeral_starts(const char *text) SQ_NODISCARD SQ_NONNULL;

#endif /* !sq_numeral_H */
