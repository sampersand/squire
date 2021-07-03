#include "value.h"
#include "book.h"
#include "form.h"
#include "function.h"
#include "shared.h"
#include "string.h"
#include "roman.h"
#include "codex.h"
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#define IS_STRING sq_value_is_string
#define AS_STRING sq_value_as_string
#define AS_NUMBER sq_value_as_number
#define AS_FORM sq_value_as_form
#define AS_IMITATION sq_value_as_imitation
#define AS_FUNCTION sq_value_as_function
#define AS_ARRAY sq_value_as_book
#define AS_CODEX sq_value_as_codex
#define TYPENAME sq_value_typename
#define AS_STR(c) (AS_STRING(c)->ptr)

void sq_value_dump(sq_value value) {
	sq_value_dump_to(stdout, value);
}

void sq_value_dump_to(FILE *out, sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		if (sq_value_is_null(value)) {
			fprintf(out, "Null()");
		} else {
			fprintf(out, "Boolean(%s)", sq_value_as_boolean(value) ? "true" : "false");
		}

		break;

	case SQ_TNUMBER:
		fprintf(out, "Number(%"PRId64")", AS_NUMBER(value));
		break;

	case SQ_TSTRING:
		fprintf(out, "String(%s)", AS_STR(value));
		break;

	case SQ_TFORM:
		sq_form_dump(out, AS_FORM(value));
		break;

	case SQ_TIMITATION:
		sq_imitation_dump(out, AS_IMITATION(value));
		break;

	case SQ_TFUNCTION:
		sq_function_dump(out, AS_FUNCTION(value));
		break;

	case SQ_TBOOK:
		sq_book_dump(out, AS_ARRAY(value));
		break;

	case SQ_TCODEX:
		sq_codex_dump(out, AS_CODEX(value));
		break;

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_value sq_value_clone(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		return sq_value_new_string(sq_string_clone(AS_STRING(value)));

	case SQ_TFORM:
		return sq_value_new_form(sq_form_clone(AS_FORM(value)));

	case SQ_TIMITATION:
		return sq_value_new_imitation(sq_imitation_clone(AS_IMITATION(value)));

	case SQ_TFUNCTION:
		return sq_value_new_function(sq_function_clone(AS_FUNCTION(value)));

	case SQ_TBOOK:
		return sq_value_new_book(sq_book_clone(AS_ARRAY(value)));

	case SQ_TCODEX:
		return sq_value_new_codex(sq_codex_clone(AS_CODEX(value)));

	default:
		return value;
	}
}

void sq_value_free(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		sq_string_free(AS_STRING(value));
		return;

	case SQ_TFORM:
		sq_form_free(AS_FORM(value));
		return;

	case SQ_TIMITATION:
		sq_imitation_free(AS_IMITATION(value));
		return;

	case SQ_TFUNCTION:
		sq_function_free(AS_FUNCTION(value));
		return;

	case SQ_TBOOK:
		sq_book_free(AS_ARRAY(value));
		return;

	case SQ_TCODEX:
		sq_codex_free(AS_CODEX(value));
		return;
	}
}

