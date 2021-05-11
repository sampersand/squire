#include "value.h"
#include "struct.h"
#include "function.h"
#include "shared.h"
#include "string.h"
#include <string.h>

#define IS_STRING sq_value_is_string
#define AS_STRING sq_value_as_string
#define AS_NUMBER sq_value_as_number
#define AS_STRUCT sq_value_as_struct
#define AS_INSTANCE sq_value_as_instance
#define AS_FUNCTION sq_value_as_function
#define TYPENAME sq_value_typename
#define AS_STR(c) (AS_STRING(c)->ptr)

void sq_value_dump(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOLEAN:
		printf("Boolean(%s)", value == SQ_TRUE ? "true" : "false");
		break;
	case SQ_TNULL:
		printf("Null()");
		break;
	case SQ_TNUMBER:
		printf("Number(%lld)", AS_NUMBER(value));
		break;
	case SQ_TSTRING:
		printf("String(%s)", AS_STR(value));
		break;
	case SQ_TSTRUCT: {
		struct sq_struct *struct_ = AS_STRUCT(value);
		printf("Struct(%s:", struct_->name);
		for (unsigned i = 0; i < struct_->nfields; ++i) {
			if (i) printf(", ");
			printf("'%s'", struct_->fields[i]);
		}
		if (!struct_->nfields) printf("<no fields>");
		printf(")");
		break;
	}
	case SQ_TINSTANCE: {
		struct sq_instance *instance = AS_INSTANCE(value);

		printf("Instance(%s: ", instance->kind->name);
		for (unsigned i = 0; i < instance->kind->nfields; ++i) {
			if (i) printf(", ");
			printf("'%s'=", instance->kind->fields[i]);
			sq_value_dump(instance->fields[i]);
		}
		if (!instance->kind->nfields) printf("<no fields>");
		printf(")");
		break;
	}
	case SQ_TFUNCTION: {
		struct sq_function *function = AS_FUNCTION(value);
		printf("Function(%s, %d args)", function->name, function->argc);
		break;
	}
	default:
		printf("<UNDEFINED: %lld>", value);
	}
}

void sq_value_clone(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		sq_string_clone(AS_STRING(value));
		return;
	case SQ_TINSTANCE:
		sq_instance_clone(AS_INSTANCE(value));
		return;
	case SQ_TFUNCTION:
		sq_function_clone(AS_FUNCTION(value));
		return;
	}
}

void sq_value_free(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TSTRING:
		sq_string_free(AS_STRING(value));
		return;
	case SQ_TINSTANCE:
		sq_instance_free(AS_INSTANCE(value));
		return;
	case SQ_TFUNCTION:
		sq_function_free(AS_FUNCTION(value));
		return;
	}
}

const char *sq_value_typename(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOLEAN: return "boolean";
	case SQ_TNULL: return "null";
	case SQ_TNUMBER: return "number";
	case SQ_TSTRING: return "string";
	case SQ_TINSTANCE: return "object";
	case SQ_TFUNCTION: return "function";
	case SQ_TSTRUCT: return "struct";
	default: die("unknown tag '%d'", (int) SQ_VTAG(value));
	}
}

bool sq_value_not(sq_value arg) {
	return sq_value_to_boolean(arg) == SQ_FALSE;
}

bool sq_value_eql(sq_value lhs, sq_value rhs) {
	if (lhs == rhs) return true;

	if (!IS_STRING(lhs) || !IS_STRING(rhs))
		return false;

	return !strcmp(AS_STR(lhs), AS_STR(rhs));
}

sq_number sq_value_cmp(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		if (!sq_value_is_number(rhs))
			break;
		return AS_NUMBER(lhs) - AS_NUMBER(rhs);
	case SQ_TSTRING:
		if (!sq_value_is_string(rhs))
			break;
		return strcmp(AS_STR(lhs), AS_STR(rhs));
	}

	die("cannot compare '%s' with '%s'", TYPENAME(lhs), TYPENAME(rhs));
}

sq_value sq_value_neg(sq_value arg) {
	if (!sq_value_is_number(arg))
		die("cannot numerically negate '%s'", TYPENAME(arg));

	return sq_value_new_number(-AS_NUMBER(arg));
}

