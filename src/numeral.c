#include "numeral.h"
#include "shared.h"
#include "text.h"
#include <ctype.h>
#include <string.h>

enum roman_numeral {
	SQ_TK_ROMAN_I = 1,
	SQ_TK_ROMAN_V = 5,
	SQ_TK_ROMAN_X = 10,
	SQ_TK_ROMAN_L = 50,
	SQ_TK_ROMAN_C = 100,
	SQ_TK_ROMAN_D = 500,
	SQ_TK_ROMAN_M = 1000,
};

static void convert(unsigned numeral, char one, char five, char ten, char **out) {
	if (5 < numeral && numeral <= 8) *(*out)++ = five;

	switch (numeral) {
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
	default: bug("uh oh, numeral %d is out of bounds", numeral);
	}
}


#ifdef SQ_NUMERAL_TO_ARABIC
struct sq_text sq_text_zero = SQ_TEXT_STATIC("0"); 
#else
struct sq_text sq_text_zero = SQ_TEXT_STATIC("N"); 
#endif /* SQ_NUMERAL_TO_ARABIC */

// lol this is so bad.
struct sq_text *sq_numeral_to_roman(sq_numeral numeral) {
	if (!numeral)
		return &sq_text_zero;

	struct sq_text *buf = sq_text_allocate(40); // todo: find an actual max size lol
	char *ret = buf->ptr;

	// todo: if numeral is min possible.
	if (numeral < 0) {
		*ret++ = '-';
		numeral *= -1;
	}

	while (numeral > 0) {
		if (numeral <= 10) {
			convert(numeral, 'I', 'V', 'X', &ret);
			break;
		} else if (numeral <= 100) {
			convert(numeral / 10, 'X', 'L', 'C', &ret);
			numeral %= 10;
		} else if (numeral <= 1000) {
			convert(numeral / 100, 'C', 'D', 'M', &ret);
			numeral %= 100;
		} else {
			*ret++ = 'M';
			numeral -= SQ_TK_ROMAN_M;
		}
	}

	*ret = '\0';
	buf->length = ret - buf->ptr;

	return buf;
}

// note that this returns an owned text.
struct sq_text *sq_numeral_to_arabic(sq_numeral numeral) {
	if (!numeral)
		return &sq_text_zero;

	struct sq_text *buf = sq_text_allocate(40);
	snprintf(buf->ptr, 40, "%"PRId64, numeral);
	buf->length = strlen(buf->ptr);

	return buf;
}

sq_numeral sq_roman_to_numeral(const char *input, const char **output) {
	sq_numeral numeral = 0;
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


		numeral += parsed;

		if (stage == 0 || parsed <= stage) stage = parsed;
		else numeral -= stage * 2;

		++input;
	}

done:

	if (output)
		*output = input;

	return numeral;
}
