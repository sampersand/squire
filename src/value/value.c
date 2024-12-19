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
#define AS_VERACITY sq_value_as_veracity
#define AS_JOURNEY sq_value_as_journey
#define AS_BOOK sq_value_as_book
#define AS_CODEX sq_value_as_codex
#define AS_OTHER sq_value_as_other
#define TYPENAME sq_value_typename
#define AS_STR(c) (AS_TEXT(c)->ptr)

void sq_value_dump(FILE *out, sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (value == SQ_NI)
			fprintf(out, "Ni()");
		else if (sq_value_is_veracity(value))
			fprintf(out, "Veracity(%s)", AS_VERACITY(value) ? "yea" : "nay");
		else
			sq_other_dump(out, AS_OTHER(value));

		break;

	case SQ_G_NUMERAL:
		fprintf(out, "%"PRId64, AS_NUMBER(value));
		break;

	case SQ_G_TEXT:
		sq_text_dump(out, AS_TEXT(value));
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
		sq_bug("<UNDEFINED: %"PRId64">", value);
	}
}

void sq_value_mark(sq_value value) {
	sq_assert_nundefined(value);

	if (sq_value_is_numeral(value)) return;

	// printf("marking: ");
	// sq_value_dump(stdout, value);
	// printf("\n");
	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT: sq_text_mark(AS_TEXT(value)); break;
	case SQ_G_FORM: sq_form_mark(AS_FORM(value)); break;
	case SQ_G_IMITATION: sq_imitation_mark(AS_IMITATION(value)); break;
	case SQ_G_JOURNEY: sq_journey_mark(AS_JOURNEY(value)); break;
	case SQ_G_BOOK: sq_book_mark(AS_BOOK(value)); break;
	case SQ_G_CODEX: sq_codex_mark(AS_CODEX(value)); break;
	case SQ_G_OTHER: if (sq_value_is_other(value)) sq_other_mark(AS_OTHER(value));
	}
}

void sq_value_deallocate(sq_value value) {
	sq_assert_nundefined(value);

	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT: sq_text_deallocate(AS_TEXT(value)); return;
	case SQ_G_FORM: sq_form_deallocate(AS_FORM(value)); return;
	case SQ_G_IMITATION: sq_imitation_deallocate(AS_IMITATION(value)); return;
	case SQ_G_JOURNEY: sq_journey_deallocate(AS_JOURNEY(value)); return;
	case SQ_G_BOOK: sq_book_deallocate(AS_BOOK(value)); return;
	case SQ_G_CODEX: sq_codex_deallocate(AS_CODEX(value)); return;
	case SQ_G_OTHER: if (sq_value_is_other(value)) sq_other_deallocate(AS_OTHER(value));
	}
}

const char *sq_value_typename(sq_value value) {
	sq_assert_nundefined(value);

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
	default: sq_bug("unknown tag '%d'", (int) SQ_VTAG(value));
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
			return sq_value_new_text(value == SQ_NI ? &KIND_NI : &KIND_VERACITY);

	case SQ_G_NUMERAL:
		return sq_value_new_text(&KIND_NUMERAL);

	case SQ_G_TEXT:
		return sq_value_new_text(&KIND_TEXT);

	case SQ_G_IMITATION:
		return sq_value_new_form(AS_IMITATION(value)->form);

	case SQ_G_JOURNEY:
		return sq_value_new_text(&KIND_FUNCTION);

	case SQ_G_FORM:
		return sq_value_new_text(&KIND_FORM);

	case SQ_G_BOOK:
		return sq_value_new_text(&KIND_ARRAY);

	case SQ_G_CODEX:
		return sq_value_new_text(&KIND_CODEX);

	default:
		sq_bug("unknown tag '%d'", (int) SQ_VTAG(value));
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
			return sq_value_to_veracity(sq_journey_run_deprecated(eql, 2, args));

		SQ_FALLTHROUGH
	}

	default:
		return lhs == rhs;
	}
}

