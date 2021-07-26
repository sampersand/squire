#include <squire/value.h>
#include <squire/book.h>
#include <squire/form.h>
#include <squire/journey.h>
#include <squire/shared.h>
#include <squire/text.h>
#include <squire/codex.h>
#include <squire/other/other.h>

#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>

#define AS_TEXT sq_value_as_text
#define AS_NUMBER sq_value_as_numeral
#define AS_FORM sq_value_as_form
#define AS_IMITATION sq_value_as_imitation
#define AS_JOURNEY sq_value_as_function
#define AS_BOOK sq_value_as_book
#define AS_CODEX sq_value_as_codex
#define AS_OTHER sq_value_as_other
#define TYPENAME sq_value_typename
#define AS_STR(c) (AS_TEXT(c)->ptr)

void sq_value_dump(sq_value value) {
	sq_value_dump_to(stdout, value);
}

void sq_value_dump_to(FILE *out, sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (value == SQ_NI)
			fprintf(out, "Ni()");
		else if (sq_value_is_veracity(value))
			fprintf(out, "Veracity(%s)", sq_value_as_veracity(value) ? "yay" : "nay");
		else
			sq_other_dump(out, AS_OTHER(value));

		break;

	case SQ_G_NUMERAL:
		fprintf(out, "Numeral(%"PRId64")", AS_NUMBER(value));
		break;

	case SQ_G_TEXT:
		fprintf(out, "Text(%s)", AS_STR(value));
		break;

	case SQ_G_FORM:
		sq_form_dump(out, AS_FORM(value));
		break;

	case SQ_G_IMITATION:
		sq_imitation_dump(out, AS_IMITATION(value));
		break;

	case SQ_G_JOURNEY:
		sq_journey_dump(out, AS_JOURNEY(value));
		break;

	case SQ_G_BOOK:
		sq_book_dump(out, AS_BOOK(value));
		break;

	case SQ_G_CODEX:
		sq_codex_dump(out, AS_CODEX(value));
		break;

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_value sq_value_clone(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT:
		return sq_value_new(sq_text_clone(AS_TEXT(value)));

	case SQ_G_FORM:
		return sq_value_new(sq_form_clone(AS_FORM(value)));

	case SQ_G_IMITATION:
		return sq_value_new(sq_imitation_clone(AS_IMITATION(value)));

	case SQ_G_JOURNEY:
		return sq_value_new(sq_journey_clone(AS_JOURNEY(value)));

	case SQ_G_BOOK:
		return sq_value_new(sq_book_clone(AS_BOOK(value)));

	case SQ_G_CODEX:
		return sq_value_new(sq_codex_clone(AS_CODEX(value)));

	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_value_new(sq_other_clone(AS_OTHER(value)));

	default:
		return value;
	}
}

void sq_value_free(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT:
		sq_text_free(AS_TEXT(value));
		return;

	case SQ_G_FORM:
		sq_form_free(AS_FORM(value));
		return;

	case SQ_G_IMITATION:
		sq_imitation_free(AS_IMITATION(value));
		return;

	case SQ_G_JOURNEY:
		sq_journey_free(AS_JOURNEY(value));
		return;

	case SQ_G_BOOK:
		sq_book_free(AS_BOOK(value));
		return;

	case SQ_G_CODEX:
		sq_codex_free(AS_CODEX(value));
		return;

	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			sq_other_free(AS_OTHER(value));

		return;
	}
}

