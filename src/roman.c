 #include "roman.h"
#include "shared.h"
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

enum roman_numeral {
	SQ_TK_ROMAN_I = 1,
	SQ_TK_ROMAN_V = 5,
	SQ_TK_ROMAN_X = 10,
	SQ_TK_ROMAN_L = 50,
	SQ_TK_ROMAN_C = 100,
	SQ_TK_ROMAN_D = 500,
	SQ_TK_ROMAN_M = 1000,
};

static void convert(unsigned number, char one, char five, char ten, char **out) {
	if (5 < number && number <= 8) *(*out)++ = five;

	switch (number) {
	case 3: *(*out)++ = one;
	case 2: *(*out)++ = one;
	case 1: *(*out)++ = one; break;

	case 4: *(*out)++ = one;
	case 5: *(*out)++ = five; break;

	case 8: *(*out)++ = one;
	case 7: *(*out)++ = one;
	case 6: *(*out)++ = one; break;

	case 9: *(*out)++ = one;
	case 10: *(*out)++ = ten; break;
	default: die("uh oh, number %d is out of bounds", number);
	}
}

// lol this is so bad.
char *sq_number_to_roman(sq_number number) {
	if (number == 0)
		return strdup("N");

	char *buf = xmalloc(40); // todo: find an actual max size lol
	char *ret = buf;

	// todo: if number is min possible.
	if (number < 0) {
		*ret++ = '-';
		number *= -1;
	}

	while (number > 0) {
		if (number <= 10) {
			convert(number, 'I', 'V', 'X', &ret);
			break;
		} else if (number <= 100) {
			convert(number / 10, 'X', 'L', 'C', &ret);
			number %= 10;
		} else if (number <= 1000) {
			convert(number / 100, 'C', 'D', 'M', &ret);
			number %= 100;
		} else {
			*ret++ = 'M';
			number -= SQ_TK_ROMAN_M;
		}
	}
	*ret = '\0';

	return buf;
}

// note that this returns an owned string.
char *sq_number_to_arabic(sq_number number) {
	char *buf = xmalloc(40);
	snprintf(buf, 40, "%"PRId64, number);
	return buf;
}

sq_number sq_roman_to_number(const char *input, const char **output) {
	sq_number number = 0;
	enum roman_numeral stage = 0, parsed;

	// ie if the input is just `N` (ie `0`).
	if (*input == 'N' && !isalnum(input[1])) {
		input++;
		goto done;
	}

	while (true) {
		switch(*input) {
		case 'I': parsed = SQ_TK_ROMAN_I; break;
		case 'V': parsed = SQ_TK_ROMAN_V; break;
		case 'X': parsed = SQ_TK_ROMAN_X; break;
		case 'L': parsed = SQ_TK_ROMAN_L; break;
		case 'C': parsed = SQ_TK_ROMAN_C; break;
		case 'D': parsed = SQ_TK_ROMAN_D; break;
		case 'M': parsed = SQ_TK_ROMAN_M; break;
		case '_': continue; // ignore `_` in roman numeral literals
		default:
			// followed by any other alphanumerics, we aren't a roman numeral.
			if (isalnum(*input)) return -1;
			goto done;
		}


		number += parsed;

		if (stage == 0 || parsed <= stage) stage = parsed;
		else number -= stage * 2;

		++input;
	}

done:

	if (output)
		*output = input;

	return number;
}
