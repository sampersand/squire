#include "value.h"
#include "array.h"
#include "class.h"
#include "function.h"
#include "shared.h"
#include "string.h"
#include "roman.h"
#include "dict.h"
#include <string.h>

#define IS_STRING sq_value_is_string
#define AS_STRING sq_value_as_string
#define AS_NUMBER sq_value_as_number
#define AS_CLASS sq_value_as_class
#define AS_INSTANCE sq_value_as_instance
#define AS_FUNCTION sq_value_as_function
#define AS_ARRAY sq_value_as_array
#define AS_DICT sq_value_as_dict
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
		fprintf(out, "Number(%lld)", AS_NUMBER(value));
		break;

	case SQ_TSTRING:
		fprintf(out, "String(%s)", AS_STR(value));
		break;

	case SQ_TCLASS:
		sq_class_dump(out, AS_CLASS(value));
		break;

	case SQ_TINSTANCE:
		sq_instance_dump(out, AS_INSTANCE(value));
		break;

	case SQ_TFUNCTION:
		sq_function_dump(out, AS_FUNCTION(value));
		break;

	case SQ_TARRAY:
		sq_array_dump(out, AS_ARRAY(value));
		break;

	case SQ_TDICT:
		sq_dict_dump(out, AS_DICT(value));
		break;

	default:
		bug("<UNDEFINED: %lld>", value);
	}
}

sq_value sq_value_clone(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		return sq_value_new_string(sq_string_clone(AS_STRING(value)));

	case SQ_TCLASS:
		return sq_value_new_class(sq_class_clone(AS_CLASS(value)));

	case SQ_TINSTANCE:
		return sq_value_new_instance(sq_instance_clone(AS_INSTANCE(value)));

	case SQ_TFUNCTION:
		return sq_value_new_function(sq_function_clone(AS_FUNCTION(value)));

	case SQ_TARRAY:
		return sq_value_new_array(sq_array_clone(AS_ARRAY(value)));

	case SQ_TDICT:
		return sq_value_new_dict(sq_dict_clone(AS_DICT(value)));

	default:
		return value;
	}
}

void sq_value_free(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		sq_string_free(AS_STRING(value));
		return;

	case SQ_TCLASS:
		sq_class_free(AS_CLASS(value));
		return;

	case SQ_TINSTANCE:
		sq_instance_free(AS_INSTANCE(value));
		return;

	case SQ_TFUNCTION:
		sq_function_free(AS_FUNCTION(value));
		return;

	case SQ_TARRAY:
		sq_array_free(AS_ARRAY(value));
		return;

	case SQ_TDICT:
		sq_dict_free(AS_DICT(value));
		return;
	}
}

