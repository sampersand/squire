#include "value.h"
#include "book.h"
#include "form.h"
#include "journey.h"
#include "shared.h"
#include "text.h"
#include "codex.h"
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#define IS_STRING sq_value_is_text
#define AS_STRING sq_value_as_text
#define AS_NUMBER sq_value_as_numeral
#define AS_FORM sq_value_as_form
#define AS_IMITATION sq_value_as_imitation
#define AS_JOURNEY sq_value_as_function
#define AS_BOOK sq_value_as_book
#define AS_CODEX sq_value_as_codex
#define TYPENAME sq_value_typename
#define AS_STR(c) (AS_STRING(c)->ptr)

void sq_value_dump(sq_value value) {
	sq_value_dump_to(stdout, value);
}

void sq_value_dump_to(FILE *out, sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		if (value == SQ_NI)
			fprintf(out, "Ni()");
		else
			fprintf(out, "Veracity(%s)", sq_value_as_veracity(value) ? "yay" : "nay");

		break;

	case SQ_TNUMERAL:
		fprintf(out, "Numeral(%"PRId64")", AS_NUMBER(value));
		break;

	case SQ_TTEXT:
		fprintf(out, "Text(%s)", AS_STR(value));
		break;

	case SQ_TFORM:
		sq_form_dump(out, AS_FORM(value));
		break;

	case SQ_TIMITATION:
		sq_imitation_dump(out, AS_IMITATION(value));
		break;

	case SQ_TFUNCTION:
		sq_journey_dump(out, AS_JOURNEY(value));
		break;

	case SQ_TBOOK:
		sq_book_dump(out, AS_BOOK(value));
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
	case SQ_TTEXT:
		return sq_value_new(sq_text_clone(AS_STRING(value)));

	case SQ_TFORM:
		return sq_value_new(sq_form_clone(AS_FORM(value)));

	case SQ_TIMITATION:
		return sq_value_new(sq_imitation_clone(AS_IMITATION(value)));

	case SQ_TFUNCTION:
		return sq_value_new(sq_journey_clone(AS_JOURNEY(value)));

	case SQ_TBOOK:
		return sq_value_new(sq_book_clone(AS_BOOK(value)));

	case SQ_TCODEX:
		return sq_value_new(sq_codex_clone(AS_CODEX(value)));

	default:
		return value;
	}
}

void sq_value_free(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TTEXT:
		sq_text_free(AS_STRING(value));
		return;

	case SQ_TFORM:
		sq_form_free(AS_FORM(value));
		return;

	case SQ_TIMITATION:
		sq_imitation_free(AS_IMITATION(value));
		return;

	case SQ_TFUNCTION:
		sq_journey_free(AS_JOURNEY(value));
		return;

	case SQ_TBOOK:
		sq_book_free(AS_BOOK(value));
		return;

	case SQ_TCODEX:
		sq_codex_free(AS_CODEX(value));
		return;
	}
}

const char *sq_value_typename(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST: return value == SQ_NI ? "ni" : "veracity";
	case SQ_TNUMERAL: return "numeral";
	case SQ_TTEXT: return "text";
	case SQ_TIMITATION: return "imitation";
	case SQ_TFUNCTION: return "journey";
	case SQ_TFORM: return "form";
	case SQ_TBOOK: return "book";
	case SQ_TCODEX: return "codex";
	default: bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

sq_value sq_value_kindof(sq_value value) {
	static struct sq_text KIND_VERACITY = SQ_TEXT_STATIC("veracity");
	static struct sq_text KIND_NI = SQ_TEXT_STATIC("ni");
	static struct sq_text KIND_NUMERAL = SQ_TEXT_STATIC("numeral");
	static struct sq_text KIND_TEXT = SQ_TEXT_STATIC("text");
	static struct sq_text KIND_FUNCTION = SQ_TEXT_STATIC("journey");
	static struct sq_text KIND_FORM = SQ_TEXT_STATIC("form");
	static struct sq_text KIND_ARRAY = SQ_TEXT_STATIC("book");
	static struct sq_text KIND_CODEX = SQ_TEXT_STATIC("codex");

	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return sq_value_new(value == SQ_NI ? &KIND_NI : &KIND_VERACITY);

	case SQ_TNUMERAL:
		return sq_value_new(&KIND_NUMERAL);

	case SQ_TTEXT:
		return sq_value_new(&KIND_TEXT);

	case SQ_TIMITATION:
		return sq_value_new(sq_form_clone(AS_IMITATION(value)->form));

	case SQ_TFUNCTION:
		return sq_value_new(&KIND_FUNCTION);

	case SQ_TFORM:
		return sq_value_new(&KIND_FORM);

	case SQ_TBOOK:
		return sq_value_new(&KIND_ARRAY);

	case SQ_TCODEX:
		return sq_value_new(&KIND_CODEX);

	default:
		bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

bool sq_value_not(sq_value arg) {
	return sq_value_to_veracity(arg) == SQ_NAY;
}

bool sq_value_eql(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TTEXT:
		return IS_STRING(rhs) && !strcmp(AS_STR(lhs), AS_STR(rhs));

	case SQ_TBOOK:
		if (!sq_value_is_book(rhs)) return false;
		struct sq_book *lary = AS_BOOK(lhs), *rary = AS_BOOK(rhs);

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
		struct sq_journey *eql = sq_imitation_lookup_change(AS_IMITATION(lhs), "==");
		sq_value args[2] = { lhs, rhs };

		if (eql != NULL)
			return sq_journey_run(eql, 2, args);
		// fallthrough
	}

	default:
		return lhs == rhs;
	}
}

