#ifndef sq_numeral_H
#define sq_numeral_H

#include <inttypes.h>

typedef int64_t sq_numeral;

struct sq_text;

sq_numeral sq_roman_to_numeral(const char *input, const char **output);
struct sq_text *sq_numeral_to_roman(sq_numeral numeral);
struct sq_text *sq_numeral_to_arabic(sq_numeral numeral);

extern struct sq_text sq_text_zero;

#ifdef SQ_NUMERAL_TO_ARABIC
# define sq_numeral_to_text sq_numeral_to_arabic
#else
# define sq_numeral_to_text sq_numeral_to_roman
#endif /* SQ_NUMERAL_TO_ARABIC */

static inline _Bool sq_numeral_is_roman(char c) {
	return c == 'N' || c == 'I' || c == 'V' || c == 'X'
		|| c == 'L' || c == 'C' || c == 'D' || c == 'M';
}

#endif /* !sq_numeral_H */