sq_value sq_value_add(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		if (!sq_value_is_number(rhs))
			break;
		return sq_value_new_number(AS_NUMBER(lhs) + AS_NUMBER(rhs));

	case SQ_TSTRING:
		if (!sq_value_is_string(rhs))
			break;

		struct sq_string *result = sq_string_alloc(
			AS_STRING(lhs)->length + AS_STRING(rhs)->length + 1
		);

		strcpy(result->ptr, AS_STR(lhs));
		strcat(result->ptr, AS_STR(rhs));
		return sq_value_new_string(result);
	}

	die("cannot add '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));
}

sq_value sq_value_sub(sq_value lhs, sq_value rhs) {
	if (!sq_value_is_number(lhs) && !sq_value_is_number(rhs))
		die("cannot subtract '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));

	return sq_value_new_number(AS_NUMBER(lhs) - AS_NUMBER(rhs));
}

sq_value sq_value_mul(sq_value lhs, sq_value rhs) {
	switch (SQ_VTAG(lhs)) {
	case SQ_TNUMBER:
		if (!sq_value_is_number(rhs))
			break;
		return sq_value_new_number(AS_NUMBER(lhs) * AS_NUMBER(rhs));

	case SQ_TSTRING:
		if (!sq_value_is_number(rhs))
			break;

		struct sq_string *result = sq_string_alloc(
			AS_STRING(lhs)->length * AS_NUMBER(rhs) + 1
		);
		*result->ptr = '\0';

		for (unsigned i = 0; i < AS_NUMBER(rhs); ++i)
			strcat(result->ptr, AS_STR(lhs));

		return sq_value_new_string(result);
	}

	die("cannot multiply '%s' to '%s'", TYPENAME(lhs), TYPENAME(rhs));

}

sq_value sq_value_div(sq_value lhs, sq_value rhs) {
	if (!sq_value_is_number(lhs) && !sq_value_is_number(rhs))
		die("cannot divide '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));

	if (!AS_NUMBER(rhs)) die("cannot divide by zero");

	return sq_value_new_number(AS_NUMBER(lhs) / AS_NUMBER(rhs));

}

sq_value sq_value_mod(sq_value lhs, sq_value rhs) {
	if (!sq_value_is_number(lhs) && !sq_value_is_number(rhs))
		die("cannot modulo '%s' from '%s'", TYPENAME(lhs), TYPENAME(rhs));

	if (!AS_NUMBER(rhs)) die("cannot modulo by zero");

	return sq_value_new_number(AS_NUMBER(lhs) % AS_NUMBER(rhs));
}

struct sq_string *sq_value_to_string(sq_value value) {
	static struct sq_string truestring = { "true", -1, 4 };
	static struct sq_string falsestring = { "false", -1, 5 };
	static struct sq_string nullstring = { "null", -1, 4 };

	switch (SQ_VTAG(value)) {
	case SQ_TBOOLEAN:
		return value == SQ_TRUE ? &truestring : &falsestring;

	case SQ_TNULL:
		return &nullstring;

	case SQ_TNUMBER: {
		char *buf = xmalloc(40);
		snprintf(buf, 40, "%lld", AS_NUMBER(value));
		return sq_string_new(buf);
	}

	case SQ_TSTRING:
		sq_string_clone(AS_STRING(value));
		return AS_STRING(value);

	case SQ_TSTRUCT:
	case SQ_TINSTANCE:
	case SQ_TFUNCTION:
		die("cannot convert %s to a string", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
	}
}

sq_number sq_value_to_number(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOLEAN:
		return value == SQ_TRUE ? 1 : 0;

	case SQ_TNULL:
		return 0;

	case SQ_TNUMBER:
		return AS_NUMBER(value);

	case SQ_TSTRING:
		return strtoll(AS_STR(value), NULL, 10);

	case SQ_TSTRUCT:
	case SQ_TINSTANCE:
	case SQ_TFUNCTION:
		die("cannot convert %s to a number", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
	}
}

bool sq_value_to_boolean(sq_value value) {
	switch (SQ_VTAG(value)) {
	case SQ_TBOOLEAN:
		return value;

	case SQ_TNULL:
		return SQ_FALSE;

	case SQ_TNUMBER:
		return AS_NUMBER(value) ? SQ_TRUE : SQ_FALSE;

	case SQ_TSTRING:
		return *AS_STR(value) ? SQ_TRUE : SQ_FALSE;

	case SQ_TSTRUCT:
	case SQ_TINSTANCE:
	case SQ_TFUNCTION:
		die("cannot convert %s to a boolean", TYPENAME(value));

	default:
		bug("<UNDEFINED: %lld>", value);
	}
}
