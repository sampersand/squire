#ifndef SQ_ROMAN_H
#define SQ_ROMAN_H

#include "numeral.h"
#include <stdbool.h>

sq_numeral sq_roman_to_numeral(const char *input, const char **output);

// note that this returns an owned text.
char *sq_numeral_to_roman(sq_numeral);

// note that this returns an owned text.
char *sq_numeral_to_arabic(sq_numeral);

static inline bool sq_roman_is_numeral(char c) {
	return c == 'N' || c == 'I' || c == 'V' || c == 'X'
		|| c == 'L' || c == 'C' || c == 'D' || c == 'M';
}

#endif /* SQ_ROMAN_H */