const char *sq_value_typename(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TCONST: return sq_value_is_null(value) ? "null" : "boolean";
	case SQ_TNUMBER: return "number";
	case SQ_TSTRING: return "string";
	case SQ_TINSTANCE: return "object";
	case SQ_TFUNCTION: return "function";
	case SQ_TCLASS: return "class";
	case SQ_TARRAY: return "array";
	case SQ_TDICT: return "dict";
	default: bug("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

sq_value sq_value_kindof(sq_value value) {
	static struct sq_string KIND_BOOLEAN = SQ_STRING_STATIC("boolean");
	static struct sq_string KIND_NULL = SQ_STRING_STATIC("null");
	static struct sq_string KIND_NUMBER = SQ_STRING_STATIC("number");
	static struct sq_string KIND_STRING = SQ_STRING_STATIC("string");
	static struct sq_string KIND_FUNCTION = SQ_STRING_STATIC("function");
	static struct sq_string KIND_CLASS = SQ_STRING_STATIC("class");
	static struct sq_string KIND_ARRAY = SQ_STRING_STATIC("array");
	static struct sq_string KIND_DICT = SQ_STRING_STATIC("dict");

	switch (SQ_VTAG(value)) {
	case SQ_TCONST:
		return sq_value_new_string(sq_value_is_null(value) ? &KIND_NULL : &KIND_BOOLEAN);

	case SQ_TNUMBER:
		return sq_value_new_string(&KIND_NUMBER);

	case SQ_TSTRING:
		return sq_value_new_string(&KIND_STRING);

	case SQ_TINSTANCE:
		return sq_value_new_class(sq_class_clone(sq_value_as_instance(value)->class));

	case SQ_TFUNCTION:
		return sq_value_new_string(&KIND_FUNCTION);

	case SQ_TCLASS:
		return sq_value_new_string(&KIND_CLASS);

	case SQ_TARRAY:
		return sq_value_new_string(&KIND_ARRAY);

	case SQ_TDICT:
		return sq_value_new_string(&KIND_DICT);

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

	case SQ_TARRAY:
		if (!sq_value_is_array(rhs)) return false;
		struct sq_array *lary = AS_ARRAY(lhs), *rary = AS_ARRAY(rhs);

		if (lary->len != rary->len) return false;
		for (unsigned i = 0; i < lary->len; ++i)
			if (!sq_value_eql(lary->eles[i], rary->eles[i]))
				return false;
		return true;

	case SQ_TDICT:
		if (!sq_value_is_dict(rhs))
			return false;

		struct sq_dict *ldict = AS_DICT(lhs), *rdict = AS_DICT(rhs);

		if (sq_dict_length(ldict) != sq_dict_length(rdict))
			return false;

		for (unsigned i = 0; i < sq_dict_length(ldict); ++i)
			if (!sq_value_eql(sq_dict_entry_index(ldict, i)->value, sq_dict_entry_index(rdict, i)->value))
				return false;

		return true;


	case SQ_TINSTANCE: {
		struct sq_function *eql = sq_instance_method(AS_INSTANCE(lhs), "==");
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

	case SQ_TINSTANCE:
		todo("cmp instance");

	default:
		die("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
	// 	struct sq_function *neg = sq_instance_method(AS_INSTANCE(arg), "<=>");

	// 	if (neg != NULL) return sq_function_run(neg, 1, &arg);
	// }
	}
}

sq_value sq_value_neg(sq_value arg) {
	switch (SQ_VTAG(arg)) {
	case SQ_TNUMBER:
		return sq_value_new_number(-AS_NUMBER(arg));

	case SQ_TINSTANCE: {
		struct sq_function *neg = sq_instance_method(AS_INSTANCE(arg), "-@");

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

		if (index < 0)
			index += AS_STRING(value)->length;

		if (index < 0 || AS_STRING(value)->length <= (unsigned) index)
			return SQ_NULL;

		char *c = xmalloc(sizeof(char [2]));
		c[0] = AS_STR(value)[index];
		c[1] = '\0';
		return sq_value_new_string(sq_string_new2(c, 2));
	}

	case SQ_TARRAY:
		return sq_array_index(AS_ARRAY(value), sq_value_to_number(key));

	case SQ_TDICT:
		return sq_dict_index(AS_DICT(value), key);

	case SQ_TINSTANCE: {
		struct sq_function *index = sq_instance_method(AS_INSTANCE(value), "[]");
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
	case SQ_TARRAY:
		sq_array_index_assign(AS_ARRAY(value), sq_value_to_number(key), val);
		return;

	case SQ_TDICT:
		sq_dict_index_assign(AS_DICT(value), key, val);
		return;

	case SQ_TINSTANCE: {
		struct sq_function *index_assign = sq_instance_method(AS_INSTANCE(value), "[]=");
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
	bool free_lhs = false;

	if (sq_value_is_string(rhs)) {
		free_lhs = true;
		lhs = sq_value_new_string(sq_value_to_string(lhs));
	}

	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		return sq_value_new_number(AS_NUMBER(lhs) + sq_value_to_number(rhs));

	case SQ_TSTRING: {
		struct sq_string *rstr = sq_value_to_string(rhs);
		struct sq_string *result = sq_string_alloc(
			AS_STRING(lhs)->length + rstr->length + 1
		);

		strcpy(result->ptr, AS_STR(lhs));
		strcat(result->ptr, rstr->ptr);
		sq_string_free(rstr);
		if (free_lhs) sq_value_free(lhs);
		return sq_value_new_string(result);
	}

	case SQ_TARRAY: {
		struct sq_array *lary = AS_ARRAY(lhs), *rary = sq_value_to_array(rhs);

		unsigned len = lary->len + rary->len;
		sq_value *eles = xmalloc(sizeof(sq_value[len]));

		for (unsigned i = 0; i < lary->len; ++i)
			eles[i] = sq_value_clone(lary->eles[i]);

		for (unsigned i = 0; i < rary->len; ++i)
			eles[lary->len + i] = sq_value_clone(rary->eles[i]);

		sq_array_free(rary);
		return sq_value_new_array(sq_array_new(len, eles));
	}

	case SQ_TDICT: {
		todo("'+' dicts");
		// struct sq_dict *ldict = AS_DICT(lhs), *rdict = sq_value_to_dict(rhs);

		// unsigned i = 0, len = lhs->len + rhs->len;
		// sq_value *eles = xmalloc(sizeof(sq_value[len]));

		// for (; i < lhs->len; ++i)
		// 	eles[i] = sq_value_clone(lary->eles[i]);
		// for (; i < lhs->len; ++i)
		// 	eles[i] = sq_value_clone(rary->eles[i]);

		// sq_array_free(rhs);
		// return sq_value_new_array(lary);
	}


	case SQ_TINSTANCE: {
		struct sq_function *add = sq_instance_method(AS_INSTANCE(lhs), "+");
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

	case SQ_TARRAY:
		todo("set difference");

	case SQ_TDICT:
		todo("set difference for dict");

	case SQ_TINSTANCE: {
		struct sq_function *sub = sq_instance_method(AS_INSTANCE(lhs), "-");
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
		struct sq_string *result = sq_string_alloc(AS_STRING(lhs)->length * amnt + 1);
		*result->ptr = '\0';

		for (unsigned i = 0; i < amnt; ++i)
			strcat(result->ptr, AS_STR(lhs));

		return sq_value_new_string(result);
	}

	case SQ_TARRAY:
		todo("array multiply");

	case SQ_TINSTANCE: {
		struct sq_function *mul = sq_instance_method(AS_INSTANCE(lhs), "*");
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
		if (!rnum) die("cannot divide by zero");
		return sq_value_new_number(AS_NUMBER(lhs) / rnum);
	}

	case SQ_TINSTANCE: {
		struct sq_function *div = sq_instance_method(AS_INSTANCE(lhs), "/");
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
		if (!rnum) die("cannot modulo by zero");
		return sq_value_new_number(AS_NUMBER(lhs) % rnum);
	}

	case SQ_TINSTANCE: {
		struct sq_function *mod = sq_instance_method(AS_INSTANCE(lhs), "%");
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
	static struct sq_string truestring = SQ_STRING_STATIC("true");
	static struct sq_string falsestring = SQ_STRING_STATIC("false");
	static struct sq_string nullstring = SQ_STRING_STATIC("null");

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

	case SQ_TCLASS:
		return sq_string_new(strdup(AS_CLASS(value)->name));

	case SQ_TINSTANCE: {
		struct sq_function *to_string = sq_instance_method(AS_INSTANCE(value), "to_string");

		if (to_string != NULL) {
			sq_value string = sq_function_run(to_string, 1, &value);
			if (!sq_value_is_string(string))
				die("to_string for an instance of '%s' didn't return a string", AS_INSTANCE(value)->class->name);
			return AS_STRING(string);
		}
		// else fallthrough
	}

	case SQ_TARRAY:
		return sq_array_to_string(AS_ARRAY(value));

	case SQ_TDICT:
		todo("dict to string");

	case SQ_TFUNCTION:
		die("cannot convert %s to a string", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
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

	case SQ_TINSTANCE: {
		struct sq_function *tally = sq_instance_method(AS_INSTANCE(value), "tally");

		if (tally != NULL) {
			sq_value number = sq_function_run(tally, 1, &value);
			if (!sq_value_is_number(number))
				die("tally for an instance of '%s' didn't return a number", AS_INSTANCE(value)->class->name);
			return AS_NUMBER(number);
		}
		// else fallthrough
	}

	case SQ_TARRAY:
		return sq_value_new_string(sq_array_to_string(AS_ARRAY(value)));

	case SQ_TCLASS:
	case SQ_TFUNCTION:
	case SQ_TDICT:
		die("cannot convert %s to a number", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
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

	case SQ_TARRAY:
		return AS_ARRAY(value)->len;

	case SQ_TDICT:
		return sq_dict_length(AS_DICT(value));

	case SQ_TINSTANCE: {
		struct sq_function *veracity = sq_instance_method(AS_INSTANCE(value), "veracity");

		if (veracity != NULL) {
			sq_value boolean = sq_function_run(veracity, 1, &value);
			if (!sq_value_is_boolean(boolean))
				die("veracity for an instance of '%s' didn't return a boolean", AS_INSTANCE(value)->class->name);
			return sq_value_as_boolean(boolean);
		}
		// else fallthrough
	}

	case SQ_TCLASS:
	case SQ_TFUNCTION:
		die("cannot convert %s to a boolean", TYPENAME(value));

	default:
*		(volatile int *)0;
		bug("<UNDEFINED: %lld>", value);
	}
}

size_t sq_value_length(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TARRAY:
		return sq_value_as_array(value)->len;

	case SQ_TDICT:
		return sq_dict_length(sq_value_as_dict(value));

	case SQ_TSTRING:
		return AS_STRING(value)->length;

	case SQ_TINSTANCE: {
		struct sq_function *length = sq_instance_method(AS_INSTANCE(value), "length");

		if (length != NULL) {
			sq_value boolean = sq_function_run(length, 1, &value);
			if (!sq_value_is_number(boolean))
				die("length for an instance of '%s' didn't return a boolean", AS_INSTANCE(value)->class->name);
			return AS_NUMBER(boolean);
		}
		// else fallthrough
	}

	case SQ_TCONST:
	case SQ_TNUMBER:
	case SQ_TCLASS:
	case SQ_TFUNCTION:
		die("cannot get length of %s", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
	}
}

struct sq_array *sq_value_to_array(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TARRAY:
		++AS_ARRAY(value)->refcount;
		return AS_ARRAY(value);
	default:
		todo("others to array");
	}
}

struct sq_dict *sq_value_to_dict(sq_value value) {
	(void) value;
	die("todo");
}
