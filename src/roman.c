#include "roman.h"
#include <ctype.h>

enum roman_numeral {
	SQ_TK_ROMAN_I = 1,
	SQ_TK_ROMAN_V = 5,
	SQ_TK_ROMAN_X = 10,
	SQ_TK_ROMAN_L = 50,
	SQ_TK_ROMAN_C = 100,
	SQ_TK_ROMAN_D = 500,
	SQ_TK_ROMAN_M = 1000,
};

char *sq_number_to_roman(sq_number number) {
	char *buf = xmalloc(40); // todo: find an actual max size lol
	char *ret = buf;

	// todo: if number is min possible.
	if (number < 0) {
		*ret++ = '-';
		number *= -1;
	}

	while (number > 0) {
		if (number <= SQ_TK_ROMAN_I) {
			*ret++ = 'I';
			number -= SQ_TK
		}
	}
}

sq_number sq_roman_to_number(const char *input, const char **output) {
	sq_number number = 0;
	enum roman_numeral stage = SQ_TK_ROMAN_I, parsed;

	while (true) {
		switch(*input++) {
		case 'I': parsed = SQ_TK_ROMAN_I; break;
		case 'V': parsed = SQ_TK_ROMAN_V; break;
		case 'X': parsed = SQ_TK_ROMAN_X; break;
		case 'L': parsed = SQ_TK_ROMAN_L; break;
		case 'C': parsed = SQ_TK_ROMAN_C; break;
		case 'D': parsed = SQ_TK_ROMAN_D; break;
		case 'M': parsed = SQ_TK_ROMAN_M; break;
		default:
			if (isalnum(parsed))
				return -1;
			goto done;
		}

		if (stage <= parsed) {
			stage = parsed;
			number += parsed;
		} else {
			number -= parsed;
		}
	}

done:

	if (output)
		*output = input;

	return number;
}