const char *sq_value_typename(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST: return sq_value_is_null(value) ? "ni" : "veracity";
	case SQ_TNUMBER: return "numeral";
	case SQ_TSTRING: return "string";
	case SQ_TIMITATION: return "imitation";
	case SQ_TFUNCTION: return "journey";
	case SQ_TFORM: return "form";
	case SQ_TBOOK: return "book";
	case SQ_TCODEX: return "codex";
	default: bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

sq_value sq_value_kindof(sq_value value) {
	static struct sq_string KIND_BOOLEAN = SQ_STRING_STATIC("veracity");
	static struct sq_string KIND_NULL = SQ_STRING_STATIC("ni");
	static struct sq_string KIND_NUMBER = SQ_STRING_STATIC("numeral");
	static struct sq_string KIND_STRING = SQ_STRING_STATIC("string");
	static struct sq_string KIND_FUNCTION = SQ_STRING_STATIC("journey");
	static struct sq_string KIND_form = SQ_STRING_STATIC("form");
	static struct sq_string KIND_ARRAY = SQ_STRING_STATIC("book");
	static struct sq_string KIND_CODEX = SQ_STRING_STATIC("codex");

	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return sq_value_new_string(sq_value_is_null(value) ? &KIND_NULL : &KIND_BOOLEAN);

	case SQ_TNUMBER:
		return sq_value_new_string(&KIND_NUMBER);

	case SQ_TSTRING:
		return sq_value_new_string(&KIND_STRING);

	case SQ_TIMITATION:
		return sq_value_new_form(sq_form_clone(AS_IMITATION(value)->form));

	case SQ_TFUNCTION:
		return sq_value_new_string(&KIND_FUNCTION);

	case SQ_TFORM:
		return sq_value_new_string(&KIND_form);

	case SQ_TBOOK:
		return sq_value_new_string(&KIND_ARRAY);

	case SQ_TCODEX:
		return sq_value_new_string(&KIND_CODEX);

	default:
		bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

bool sq_value_not(sq_value arg) {
	return sq_value_to_boolean(arg) == SQ_FALSE;
}

bool sq_value_eql(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TSTRING:
		return IS_STRING(rhs) && !strcmp(AS_STR(lhs), AS_STR(rhs));

	case SQ_TBOOK:
		if (!sq_value_is_book(rhs)) return false;
		struct sq_book *lary = AS_ARRAY(lhs), *rary = AS_ARRAY(rhs);

		if (lary->length != rary->length)
			return false;

		for (unsigned i = 0; i < lary->length; ++i)
			if (!sq_value_eql(lary->pages[i], rary->pages[i]))
				return false;
		return true;

	case SQ_TCODEX:
		if (!sq_value_is_codex(rhs))
			return false;

		struct sq_codex *lcodex = AS_CODEX(lhs), *rcodex = AS_CODEX(rhs);

		if (lcodex->length != rcodex->length)
			return false;

		for (unsigned i = 0; i < lcodex->length; ++i)
			if (!sq_value_eql(lcodex->pages[i].value, rcodex->pages[i].value))
				return false;

		return true;


	case SQ_TIMITATION: {
		struct sq_function *eql = sq_imitation_lookup_change(AS_IMITATION(lhs), "==");
		sq_value args[2] = { lhs, rhs };

		if (eql != NULL)
			return sq_function_run(eql, 2, args);
		// fallthrough
	}

	default:
		return lhs == rhs;
	}
}

sq_number sq_value_cmp(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		return AS_NUMBER(lhs) - sq_value_to_number(rhs);

	case SQ_TSTRING:
		// todo: free string
		return strcmp(AS_STR(lhs), sq_value_to_string(rhs)->ptr);

	case SQ_TIMITATION:
		todo("cmp imitation");

	default:
		die("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
	// 	struct sq_function *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "<=>");

	// 	if (neg != NULL) return sq_function_run(neg, 1, &arg);
	// }
	}
}

sq_value sq_value_neg(sq_value arg) {
	switch (SQ_VTAG(arg)) {
	case SQ_TNUMBER:
		return sq_value_new_number(-AS_NUMBER(arg));

	case SQ_TIMITATION: {
		struct sq_function *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "-@");

		if (neg != NULL)
			return sq_function_run(neg, 1, &arg);
		// fallthrough
	}

	default:
		die("cannot numerically negate '%s'", TYPENAME(arg));
	}
}

