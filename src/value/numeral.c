#include <squire/numeral.h>
#include <squire/shared.h>
#include <squire/text.h>

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


struct sq_text sq_text_zero_numeral = SQ_TEXT_STATIC("N");
struct sq_text sq_text_zero_arabic = SQ_TEXT_STATIC("0");

// lol this is so bad.
struct sq_text *sq_numeral_to_roman(sq_numeral numeral) {
	if (!numeral)
		return &sq_text_zero_numeral;

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
		return &sq_text_zero_arabic;

	struct sq_text *buf = sq_text_allocate(40);
	snprintf(buf->ptr, 40, "%"PRId64, numeral);
	buf->length = strlen(buf->ptr);

	return buf;
}


static sq_numeral unicode_roman(const uint8_t *input, const char **output) {
	assert(input[0] == 0xE2);
	sq_numeral result;

	if (input[1] == 0x85) {
		assert(0xA0 <= input[2] && input[2] <= 0xBF);
		uint8_t which = (input[2] - 0xA0) % 16; // % 16 because the `B` range is lowercase.

		switch (which) {
		case 0xC: result = 50; break;   /* L */
		case 0xD: result = 100; break;  /* C */
		case 0xE: result = 500; break;  /* D */
		case 0xF: result = 1000; break; /* M */
		default: result = which + 1; break; // all else
		}
	} else {
		assert(input[1] == 0x86);
		assert(0x80 <= input[2] && input[2] <= 0x88);

		switch (input[2]) {
		case 0: result = 1000; break;
		case 1: result = 5000; break;
		case 2: result = 10000; break;
		case 3: result = 100; break;
		case 4: result = 100; break;
		case 5: result = 6; break;
		case 6: result = 50; break;
		case 7: result = 50000; break;
		case 8: result = 100000; break;
		default:
			bug("bad input: %d", input[2]);
		}
	}

	if (output)
		*output = (const char *) (input + 3);

	return result;
}

sq_numeral sq_roman_to_numeral(const char *input, const char **output) {
	if (strpbrk(input, "NIVXLCDM") == NULL)
		return unicode_roman((const uint8_t *) input, output);

	sq_numeral numeral = 0;
	enum roman_numeral stage = 0, parsed;

	// ie if the input is just `N` (ie `0`).
	if (input[0] == 'N' && !isalnum(input[1])) {
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

bool sq_numeral_starts(const char *text) {
	if (strpbrk(text, "NIVXLCDM") == text) return true;
	const uint8_t *utext = (const uint8_t *) text;
	if (utext[0] != 0xE2) return false;

	return (utext[1] == 0x85 && (0xa0 <= utext[2] && utext[2] <= 0xbf))
	    || (utext[1] == 0x86 && (0x80 <= utext[2] && utext[2] <= 0x88));
}