sq_numeral sq_value_cmp(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral l = AS_NUMBER(lhs);
		sq_numeral r = sq_value_to_numeral(rhs);

		return l < r ? -1 : l == r ? 0 : 1;
	}

	case SQ_G_TEXT:
		// todo: free text
		return strcmp(AS_STR(lhs), sq_value_to_text(rhs)->ptr);

	case SQ_G_IMITATION: {
		struct sq_journey *cmp = sq_imitation_lookup_change(AS_IMITATION(lhs), "<=>");
		sq_value args[2] = { lhs, rhs };

		if (cmp != NULL)
			return sq_value_to_numeral(sq_journey_run_deprecated(cmp, 2, args));

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
	// 	struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "<=>");

	// 	if (neg != NULL) return sq_journey_run_deprecated(neg, 1, &arg);
	// }
	}
}

sq_value sq_value_neg(sq_value arg) {
	switch (SQ_VTAG(arg)) {
	case SQ_G_NUMERAL:
		return sq_value_new_numeral(-AS_NUMBER(arg));

	case SQ_G_IMITATION: {
		struct sq_journey *neg = sq_imitation_lookup_change(AS_IMITATION(arg), "-@");

		if (neg != NULL)
			return sq_journey_run_deprecated(neg, 1, &arg);

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot numerically negate '%s'", TYPENAME(arg));
	}
}

sq_value sq_value_index(sq_value value, sq_value key) {
	switch (SQ_VTAG(value)) {
	case SQ_G_TEXT: {
		int index = sq_value_to_numeral(key);

		if (!index--) sq_throw("cannot index by N.");
		if (index < 0)
			index += AS_TEXT(value)->length + 1;

		if (index < 0 || AS_TEXT(value)->length <= (unsigned) index)
			return SQ_NI;

		char *c = sq_malloc_vec(char, 2);
		c[0] = AS_STR(value)[index];
		c[1] = '\0';
		return sq_value_new_text(sq_text_new2(c, 1));
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

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot index into '%s'", TYPENAME(value));
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

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot index assign into '%s'", TYPENAME(value));
	}
}

sq_value sq_value_add(sq_value lhs, sq_value rhs) {
	// bool free_rhs = false;

	if (sq_value_is_text(rhs)) {
		// free_lhs = true;
		lhs = sq_value_new_text(sq_value_to_text(lhs));
	}

	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return sq_value_new_numeral(AS_NUMBER(lhs) + sq_value_to_numeral(rhs));

	case SQ_G_TEXT: {
		struct sq_text *rstr = sq_value_to_text(rhs);
		struct sq_text *result = sq_text_allocate(AS_TEXT(lhs)->length + rstr->length);

		memcpy(result->ptr, AS_STR(lhs), AS_TEXT(lhs)->length);
		memcpy(result->ptr + AS_TEXT(lhs)->length, rstr->ptr, rstr->length + 1);

		return sq_value_new_text(result);
	}

	case SQ_G_BOOK: {
		if (sq_value_is_journey(rhs))
			return sq_value_new_book(sq_book_select(AS_BOOK(lhs), AS_JOURNEY(rhs)));

		struct sq_book *lary = AS_BOOK(lhs), *rary = sq_value_to_book(rhs);

		unsigned length = lary->length + rary->length;
		sq_value *pages = sq_malloc_vec(sq_value, length);

		// todo: memcpy
		for (unsigned i = 0; i < lary->length; ++i)
			pages[i] = lary->pages[i];

		for (unsigned i = 0; i < rary->length; ++i)
			pages[lary->length + i] = rary->pages[i];

		return sq_value_new_book(sq_book_new2(length, pages));
	}

	case SQ_G_CODEX: {
		sq_todo("'+' dicts");
		// struct sq_codex *ldict = AS_CODEX(lhs), *rdict = sq_value_to_codex(rhs);

		// unsigned i = 0, length = lhs->length + rhs->length;
		// sq_value *elements = sq_malloc_vec(sq_value, length);

		// for (; i < lhs->length; ++i)
		// 	elements[i] = lary->elements[i];
		// for (; i < lhs->length; ++i)
		// 	elements[i] = rary->elements[i];

		// return sq_value_new(lary);
	}


	case SQ_G_IMITATION: {
		struct sq_journey *add = sq_imitation_lookup_change(AS_IMITATION(lhs), "+");
		sq_value args[2] = { lhs, rhs };

		if (add != NULL)
			return sq_journey_run_deprecated(add, 2, args);

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot add '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_sub(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return sq_value_new_numeral(AS_NUMBER(lhs) - sq_value_to_numeral(rhs));

	case SQ_G_BOOK:
		sq_todo("set difference");

	case SQ_G_CODEX:
		sq_todo("set difference for dict");

	case SQ_G_IMITATION: {
		struct sq_journey *sub = sq_imitation_lookup_change(AS_IMITATION(lhs), "-");
		sq_value args[2] = { lhs, rhs };

		if (sub != NULL)
			return sq_journey_run_deprecated(sub, 2, args);

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot subtract '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mul(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL:
		return sq_value_new_numeral(AS_NUMBER(lhs) * sq_value_to_numeral(rhs));

	case SQ_G_TEXT: {
		sq_numeral amnt = sq_value_to_numeral(rhs);
		if (amnt == 0 || AS_TEXT(lhs)->length == 0)
			return sq_value_new_text(&sq_text_empty);
		if (amnt < 0 || amnt >= UINT_MAX || (amnt * AS_TEXT(lhs)->length) >= UINT_MAX)
			sq_throw("text multiplication by %"PRId64" is out of range", amnt);
		if (amnt == 1)
			return sq_value_new_text(AS_TEXT(lhs));

		struct sq_text *result = sq_text_allocate(AS_TEXT(lhs)->length * amnt);
		char *ptr = result->ptr;

		for (unsigned i = 0; i < amnt; ++i) {
			memcpy(ptr, AS_STR(lhs), AS_TEXT(lhs)->length + 1);
			ptr += AS_TEXT(lhs)->length;
		}

		return sq_value_new_text(result);
	}

	case SQ_G_BOOK:;
		struct sq_book *book = AS_BOOK(lhs);

		if (sq_value_is_numeral(rhs)) {
			sq_numeral num = AS_NUMBER(rhs);
			if (num < 0) sq_throw("cannot repeat by %"PRId64" is out of range", num);
			return sq_value_new_book(sq_book_repeat(book, num));
		}

		if (sq_value_is_text(rhs))
			return sq_value_new_text(sq_book_join(book, AS_TEXT(rhs)));

		if (sq_value_is_book(rhs))
			return sq_value_new_book(sq_book_product(book, AS_BOOK(rhs)));

		if (sq_value_is_journey(rhs))
			return sq_value_new_book(sq_book_map(book, AS_JOURNEY(rhs)));

		goto error;

	case SQ_G_IMITATION: {
		struct sq_journey *mul = sq_imitation_lookup_change(AS_IMITATION(lhs), "*");
		sq_value args[2] = { lhs, rhs };

		if (mul != NULL)
			return sq_journey_run_deprecated(mul, 2, args);

		SQ_FALLTHROUGH
	}

	default:
	error:
		sq_throw("cannot multiply '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}

sq_value sq_value_div(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) sq_throw("cannot divide by N");
		return sq_value_new_numeral(AS_NUMBER(lhs) / rnum);
	}

	case SQ_G_IMITATION: {
		struct sq_journey *div = sq_imitation_lookup_change(AS_IMITATION(lhs), "/");
		sq_value args[2] = { lhs, rhs };

		if (div != NULL)
			return sq_journey_run_deprecated(div, 2, args);

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot divide '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}

}

sq_value sq_value_mod(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) sq_throw("cannot modulo by N");
		return sq_value_new_numeral(AS_NUMBER(lhs) % rnum);
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

		SQ_FALLTHROUGH
	}

	default:
	error:
		sq_throw("cannot modulo '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}


sq_value sq_value_pow(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_G_NUMERAL: {
		sq_numeral rnum = sq_value_to_numeral(rhs);
		if (!rnum) return sq_value_new_numeral(1);

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wbad-function-cast"
#endif
		return sq_value_new_numeral((sq_numeral) pow(AS_NUMBER(lhs), rnum));
#ifdef __clang__
# pragma clang diagnostic pop
#endif
	}

	case SQ_G_IMITATION: {
		struct sq_journey *pow = sq_imitation_lookup_change(AS_IMITATION(lhs), "^");
		sq_value args[2] = { lhs, rhs };

		if (pow != NULL)
			return sq_journey_run_deprecated(pow, 2, args);

		SQ_FALLTHROUGH
	}

	default:
		sq_throw("cannot exponentiate '%s' by '%s'", TYPENAME(lhs), TYPENAME(rhs));
	}
}


sq_value sq_value_call(sq_value tocall, struct sq_args args) {
	sq_assert_nundefined(tocall);

	switch (sq_value_genus_tag(tocall)) {
	case SQ_G_FORM:
		return sq_value_new_imitation(sq_form_imitate(AS_FORM(tocall), args));

	case SQ_G_JOURNEY:
		return sq_journey_run(AS_JOURNEY(tocall), args);

	case SQ_G_IMITATION:
		sq_todo("call imitation"); 

	case SQ_G_OTHER:
		if (sq_value_is_other(tocall)) {
			sq_value result = sq_other_call(AS_OTHER(tocall), args);

			if (result != SQ_UNDEFINED)
				return result;
		}

		SQ_FALLTHROUGH

	default:
		sq_throw("cannot call '%s'.", TYPENAME(tocall));
	}
}

struct sq_text *sq_value_to_text(sq_value value) {
	static struct sq_text yea_string = SQ_TEXT_STATIC("yea");
	static struct sq_text nay_string = SQ_TEXT_STATIC("nay");
	static struct sq_text ni_string = SQ_TEXT_STATIC("ni");

	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_text(AS_OTHER(value));
		else if (value == SQ_NI)
			return &ni_string;
		else
			return value == SQ_YEA ? &yea_string : &nay_string;

	case SQ_G_NUMERAL:
		return sq_numeral_to_text(AS_NUMBER(value));

	case SQ_G_TEXT:
		return AS_TEXT(value);

	case SQ_G_FORM:
		return sq_text_new(strdup(AS_FORM(value)->vt->name));

	case SQ_G_BOOK:
		return sq_book_to_text(AS_BOOK(value));

	case SQ_G_CODEX:
		return sq_codex_to_text(AS_CODEX(value));

	case SQ_G_IMITATION: {
		struct sq_journey *to_text = sq_imitation_lookup_change(AS_IMITATION(value), "to_text");

		if (to_text != NULL) {
			sq_value text = sq_journey_run_deprecated(to_text, 1, &value);
			if (!sq_value_is_text(text))
				sq_throw("to_text for an imitation of '%s' didn't return a text", AS_IMITATION(value)->form->vt->name);
			return AS_TEXT(text);
		}

		SQ_FALLTHROUGH
	}

	case SQ_G_JOURNEY:
		sq_throw("cannot convert %s to a text", TYPENAME(value));

	default:
		sq_bug("<UNDEFINED: %"PRId64">", value);
	}
}

sq_numeral sq_value_to_numeral(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_numeral(AS_OTHER(value));
		else
			return value == SQ_YEA ? 1 : 0;

	case SQ_G_NUMERAL:
		return AS_NUMBER(value);

	case SQ_G_TEXT:
		if (sq_numeral_starts(AS_STR(value)))
			return sq_roman_to_numeral(AS_STR(value), NULL);
		else
			return strtoll(AS_STR(value), NULL, 10);

	case SQ_G_BOOK:
		return AS_BOOK(value)->length;

	case SQ_G_IMITATION: {
		struct sq_journey *to_numeral = sq_imitation_lookup_change(AS_IMITATION(value), "to_numeral");

		if (to_numeral != NULL) {
			sq_value numeral = sq_journey_run_deprecated(to_numeral, 1, &value);
			if (!sq_value_is_numeral(numeral))
				sq_throw("to_numeral for an imitation of '%s' didn't return a numeral", AS_IMITATION(value)->form->vt->name);
			return AS_NUMBER(numeral);
		}

		SQ_FALLTHROUGH
	}

	case SQ_G_FORM:
	case SQ_G_JOURNEY:
	case SQ_G_CODEX:
		sq_throw("cannot convert %s to a numeral", TYPENAME(value));

	default:
		sq_bug("<UNDEFINED: %"PRId64">", value);
	}
}

bool sq_value_to_veracity(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_OTHER:
		if (sq_value_is_other(value))
			return sq_other_to_veracity(AS_OTHER(value));
		else
			return value == SQ_YEA;

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
				sq_throw("to_veracity for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->vt->name);
			return AS_VERACITY(veracity);
		}

		SQ_FALLTHROUGH
	}

	case SQ_G_FORM:
	case SQ_G_JOURNEY:
		sq_throw("cannot convert %s to a veracity", TYPENAME(value));

	default:
		sq_bug("<UNDEFINED: %"PRId64">", value);
	}
}