sq_value sq_value_index(sq_value value, sq_value key) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING: {
		int index = sq_value_to_number(key);

		if (!index--) sq_throw("cannot index by N.");
		if (index < 0)
			index += AS_STRING(value)->length;

		if (index < 0 || AS_STRING(value)->length <= (unsigned) index)
			return SQ_NULL;

		char *c = xmalloc(sizeof(char [2]));
		c[0] = AS_STR(value)[index];
		c[1] = '\0';
		return sq_value_new_string(sq_string_new2(c, 2));
	}

	case SQ_TBOOK:
		return sq_book_index2(AS_ARRAY(value), sq_value_to_number(key));

	case SQ_TCODEX:
		return sq_codex_index(AS_CODEX(value), key);

	case SQ_TIMITATION: {
		struct sq_function *index = sq_imitation_lookup_change(AS_IMITATION(value), "[]");
		sq_value args[2] = { value, key };

		if (index != NULL)
			return sq_function_run(index, 2, args);
		// fallthrough
	}

	default:
		die("cannot index into '%s'", TYPENAME(value));
	}
}


void sq_value_index_assign(sq_value value, sq_value key, sq_value val) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOK:
		sq_book_index_assign2(AS_ARRAY(value), sq_value_to_number(key), val);
		return;

	case SQ_TCODEX:
		sq_codex_index_assign(AS_CODEX(value), key, val);
		return;

	case SQ_TIMITATION: {
		struct sq_function *index_assign = sq_imitation_lookup_change(AS_IMITATION(value), "[]=");
		sq_value args[3] = { value, key, val };

		if (index_assign != NULL) {
			sq_function_run(index_assign, 2, args);
			return;
		}

		// fallthrough
	}

	default:
		die("cannot index assign into '%s'", TYPENAME(value));
	}
}