const char *sq_value_typename(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_typename(AS_OTHER(value));
		else
			return value == SQ_NI ? "Ni" : "Veracity";

	case SQ_G_NUMERAL: return "Numeral";
	case SQ_G_TEXT: return "Text";
	case SQ_G_IMITATION: return "Imitation";
	case SQ_G_JOURNEY: return "Journey";
	case SQ_G_FORM: return "Form";
	case SQ_G_BOOK: return "Book";
	case SQ_G_CODEX: return "Codex";
	default: bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

sq_value sq_value_genus(sq_value value) {
	static struct sq_text KIND_VERACITY = SQ_TEXT_STATIC("Veracity");
	static struct sq_text KIND_NI = SQ_TEXT_STATIC("Ni");
	static struct sq_text KIND_NUMERAL = SQ_TEXT_STATIC("Numeral");
	static struct sq_text KIND_TEXT = SQ_TEXT_STATIC("Text");
	static struct sq_text KIND_FUNCTION = SQ_TEXT_STATIC("Journey");
	static struct sq_text KIND_FORM = SQ_TEXT_STATIC("Form");
	static struct sq_text KIND_ARRAY = SQ_TEXT_STATIC("Book");
	static struct sq_text KIND_CODEX = SQ_TEXT_STATIC("Codex");

	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_genus(AS_OTHER(value));
		else
			return sq_value_new(value == SQ_NI ? &KIND_NI : &KIND_VERACITY);

	case SQ_G_NUMERAL:
		return sq_value_new(&KIND_NUMERAL);

	case SQ_G_TEXT:
		return sq_value_new(&KIND_TEXT);

	case SQ_G_IMITATION:
		return sq_value_new(sq_form_clone(AS_IMITATION(value)->form));

	case SQ_G_JOURNEY:
		return sq_value_new(&KIND_FUNCTION);

	case SQ_G_FORM:
		return sq_value_new(&KIND_FORM);

	case SQ_G_BOOK:
		return sq_value_new(&KIND_ARRAY);

	case SQ_G_CODEX:
		return sq_value_new(&KIND_CODEX);

	default:
		bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

bool sq_value_not(sq_value arg) {
	return !sq_value_to_veracity(arg);
}

bool sq_value_eql(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_TEXT:
		return sq_value_is_text(rhs) && !strcmp(AS_STR(lhs), AS_STR(rhs));

	case SQ_G_BOOK:
		if (!sq_value_is_book(rhs)) return false;
		struct sq_book *lary = AS_BOOK(lhs), *rary = AS_BOOK(rhs);

		if (lary->length != rary->length)
			return false;

		for (unsigned i = 0; i < lary->length; ++i)
			if (!sq_value_eql(lary->pages[i], rary->pages[i]))
				return false;
		return true;

	case SQ_G_CODEX:
		if (!sq_value_is_codex(rhs))
			return false;

		struct sq_codex *lcodex = AS_CODEX(lhs), *rcodex = AS_CODEX(rhs);

		if (lcodex->length != rcodex->length)
			return false;

		for (unsigned i = 0; i < lcodex->length; ++i)
			if (!sq_value_eql(lcodex->pages[i].value, rcodex->pages[i].value))
				return false;

		return true;


	case SQ_G_IMITATION: {
		struct sq_journey *eql = sq_imitation_lookup_change(AS_IMITATION(lhs), "==");
		sq_value args[2] = { lhs, rhs };

		if (eql != NULL)
			return sq_journey_run_deprecated(eql, 2, args);
		// fallthrough
	}

	default:
		return lhs == rhs;
	}
}

sq_numeral sq_value_cmp(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return AS_NUMBER(lhs) - sq_value_to_numeral(rhs);

	case SQ_G_TEXT:
		// todo: free text
		return strcmp(AS_STR(lhs), sq_value_to_text(rhs)->ptr);

	case SQ_G_IMITATION:
		todo("cmp imitation");

	default:
		die("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
	// 	struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "<=>");

	// 	if (neg != NULL) return sq_journey_run_deprecated(neg, 1, &arg);
	// }
	}
}

sq_value sq_value_neg(sq_value arg) {
	switch (SQ_VTAG(arg)) {
	case SQ_G_NUMERAL:
		return sq_value_new(-AS_NUMBER(arg));

	case SQ_G_IMITATION: {
		struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "-@");

		if (neg != NULL)
			return sq_journey_run_deprecated(neg, 1, &arg);
		// fallthrough
	}

	default:
		die("cannot numerically negate '%s'", TYPENAME(arg));
	}
}

