#ifndef SQ_ROMAN_H
#define SQ_ROMAN_H

#include "number.h"
#include <stdbool.h>

sq_number sq_roman_to_number(const char *input, const char **output);

// note that this returns an owned string.
char *sq_number_to_roman(sq_number);

// note that this returns an owned string.
char *sq_number_to_arabic(sq_number);

static inline bool sq_roman_is_numeral(char c) {
	return c == 'N' || c == 'I' || c == 'V' || c == 'X'
		|| c == 'L' || c == 'C' || c == 'D' || c == 'M';
}

#endif /* SQ_ROMAN_H */