sq_numeral sq_value_cmp(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL:
		return AS_NUMBER(lhs) - sq_value_to_numeral(rhs);

	case SQ_TTEXT:
		// todo: free text
		return strcmp(AS_STR(lhs), sq_value_to_text(rhs)->ptr);

	case SQ_TIMITATION:
		todo("cmp imitation");

	default:
		die("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
	// 	struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "<=>");

	// 	if (neg != NULL) return sq_journey_run(neg, 1, &arg);
	// }
	}
}

sq_value sq_value_neg(sq_value arg) {
	switch (SQ_VTAG(arg)) {
	case SQ_TNUMERAL:
		return sq_value_new(-AS_NUMBER(arg));

	case SQ_TIMITATION: {
		struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "-@");

		if (neg != NULL)
			return sq_journey_run(neg, 1, &arg);
		// fallthrough
	}

	default:
		die("cannot numerically negate '%s'", TYPENAME(arg));
	}
}

sq_value sq_value_index(sq_value value, sq_value key) {
	switch (SQ_VTAG(value)) {
	case SQ_TTEXT: {
		int index = sq_value_to_numeral(key);

		if (!index--) sq_throw("cannot index by N.");
		if (index < 0)
			index += AS_STRING(value)->length;

		if (index < 0 || AS_STRING(value)->length <= (unsigned) index)
			return SQ_NI;

		char *c = xmalloc(sizeof(char [2]));
		c[0] = AS_STR(value)[index];
		c[1] = '\0';
		return sq_value_new(sq_text_new2(c, 2));
	}

	case SQ_TBOOK:
		return sq_book_index2(AS_BOOK(value), sq_value_to_numeral(key));

	case SQ_TCODEX:
		return sq_codex_index(AS_CODEX(value), key);

	case SQ_TIMITATION: {
		struct sq_journey *index = sq_imitation_lookup_change(AS_IMITATION(value), "[]");
		sq_value args[2] = { value, key };

		if (index != NULL)
			return sq_journey_run(index, 2, args);
		// fallthrough
	}

	default:
		die("cannot index into '%s'", TYPENAME(value));
	}
}


