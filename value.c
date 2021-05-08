#include "value.h"
#include "struct.h"
#include "function.h"
#include "string.h"

#define VALUE_KIND(value) ((value) & 7)
#define CAST(value, kind) ((kind *) ((value) & ~7))

void sq_value_clone(sq_value value) {
	switch (VALUE_KIND(value)) {
	case SQ_VK_STRING:
		sq_string_clone(CAST(value, struct sq_string));
		return;
	case SQ_VK_INSTANCE:
		sq_instance_clone(CAST(value, struct sq_instance));
		return;
	case SQ_VK_FUNCTION:
		sq_function_clone(CAST(value, struct sq_function));
		return;
	}
}

void sq_value_free(sq_value value) {
	switch (VALUE_KIND(value)) {
	case SQ_VK_STRING:
		sq_string_free(CAST(value, struct sq_string));
		return;
	case SQ_VK_INSTANCE:
		sq_instance_free(CAST(value, struct sq_instance));
		return;
	case SQ_VK_FUNCTION:
		sq_function_free(CAST(value, struct sq_function));
		return;
	}
}