size_t sq_value_length(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_BOOK:
		return AS_BOOK(value)->length;

	case SQ_G_CODEX:
		return AS_CODEX(value)->length;

	case SQ_G_TEXT:
		return AS_TEXT(value)->length;

	case SQ_G_IMITATION: {
		struct sq_journey *length = sq_imitation_lookup_change(AS_IMITATION(value), "length");

		if (length != NULL) {
			sq_value veracity = sq_journey_run_deprecated(length, 1, &value);
			if (!sq_value_is_numeral(veracity))
				sq_throw("length for an imitation of '%s' didn't return a veracity", AS_IMITATION(value)->form->vt->name);
			return AS_NUMBER(veracity);
		}

		SQ_FALLTHROUGH
	}

	case SQ_G_OTHER:
	case SQ_G_NUMERAL:
	case SQ_G_FORM:
	case SQ_G_JOURNEY:
		sq_throw("cannot get length of %s", TYPENAME(value));

	default:
		sq_bug("<UNDEFINED: %"PRId64">", value);
	}
}

struct sq_book *sq_value_to_book(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_G_BOOK:
		return AS_BOOK(value);

	case SQ_G_TEXT: {
		const struct sq_text *text = AS_TEXT(value);
		struct sq_book *book = sq_book_allocate(text->length);

		for (unsigned i = 0; i < text->length; ++i) {
			char *data = sq_malloc_heap(2);
			data[0] = text->ptr[i];
			data[1] = '\0';
			book->pages[book->length++] = sq_value_new_text(sq_text_new2(data, 1));
		}

		return book;
	}

	case SQ_G_NUMERAL: {
		sq_todo("make numeral -> book use roman numerals");
		sq_numeral numeral = AS_NUMBER(value);
		if (numeral == 0) {
			struct sq_book *book = sq_book_allocate(1);
			book->pages[book->length++] = value;
			return book;
		}

		struct sq_book *book = sq_book_allocate(40); // eh, 40's enough memory right?

		for (; numeral; numeral /= 10) {
			book->pages[book->length++] = sq_value_new_numeral(numeral % 10);
		}

		sq_value tmp;
		for (unsigned i = 0; i < book->length / 2; ++i)
			tmp = book->pages[i],
			book->pages[i] = book->pages[book->length - i - 1],
			book->pages[book->length - i - 1] = tmp;

		return book;
	}

	case SQ_G_OTHER:
		if (value == SQ_NI)
			return sq_book_new2(0, NULL);
		SQ_FALLTHROUGH
	default:
		sq_todo("others to book");
	}
}