sq_value sq_value_index(sq_value value, sq_value key) {
	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT: {
		int index = sq_value_to_numeral(key);

		if (!index--) sq_throw("cannot index by N.");
		if (index < 0)
			index += AS_TEXT(value)->length;

		if (index < 0 || AS_TEXT(value)->length <= (unsigned) index)
			return SQ_NI;

		char *c = xmalloc(sizeof_array(char , 2));
		c[0] = AS_STR(value)[index];
		c[1] = '\0';
		return sq_value_new(sq_text_new2(c, 2));
	}

	case SQ_G_BOOK:
		return sq_book_index2(AS_BOOK(value), sq_value_to_numeral(key));

	case SQ_G_CODEX:
		return sq_codex_index(AS_CODEX(value), key);

	case SQ_G_IMITATION: {
		struct sq_journey *index = sq_imitation_lookup_change(AS_IMITATION(value), "[]");
		sq_value args[2] = { value, key };

		if (index != NULL)
			return sq_journey_run_deprecated(index, 2, args);
		// fallthrough
	}

	default:
		die("cannot index into '%s'", TYPENAME(value));
	}
}


void sq_value_index_assign(sq_value value, sq_value key, sq_value val) {
	switch (SQ_VTAG(value)) {
	case SQ_G_BOOK:
		sq_book_index_assign2(AS_BOOK(value), sq_value_to_numeral(key), val);
		return;

	case SQ_G_CODEX:
		sq_codex_index_assign(AS_CODEX(value), key, val);
		return;

	case SQ_G_IMITATION: {
		struct sq_journey *index_assign = sq_imitation_lookup_change(AS_IMITATION(value), "[]=");
		sq_value args[3] = { value, key, val };

		if (index_assign != NULL) {
			sq_journey_run_deprecated(index_assign, 2, args);
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
	case SQ_G_NUMERAL:
		return sq_value_new(AS_NUMBER(lhs) + sq_value_to_numeral(rhs));

	case SQ_G_TEXT: {
		struct sq_text *rstr = sq_value_to_text(rhs);
		struct sq_text *result = sq_text_allocate(AS_TEXT(lhs)->length + rstr->length);

		memcpy(result->ptr, AS_STR(lhs), AS_TEXT(lhs)->length);
		memcpy(result->ptr + AS_TEXT(lhs)->length, rstr->ptr, AS_TEXT(rhs)->length + 1);

		// sq_text_free(rstr);
		// if (free_lhs) sq_value_free(lhs);
		return sq_value_new(result);
	}

	case SQ_G_BOOK: {
		if (sq_value_is_journey(rhs))
			return sq_value_new(sq_book_select(AS_BOOK(lhs), AS_JOURNEY(rhs)));

		struct sq_book *lary = AS_BOOK(lhs), *rary = sq_value_to_book(rhs);

		unsigned length = lary->length + rary->length;
		sq_value *pages = xmalloc(sizeof_array(sq_value, length));

		for (unsigned i = 0; i < lary->length; ++i)
			pages[i] = sq_value_clone(lary->pages[i]);

		for (unsigned i = 0; i < rary->length; ++i)
			pages[lary->length + i] = sq_value_clone(rary->pages[i]);

		// sq_book_free(rary);
		return sq_value_new(sq_book_new2(length, pages));
	}

	case SQ_G_CODEX: {
		todo("'+' dicts");
		// struct sq_codex *ldict = AS_CODEX(lhs), *rdict = sq_value_to_codex(rhs);

		// unsigned i = 0, length = lhs->length + rhs->length;
		// sq_value *elements = xmalloc(sizeof_array(sq_value, length));

		// for (; i < lhs->length; ++i)
		// 	elements[i] = sq_value_clone(lary->elements[i]);
		// for (; i < lhs->length; ++i)
		// 	elements[i] = sq_value_clone(rary->elements[i]);

		// sq_book_free(rhs);
		// return sq_value_new(lary);
	}


	case SQ_G_IMITATION: {
		struct sq_journey *add = sq_imitation_lookup_change(AS_IMITATION(lhs), "+");
		sq_value args[2] = { lhs, rhs };

		if (add != NULL)
			return sq_journey_run_deprecated(add, 2, args);
		// fallthrough
	}

	default:
		die("cannot add '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_sub(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return sq_value_new(AS_NUMBER(lhs) - sq_value_to_numeral(rhs));

	case SQ_G_BOOK:
		todo("set difference");

	case SQ_G_CODEX:
		todo("set difference for dict");

	case SQ_G_IMITATION: {
		struct sq_journey *sub = sq_imitation_lookup_change(AS_IMITATION(lhs), "-");
		sq_value args[2] = { lhs, rhs };

		if (sub != NULL)
			return sq_journey_run_deprecated(sub, 2, args);
		// fallthrough
	}

	default:
		die("cannot subtract '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mul(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return sq_value_new(AS_NUMBER(lhs) * sq_value_to_numeral(rhs));

	case SQ_G_TEXT: {
		sq_numeral amnt = sq_value_to_numeral(rhs);
		if (amnt == 0 || AS_TEXT(lhs)->length == 0)
			return sq_value_new(&sq_text_empty);
		if (amnt < 0 || amnt >= UINT_MAX || (amnt * AS_TEXT(lhs)->length) >= UINT_MAX)
			sq_throw("text multiplication by %"PRId64" is out of range", amnt);
		if (amnt == 1)
			return sq_value_new(sq_text_clone(AS_TEXT(lhs)));

		struct sq_text *result = sq_text_allocate(AS_TEXT(lhs)->length * amnt);
		char *ptr = result->ptr;

		for (unsigned i = 0; i < amnt; ++i) {
			memcpy(ptr, AS_STR(lhs), AS_TEXT(lhs)->length + 1);
			ptr += AS_TEXT(lhs)->length;
		}

		return sq_value_new(result);
	}

	case SQ_G_BOOK:;
		struct sq_book *book = AS_BOOK(lhs);

		if (sq_value_is_numeral(rhs)) {
			sq_numeral num = AS_NUMBER(rhs);
			if (num < 0) sq_throw("cannot repeat by %"PRId64" is out of range", num);
			return sq_value_new(sq_book_repeat(book, num));
		}

		if (sq_value_is_text(rhs))
			return sq_value_new(sq_book_join(book, AS_TEXT(rhs)));

		if (sq_value_is_book(rhs))
			return sq_value_new(sq_book_product(book, AS_BOOK(rhs)));

		if (sq_value_is_journey(rhs))
			return sq_value_new(sq_book_map(book, AS_JOURNEY(rhs)));

		goto error;

	case SQ_G_IMITATION: {
		struct sq_journey *mul = sq_imitation_lookup_change(AS_IMITATION(lhs), "*");
		sq_value args[2] = { lhs, rhs };

		if (mul != NULL)
			return sq_journey_run_deprecated(mul, 2, args);
		// fallthrough
	}

	default:
	error:
		die("cannot multiply '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

sq_value sq_value_div(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) die("cannot divide by N");
		return sq_value_new(AS_NUMBER(lhs) / rnum);
	}

	case SQ_G_IMITATION: {
		struct sq_journey *div = sq_imitation_lookup_change(AS_IMITATION(lhs), "/");
		sq_value args[2] = { lhs, rhs };

		if (div != NULL)
			return sq_journey_run_deprecated(div, 2, args);

		// fallthrough
	}

	default:
		die("cannot divide '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mod(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) die("cannot modulo by N");
		return sq_value_new(AS_NUMBER(lhs) % rnum);
	}

	case SQ_G_BOOK:;
		struct sq_book *book = AS_BOOK(lhs);

		if (sq_value_is_journey(rhs))
			return sq_book_reduce(book, AS_JOURNEY(rhs));

		goto error;

	case SQ_G_IMITATION: {
		struct sq_journey *mod = sq_imitation_lookup_change(AS_IMITATION(lhs), "%");
		sq_value args[2] = { lhs, rhs };

		if (mod != NULL)
			return sq_journey_run_deprecated(mod, 2, args);

		// fallthrough
	}

	default:
	error:
		die("cannot modulo '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}


sq_value sq_value_pow(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) die("cannot modulo by N");
		return sq_value_new((sq_numeral) pow(AS_NUMBER(lhs), rnum));
	}

	case SQ_G_IMITATION: {
		struct sq_journey *pow = sq_imitation_lookup_change(AS_IMITATION(lhs), "**");
		sq_value args[2] = { lhs, rhs };

		if (pow != NULL)
			return sq_journey_run_deprecated(pow, 2, args);

		// fallthrough
	}

	default:
		die("cannot exponentiate '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}


sq_value sq_value_call(sq_value tocall, struct sq_args args) {
	switch (sq_value_genus_tag(tocall)) {
	case SQ_G_FORM:
		return sq_value_new(sq_form_imitate(sq_value_as_form(tocall), args));

	case SQ_G_JOURNEY:
		return sq_journey_run(sq_value_as_journey(tocall), args);

	case SQ_G_IMITATION:
		todo("call imitation"); 

	default:
		sq_throw("cannot call '%s'.", TYPENAME(tocall));
	}
}

struct sq_text *sq_value_to_text(sq_value value) {
	static struct sq_text yay_string = SQ_TEXT_STATIC("yay");
	static struct sq_text nay_string = SQ_TEXT_STATIC("nay");
	static struct sq_text ni_string = SQ_TEXT_STATIC("ni");

	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_text(AS_OTHER(value));
		else if (value == SQ_NI)
			return &ni_string;
		else
			return value == SQ_YAY ? &yay_string : &nay_string;

	case SQ_G_NUMERAL:
		return sq_numeral_to_text(AS_NUMBER(value));

	case SQ_G_TEXT:
		sq_text_clone(AS_TEXT(value));
		return AS_TEXT(value);

	case SQ_G_FORM:
		return sq_text_new(strdup(AS_FORM(value)->name));

	case SQ_G_BOOK:
		return sq_book_to_text(AS_BOOK(value));

	case SQ_G_CODEX:
		return sq_codex_to_text(AS_CODEX(value));

	case SQ_G_IMITATION: {
		struct sq_journey *to_text = sq_imitation_lookup_change(AS_IMITATION(value), "to_text");

		if (to_text != NULL) {
			sq_value text = sq_journey_run_deprecated(to_text, 1, &value);
			if (!sq_value_is_text(text))
				die("to_text for an imitation of '%s' didn't return a text", AS_IMITATION(value)->form->name);
			return AS_TEXT(text);
		}
		// else fallthrough
	}

	case SQ_G_JOURNEY:
		die("cannot convert %s to a text", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_numeral sq_value_to_numeral(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_numeral(AS_OTHER(value));
		else
			return value == SQ_YAY ? 1 : 0;

	case SQ_G_NUMERAL:
		return AS_NUMBER(value);

	case SQ_G_TEXT:
		if (sq_numeral_starts(AS_STR(value)))
			return sq_roman_to_numeral(AS_STR(value), NULL);
		else
			return strtoll(AS_STR(value), NULL, 10);

	case SQ_G_BOOK:
		return sq_value_new(sq_book_to_text(AS_BOOK(value)));

	case SQ_G_IMITATION: {
		struct sq_journey *to_numeral = sq_imitation_lookup_change(AS_IMITATION(value), "to_numeral");

		if (to_numeral != NULL) {
			sq_value numeral = sq_journey_run_deprecated(to_numeral, 1, &value);
			if (!sq_value_is_numeral(numeral))
				die("to_numeral for an imitation of '%s' didn't return a numeral", AS_IMITATION(value)->form->name);
			return AS_NUMBER(numeral);
		}
		// else fallthrough
	}

	case SQ_G_FORM:
	case SQ_G_JOURNEY:
	case SQ_G_CODEX:
		die("cannot convert %s to a numeral", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

bool sq_value_to_veracity(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_veracity(AS_OTHER(value));
		else
			return value == SQ_YAY;

	case SQ_G_NUMERAL:
		return AS_NUMBER(value);

	case SQ_G_TEXT:
		return *AS_STR(value);

	case SQ_G_BOOK:
		return AS_BOOK(value)->length;

	case SQ_G_CODEX:
		return AS_CODEX(value)->length;

	case SQ_G_IMITATION: {
		struct sq_journey *to_veracity = sq_imitation_lookup_change(AS_IMITATION(value), "to_veracity");

		if (to_veracity != NULL) {
			sq_value veracity = sq_journey_run_deprecated(to_veracity, 1, &value);
			if (!sq_value_is_veracity(veracity))
				die("to_veracity for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->name);
			return sq_value_as_veracity(veracity);
		}
		// else fallthrough
	}

	case SQ_G_FORM:
	case SQ_G_JOURNEY:
		die("cannot convert %s to a veracity", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

size_t sq_value_length(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_BOOK:
		return sq_value_as_book(value)->length;

	case SQ_G_CODEX:
		return sq_value_as_codex(value)->length;

	case SQ_G_TEXT:
		return AS_TEXT(value)->length;

	case SQ_G_IMITATION: {
		struct sq_journey *length = sq_imitation_lookup_change(AS_IMITATION(value), "length");

		if (length != NULL) {
			sq_value veracity = sq_journey_run_deprecated(length, 1, &value);
			if (!sq_value_is_numeral(veracity))
				die("length for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->name);
			return AS_NUMBER(veracity);
		}
		// else fallthrough
	}

	case SQ_G_OTHER:
	case SQ_G_NUMERAL:
	case SQ_G_FORM:
	case SQ_G_JOURNEY:
		die("cannot get length of %s", TYPENAME(value));

	default:
		bug("<UNDEFINED: %"PRId64">", value);
	}
}

struct sq_book *sq_value_to_book(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_BOOK:
		++AS_BOOK(value)->refcount;
		return AS_BOOK(value);

	case SQ_G_TEXT: {
		const struct sq_text *text = AS_TEXT(value);
		struct sq_book *book = sq_book_allocate(text->length);

		for (unsigned i = 0; i < text->length; ++i) {
			char *data = xmalloc(2);
			data[0] = text->ptr[i];
			data[1] = '\0';
			book->pages[book->length++] = sq_value_new(sq_text_new2(data, 1));
		}

		return book;
	}

	default:
		todo("others to book");
	}
}

struct sq_codex *sq_value_to_codex(sq_value value) {
	(void) value;
	die("todo");
}


sq_value sq_value_get_attr(sq_value soul, const char *attr) {
	if (!strcmp(attr, "genus"))
		return sq_value_genus(soul);

	if (!strcmp(attr, "length"))
		return sq_value_new((sq_numeral) sq_value_length(soul));

	sq_value result = SQ_UNDEFINED;

	switch (sq_value_genus_tag(soul)) {
	case SQ_G_FORM:
		result = sq_form_get_attr(sq_value_as_form(soul), attr);
		break;

	case SQ_G_IMITATION:
		result = sq_imitation_get_attr(sq_value_as_imitation(soul), attr);
		break;

	case SQ_G_BOOK:
		if (!strcmp(attr, "verso"))
			result = sq_book_index(sq_value_as_book(soul), 1);
		else if (!strcmp(attr, "recto"))
			result = sq_book_index2(sq_value_as_book(soul), -1);
		break;

	case SQ_G_OTHER:
		if (sq_value_is_other(soul))
			return sq_other_get_attr(AS_OTHER(soul), attr);
		// else, fallthrough

	case SQ_G_NUMERAL:
	case SQ_G_TEXT:
	case SQ_G_JOURNEY:
	case SQ_G_CODEX:
		break;
	}

	if (result == SQ_UNDEFINED)
		sq_throw("unknown attribute '%s' for genus '%s'", attr, TYPENAME(soul));

	return result;
}

void sq_value_set_attr(sq_value soul, const char *attr, sq_value value) {
	switch (sq_value_genus_tag(soul)) {
	case SQ_G_FORM:
		if (sq_form_set_attr(sq_value_as_form(soul), attr, value))
			return;

		break;

	case SQ_G_IMITATION:
		if (sq_imitation_set_attr(sq_value_as_imitation(soul), attr, value))
			return;

		break;

	case SQ_G_BOOK:
		if (!strcmp(attr, "verso")) {
			sq_book_index_assign(sq_value_as_book(soul), 1, value);
			return;
		}

		if (!strcmp(attr, "recto")) {
			sq_book_index_assign2(sq_value_as_book(soul), -1, value);
			return;
		}

		break;

	case SQ_G_OTHER:
		if (sq_value_is_other(soul) && sq_other_set_attr(AS_OTHER(soul), attr, value))
			return;
		// else, fallthrough

	case SQ_G_NUMERAL:
	case SQ_G_TEXT:
	case SQ_G_JOURNEY:
	case SQ_G_CODEX:
		break;
	}

	sq_throw("cannot assign attribute '%s' for a type of genus '%s'", attr, TYPENAME(soul));
}

bool sq_value_matches(sq_value formlike, sq_value to_check) {
	bool matches;
	switch (sq_value_genus_tag(formlike)) {
	case SQ_G_FORM:
		return sq_form_is_parent_of(sq_value_as_form(formlike), to_check);

	case SQ_G_JOURNEY: {
		struct sq_args args = { .pargc = 1, .pargv = &to_check };
		sq_value_clone(to_check); // as we pass ownership.

		sq_value result = sq_journey_run(sq_value_as_journey(formlike), args);
		matches = sq_value_to_veracity(result);

		sq_value_free(result);
		return matches;
	}

	case SQ_G_TEXT:
		// temporary hack until we get forms for primitives too
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Numeral") && sq_value_is_numeral(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Text") && sq_value_is_text(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Veracity") && sq_value_is_veracity(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Ni") && to_check == SQ_NI) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Form") && sq_value_is_form(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Imitation") && sq_value_is_imitation(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Journey") && sq_value_is_journey(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Book") && sq_value_is_book(to_check)) return true;
		if (!strcmp(sq_value_as_text(formlike)->ptr, "Codex") && sq_value_is_codex(to_check)) return true;
		// fallthrough

	case SQ_G_OTHER:
		if (sq_value_is_other(formlike))
			return sq_other_matches(AS_OTHER(formlike), to_check);
		// else, fallthrough
	case SQ_G_NUMERAL:
		return sq_value_eql(formlike, to_check);

	case SQ_G_BOOK:
		for (unsigned i = 0; i < sq_value_as_book(formlike)->length; ++i)
			if (sq_value_matches(sq_value_as_book(formlike)->pages[i], to_check))
				return true;
		return false;

	case SQ_G_IMITATION:
	case SQ_G_CODEX:
		sq_throw("cannot `match` on %s", TYPENAME(to_check));
	}

	bug("unknown genus encountered: %d", sq_value_genus_tag(formlike));
}