sq_value sq_value_add(sq_value lhs, sq_value rhs) {
	// bool free_rhs = false;

	if (sq_value_is_string(rhs)) {
		// free_lhs = true;
		lhs = sq_value_new_string(sq_value_to_string(lhs));
	}

	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		return sq_value_new_number(AS_NUMBER(lhs) + sq_value_to_number(rhs));

	case SQ_TSTRING: {
		struct sq_string *rstr = sq_value_to_string(rhs);
		struct sq_string *result = sq_string_alloc(AS_STRING(lhs)->length + rstr->length);

		memcpy(result->ptr, AS_STR(lhs), AS_STRING(lhs)->length);
		memcpy(result->ptr + AS_STRING(lhs)->length, rstr->ptr, AS_STRING(rhs)->length + 1);

		// sq_string_free(rstr);
		// if (free_lhs) sq_value_free(lhs);
		return sq_value_new_string(result);
	}

	case SQ_TBOOK: {
		struct sq_book *lary = AS_ARRAY(lhs), *rary = sq_value_to_book(rhs);

		unsigned length = lary->length + rary->length;
		sq_value *pages = xmalloc(sizeof(sq_value[length]));

		for (unsigned i = 0; i < lary->length; ++i)
			pages[i] = sq_value_clone(lary->pages[i]);

		for (unsigned i = 0; i < rary->length; ++i)
			pages[lary->length + i] = sq_value_clone(rary->pages[i]);

		// sq_book_free(rary);
		return sq_value_new_book(sq_book_new2(length, pages));
	}

	case SQ_TCODEX: {
		todo("'+' dicts");
		// struct sq_codex *ldict = AS_CODEX(lhs), *rdict = sq_value_to_codex(rhs);

		// unsigned i = 0, length = lhs->length + rhs->length;
		// sq_value *elements = xmalloc(sizeof(sq_value[length]));

		// for (; i < lhs->length; ++i)
		// 	elements[i] = sq_value_clone(lary->elements[i]);
		// for (; i < lhs->length; ++i)
		// 	elements[i] = sq_value_clone(rary->elements[i]);

		// sq_book_free(rhs);
		// return sq_value_new_book(lary);
	}


	case SQ_TIMITATION: {
		struct sq_function *add = sq_imitation_lookup_change(AS_IMITATION(lhs), "+");
		sq_value args[2] = { lhs, rhs };

		if (add != NULL)
			return sq_function_run(add, 2, args);
		// fallthrough
	}

	default:
		die("cannot add '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_sub(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		return sq_value_new_number(AS_NUMBER(lhs) - sq_value_to_number(rhs));

	case SQ_TBOOK:
		todo("set difference");

	case SQ_TCODEX:
		todo("set difference for dict");

	case SQ_TIMITATION: {
		struct sq_function *sub = sq_imitation_lookup_change(AS_IMITATION(lhs), "-");
		sq_value args[2] = { lhs, rhs };

		if (sub != NULL)
			return sq_function_run(sub, 2, args);
		// fallthrough
	}

	default:
		die("cannot subtract '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mul(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		return sq_value_new_number(AS_NUMBER(lhs) * sq_value_to_number(rhs));

	case SQ_TSTRING: {
		sq_number amnt = sq_value_to_number(rhs);
		if (amnt == 0 || AS_STRING(lhs)->length == 0)
			return sq_value_new_string(&sq_string_empty);
		if (amnt < 0 || amnt >= UINT_MAX || (amnt * AS_STRING(lhs)->length) >= UINT_MAX)
			sq_throw("string multiplication by %"PRId64" is out of range", amnt);
		if (amnt == 1)
			return sq_value_new_string(sq_string_clone(AS_STRING(lhs)));

		struct sq_string *result = sq_string_alloc(AS_STRING(lhs)->length * amnt);
		char *ptr = result->ptr;

		for (unsigned i = 0; i < amnt; ++i) {
			memcpy(ptr, AS_STR(lhs), AS_STRING(lhs)->length + 1);
			ptr += AS_STRING(lhs)->length;
		}

		return sq_value_new_string(result);
	}

	case SQ_TBOOK:
		todo("book multiply");

	case SQ_TIMITATION: {
		struct sq_function *mul = sq_imitation_lookup_change(AS_IMITATION(lhs), "*");
		sq_value args[2] = { lhs, rhs };

		if (mul != NULL)
			return sq_function_run(mul, 2, args);
		// fallthrough
	}

	default:
		die("cannot multiply '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

sq_value sq_value_div(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER: {
		sq_number rnum = sq_value_to_number(rhs);
		if (!rnum) die("cannot divide by N");
		return sq_value_new_number(AS_NUMBER(lhs) / rnum);
	}

	case SQ_TIMITATION: {
		struct sq_function *div = sq_imitation_lookup_change(AS_IMITATION(lhs), "/");
		sq_value args[2] = { lhs, rhs };

		if (div != NULL)
			return sq_function_run(div, 2, args);

		// fallthrough
	}

	default:
		die("cannot divide '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mod(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER: {
		sq_number rnum = sq_value_to_number(rhs);
		if (!rnum) die("cannot modulo by N");
		return sq_value_new_number(AS_NUMBER(lhs) % rnum);
	}

	case SQ_TIMITATION: {
		struct sq_function *mod = sq_imitation_lookup_change(AS_IMITATION(lhs), "%");
		sq_value args[2] = { lhs, rhs };

		if (mod != NULL)
			return sq_function_run(mod, 2, args);

		// fallthrough
	}

	default:
		die("cannot modulo '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

struct sq_string *sq_value_to_string(sq_value value) {
	static struct sq_string truestring = SQ_STRING_STATIC("yay");
	static struct sq_string falsestring = SQ_STRING_STATIC("nay");
	static struct sq_string nullstring = SQ_STRING_STATIC("ni");

	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		if (sq_value_is_null(value))
			return &nullstring;
		else
			return value == SQ_TRUE ? &truestring : &falsestring;

	case SQ_TNUMBER: {
		char *buf;
#ifdef SQ_NUMBER_TO_ROMAN
		buf = sq_number_to_roman(AS_NUMBER(value));
#else
		buf = sq_number_to_arabic(AS_NUMBER(value));
#endif
		return sq_string_new(buf);
	}

	case SQ_TSTRING:
		sq_string_clone(AS_STRING(value));
		return AS_STRING(value);

	case SQ_TFORM:
		return sq_string_new(strdup(AS_FORM(value)->name));

	case SQ_TIMITATION: {
		struct sq_function *to_Text = sq_imitation_lookup_change(AS_IMITATION(value), "to_text");

		if (to_Text != NULL) {
			sq_value string = sq_function_run(to_Text, 1, &value);
			if (!sq_value_is_string(string))
				die("to_Text for an imitation of '%s' didn't return a text", AS_IMITATION(value)->form->name);
			return AS_STRING(string);
		}
		// else fallthrough
	}

	case SQ_TBOOK:
		return sq_book_to_string(AS_ARRAY(value));

	case SQ_TCODEX:
		todo("codex to string");

	case SQ_TFUNCTION:
		die("cannot convert %s to a string", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_number sq_value_to_number(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return value == SQ_TRUE ? 1 : 0;

	case SQ_TNUMBER:
		return AS_NUMBER(value);

	case SQ_TSTRING:
		return strtoll(AS_STR(value), NULL, 10);

	case SQ_TIMITATION: {
		struct sq_function *to_numeral = sq_imitation_lookup_change(AS_IMITATION(value), "to_numeral");

		if (to_numeral != NULL) {
			sq_value number = sq_function_run(to_numeral, 1, &value);
			if (!sq_value_is_number(number))
				die("to_numeral for an imitation of '%s' didn't return a number", AS_IMITATION(value)->form->name);
			return AS_NUMBER(number);
		}
		// else fallthrough
	}

	case SQ_TBOOK:
		return sq_value_new_string(sq_book_to_string(AS_ARRAY(value)));

	case SQ_TFORM:
	case SQ_TFUNCTION:
	case SQ_TCODEX:
		die("cannot convert %s to a number", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

bool sq_value_to_boolean(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return value == SQ_TRUE;

	case SQ_TNUMBER:
		return AS_NUMBER(value) ? SQ_TRUE : SQ_FALSE;

	case SQ_TSTRING:
		return *AS_STR(value) ? SQ_TRUE : SQ_FALSE;

	case SQ_TBOOK:
		return AS_ARRAY(value)->length;

	case SQ_TCODEX:
		return AS_CODEX(value)->length;

	case SQ_TIMITATION: {
		struct sq_function *to_veracity = sq_imitation_lookup_change(AS_IMITATION(value), "to_veracity");

		if (to_veracity != NULL) {
			sq_value boolean = sq_function_run(to_veracity, 1, &value);
			if (!sq_value_is_boolean(boolean))
				die("to_veracity for an imitation of '%s' didn't return a boolean", AS_IMITATION(value)->form->name);
			return sq_value_as_boolean(boolean);
		}
		// else fallthrough
	}

	case SQ_TFORM:
	case SQ_TFUNCTION:
		die("cannot convert %s to a boolean", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

size_t sq_value_length(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOK:
		return sq_value_as_book(value)->length;

	case SQ_TCODEX:
		return sq_value_as_codex(value)->length;

	case SQ_TSTRING:
		return AS_STRING(value)->length;

	case SQ_TIMITATION: {
		struct sq_function *length = sq_imitation_lookup_change(AS_IMITATION(value), "length");

		if (length != NULL) {
			sq_value boolean = sq_function_run(length, 1, &value);
			if (!sq_value_is_number(boolean))
				die("length for an imitation of '%s' didn't return a boolean", AS_IMITATION(value)->form->name);
			return AS_NUMBER(boolean);
		}
		// else fallthrough
	}

	case SQ_TCONST:
	case SQ_TNUMBER:
	case SQ_TFORM:
	case SQ_TFUNCTION:
		die("cannot get length of %s", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

struct sq_book *sq_value_to_book(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOK:
		++AS_ARRAY(value)->refcount;
		return AS_ARRAY(value);
	default:
		todo("others to book");
	}
}

struct sq_codex *sq_value_to_codex(sq_value value) {
	(void) value;
	die("todo");
}