struct sq_codex *sq_value_to_codex(sq_value value) {
	(void) value;
	sq_throw("todo");
}


sq_value sq_value_get_attr(sq_value soul, const char *attr) {
	if (!strcmp(attr, "genus"))
		return sq_value_genus(soul);

	if (!strcmp(attr, "length"))
		return sq_value_new_numeral(sq_value_length(soul));

	sq_value result = SQ_UNDEFINED;

	switch (sq_value_genus_tag(soul)) {
	case SQ_G_FORM:
		result = sq_form_get_attr(AS_FORM(soul), attr);
		break;

	case SQ_G_IMITATION:
		result = sq_imitation_get_attr(AS_IMITATION(soul), attr);
		break;

	case SQ_G_BOOK:
		if (!strcmp(attr, "verso"))
			result = sq_book_index(AS_BOOK(soul), 1);
		else if (!strcmp(attr, "recto"))
			result = sq_book_index2(AS_BOOK(soul), -1);
		break;

	case SQ_G_JOURNEY:
		// this should probably be more sophisticated, eg the arity of the nth pattern
		if (!strcmp(attr, "arity"))
			result = sq_value_new_numeral(AS_JOURNEY(soul)->patterns[0].pargc);
		break;

	case SQ_G_OTHER:
		if (sq_value_is_other(soul))
			result = sq_other_get_attr(AS_OTHER(soul), attr);
		SQ_FALLTHROUGH

	case SQ_G_TEXT:
		if (!strcmp(attr, "verso"))
			result = sq_value_index(soul, sq_value_new_numeral(1));
		else if (!strcmp(attr, "recto"))
			result = sq_value_index(soul, sq_value_new_numeral(-1));
		break;

	case SQ_G_NUMERAL:
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
		if (sq_form_set_attr(AS_FORM(soul), attr, value))
			return;

		break;

	case SQ_G_IMITATION:
		if (sq_imitation_set_attr(AS_IMITATION(soul), attr, value))
			return;

		break;

	case SQ_G_BOOK:
		if (!strcmp(attr, "verso")) {
			sq_book_index_assign(AS_BOOK(soul), 1, value);
			return;
		}

		if (!strcmp(attr, "recto")) {
			sq_book_index_assign2(AS_BOOK(soul), -1, value);
			return;
		}

		break;

	case SQ_G_OTHER:
		if (sq_value_is_other(soul) && sq_other_set_attr(AS_OTHER(soul), attr, value))
			return;
		SQ_FALLTHROUGH

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
		return sq_form_is_parent_of(AS_FORM(formlike), to_check);

	case SQ_G_JOURNEY: {
		struct sq_args args = { .pargc = 1, .pargv = &to_check };

		sq_value result = sq_journey_run(AS_JOURNEY(formlike), args);
		matches = sq_value_to_veracity(result);

		return matches;
	}

	case SQ_G_TEXT:
		// temporary hack until we get forms for primitives too
		if (!strcmp(AS_TEXT(formlike)->ptr, "Numeral") && sq_value_is_numeral(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Text") && sq_value_is_text(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Veracity") && sq_value_is_veracity(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Ni") && to_check == SQ_NI) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Form") && sq_value_is_form(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Imitation") && sq_value_is_imitation(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Journey") && sq_value_is_journey(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Book") && sq_value_is_book(to_check)) return true;
		if (!strcmp(AS_TEXT(formlike)->ptr, "Codex") && sq_value_is_codex(to_check)) return true;
		SQ_FALLTHROUGH

	case SQ_G_OTHER:
		if (sq_value_is_other(formlike))
			return sq_other_matches(AS_OTHER(formlike), to_check);
		SQ_FALLTHROUGH
	
	case SQ_G_NUMERAL:
		return sq_value_eql(formlike, to_check);

	case SQ_G_BOOK:
		for (unsigned i = 0; i < AS_BOOK(formlike)->length; ++i)
			if (sq_value_matches(AS_BOOK(formlike)->pages[i], to_check))
				return true;
		return false;

	case SQ_G_IMITATION:
	case SQ_G_CODEX:
		sq_throw("cannot `match` on %s", TYPENAME(to_check));
	}

	sq_bug("unknown genus encountered: %d", sq_value_genus_tag(formlike));
}