void sq_value_index_assign(sq_value value, sq_value key, sq_value val) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOK:
		sq_book_index_assign2(AS_BOOK(value), sq_value_to_numeral(key), val);
		return;

	case SQ_TCODEX:
		sq_codex_index_assign(AS_CODEX(value), key, val);
		return;

	case SQ_TIMITATION: {
		struct sq_journey *index_assign = sq_imitation_lookup_change(AS_IMITATION(value), "[]=");
		sq_value args[3] = { value, key, val };

		if (index_assign != NULL) {
			sq_journey_run(index_assign, 2, args);
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

	if (sq_value_is_text(rhs)) {
		// free_lhs = true;
		lhs = sq_value_new(sq_value_to_text(lhs));
	}

	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL:
		return sq_value_new(AS_NUMBER(lhs) + sq_value_to_numeral(rhs));

	case SQ_TTEXT: {
		struct sq_text *rstr = sq_value_to_text(rhs);
		struct sq_text *result = sq_text_allocate(AS_STRING(lhs)->length + rstr->length);

		memcpy(result->ptr, AS_STR(lhs), AS_STRING(lhs)->length);
		memcpy(result->ptr + AS_STRING(lhs)->length, rstr->ptr, AS_STRING(rhs)->length + 1);

		// sq_text_free(rstr);
		// if (free_lhs) sq_value_free(lhs);
		return sq_value_new(result);
	}

	case SQ_TBOOK: {
		if (sq_value_is_function(rhs))
			return sq_value_new(sq_book_select(AS_BOOK(lhs), AS_JOURNEY(rhs)));

		struct sq_book *lary = AS_BOOK(lhs), *rary = sq_value_to_book(rhs);

		unsigned length = lary->length + rary->length;
		sq_value *pages = xmalloc(sizeof(sq_value[length]));

		for (unsigned i = 0; i < lary->length; ++i)
			pages[i] = sq_value_clone(lary->pages[i]);

		for (unsigned i = 0; i < rary->length; ++i)
			pages[lary->length + i] = sq_value_clone(rary->pages[i]);

		// sq_book_free(rary);
		return sq_value_new(sq_book_new2(length, pages));
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
		// return sq_value_new(lary);
	}


	case SQ_TIMITATION: {
		struct sq_journey *add = sq_imitation_lookup_change(AS_IMITATION(lhs), "+");
		sq_value args[2] = { lhs, rhs };

		if (add != NULL)
			return sq_journey_run(add, 2, args);
		// fallthrough
	}

	default:
		die("cannot add '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_sub(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL:
		return sq_value_new(AS_NUMBER(lhs) - sq_value_to_numeral(rhs));

	case SQ_TBOOK:
		todo("set difference");

	case SQ_TCODEX:
		todo("set difference for dict");

	case SQ_TIMITATION: {
		struct sq_journey *sub = sq_imitation_lookup_change(AS_IMITATION(lhs), "-");
		sq_value args[2] = { lhs, rhs };

		if (sub != NULL)
			return sq_journey_run(sub, 2, args);
		// fallthrough
	}

	default:
		die("cannot subtract '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mul(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL:
		return sq_value_new(AS_NUMBER(lhs) * sq_value_to_numeral(rhs));

	case SQ_TTEXT: {
		sq_numeral amnt = sq_value_to_numeral(rhs);
		if (amnt == 0 || AS_STRING(lhs)->length == 0)
			return sq_value_new(&sq_text_empty);
		if (amnt < 0 || amnt >= UINT_MAX || (amnt * AS_STRING(lhs)->length) >= UINT_MAX)
			sq_throw("text multiplication by %"PRId64" is out of range", amnt);
		if (amnt == 1)
			return sq_value_new(sq_text_clone(AS_STRING(lhs)));

		struct sq_text *result = sq_text_allocate(AS_STRING(lhs)->length * amnt);
		char *ptr = result->ptr;

		for (unsigned i = 0; i < amnt; ++i) {
			memcpy(ptr, AS_STR(lhs), AS_STRING(lhs)->length + 1);
			ptr += AS_STRING(lhs)->length;
		}

		return sq_value_new(result);
	}

	case SQ_TBOOK:;
		struct sq_book *book = AS_BOOK(lhs);

		if (sq_value_is_numeral(rhs)) {
			sq_numeral num = AS_NUMBER(rhs);
			if (num < 0) sq_throw("cannot repeat by %"PRId64" is out of range", num);
			return sq_value_new(sq_book_repeat(book, num));
		}

		if (sq_value_is_text(rhs))
			return sq_value_new(sq_book_join(book, AS_STRING(rhs)));

		if (sq_value_is_book(rhs))
			return sq_value_new(sq_book_product(book, AS_BOOK(rhs)));
		if (sq_value_is_function(rhs))
			return sq_value_new(sq_book_map(book, AS_JOURNEY(rhs)));

		goto error;

	case SQ_TIMITATION: {
		struct sq_journey *mul = sq_imitation_lookup_change(AS_IMITATION(lhs), "*");
		sq_value args[2] = { lhs, rhs };

		if (mul != NULL)
			return sq_journey_run(mul, 2, args);
		// fallthrough
	}

	default:
	error:
		die("cannot multiply '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

sq_value sq_value_div(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) die("cannot divide by N");
		return sq_value_new(AS_NUMBER(lhs) / rnum);
	}

	case SQ_TIMITATION: {
		struct sq_journey *div = sq_imitation_lookup_change(AS_IMITATION(lhs), "/");
		sq_value args[2] = { lhs, rhs };

		if (div != NULL)
			return sq_journey_run(div, 2, args);

		// fallthrough
	}

	default:
		die("cannot divide '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mod(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) die("cannot modulo by N");
		return sq_value_new(AS_NUMBER(lhs) % rnum);
	}

	case SQ_TBOOK:;
		struct sq_book *book = AS_BOOK(lhs);

		if (sq_value_is_function(rhs))
			return sq_book_reduce(book, AS_JOURNEY(rhs));

		goto error;

	case SQ_TIMITATION: {
		struct sq_journey *mod = sq_imitation_lookup_change(AS_IMITATION(lhs), "%");
		sq_value args[2] = { lhs, rhs };

		if (mod != NULL)
			return sq_journey_run(mod, 2, args);

		// fallthrough
	}

	default:
	error:
		die("cannot modulo '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

struct sq_text *sq_value_to_text(sq_value value) {
	static struct sq_text yay_string = SQ_TEXT_STATIC("yay");
	static struct sq_text nay_string = SQ_TEXT_STATIC("nay");
	static struct sq_text ni_string = SQ_TEXT_STATIC("ni");

	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		if (value == SQ_NI)
			return &ni_string;
		else
			return value == SQ_YAY ? &yay_string : &nay_string;

	case SQ_TNUMERAL:
		return sq_numeral_to_text(AS_NUMBER(value));

	case SQ_TTEXT:
		sq_text_clone(AS_STRING(value));
		return AS_STRING(value);

	case SQ_TFORM:
		return sq_text_new(strdup(AS_FORM(value)->name));

	case SQ_TBOOK:
		return sq_book_to_text(AS_BOOK(value));

	case SQ_TCODEX:
		return sq_codex_to_text(AS_CODEX(value));

	case SQ_TIMITATION: {
		struct sq_journey *to_text = sq_imitation_lookup_change(AS_IMITATION(value), "to_text");

		if (to_text != NULL) {
			sq_value text = sq_journey_run(to_text, 1, &value);
			if (!sq_value_is_text(text))
				die("to_text for an imitation of '%s' didn't return a text", AS_IMITATION(value)->form->name);
			return AS_STRING(text);
		}
		// else fallthrough
	}

	case SQ_TFUNCTION:
		die("cannot convert %s to a text", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_numeral sq_value_to_numeral(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return value == SQ_YAY ? 1 : 0;

	case SQ_TNUMERAL:
		return AS_NUMBER(value);

	case SQ_TTEXT:
		return strtoll(AS_STR(value), NULL, 10);

	case SQ_TBOOK:
		return sq_value_new(sq_book_to_text(AS_BOOK(value)));

	case SQ_TIMITATION: {
		struct sq_journey *to_numeral = sq_imitation_lookup_change(AS_IMITATION(value), "to_numeral");

		if (to_numeral != NULL) {
			sq_value numeral = sq_journey_run(to_numeral, 1, &value);
			if (!sq_value_is_numeral(numeral))
				die("to_numeral for an imitation of '%s' didn't return a numeral", AS_IMITATION(value)->form->name);
			return AS_NUMBER(numeral);
		}
		// else fallthrough
	}

	case SQ_TFORM:
	case SQ_TFUNCTION:
	case SQ_TCODEX:
		die("cannot convert %s to a numeral", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

bool sq_value_to_veracity(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return value == SQ_YAY;

	case SQ_TNUMERAL:
		return AS_NUMBER(value) ? SQ_YAY : SQ_NAY;

	case SQ_TTEXT:
		return *AS_STR(value) ? SQ_YAY : SQ_NAY;

	case SQ_TBOOK:
		return AS_BOOK(value)->length;

	case SQ_TCODEX:
		return AS_CODEX(value)->length;

	case SQ_TIMITATION: {
		struct sq_journey *to_veracity = sq_imitation_lookup_change(AS_IMITATION(value), "to_veracity");

		if (to_veracity != NULL) {
			sq_value veracity = sq_journey_run(to_veracity, 1, &value);
			if (!sq_value_is_veracity(veracity))
				die("to_veracity for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->name);
			return sq_value_as_veracity(veracity);
		}
		// else fallthrough
	}

	case SQ_TFORM:
	case SQ_TFUNCTION:
		die("cannot convert %s to a veracity", TYPENAME(value));

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

	case SQ_TTEXT:
		return AS_STRING(value)->length;

	case SQ_TIMITATION: {
		struct sq_journey *length = sq_imitation_lookup_change(AS_IMITATION(value), "length");

		if (length != NULL) {
			sq_value veracity = sq_journey_run(length, 1, &value);
			if (!sq_value_is_numeral(veracity))
				die("length for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->name);
			return AS_NUMBER(veracity);
		}
		// else fallthrough
	}

	case SQ_TCONST:
	case SQ_TNUMERAL:
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
		++AS_BOOK(value)->refcount;
		return AS_BOOK(value);
	default:
		todo("others to book");
	}
}

struct sq_codex *sq_value_to_codex(sq_value value) {
	(void) value;
	die("todo");
}
