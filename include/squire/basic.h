#ifndef SQ_BASIC_H
#define SQ_BASIC_H

#include <squire/attributes.h>

#define SQ_VALUE_SIZE 64
#define SQ_VALUE_ALIGNMENT 8

#define SQ_VALUE_ASSERT_SIZE(type) \
	SQ_STATIC_ASSERT(sizeof(type) <= SQ_VALUE_SIZE, "type '" #type "' is too large")

struct sq_basic {
	SQ_ALIGNAS(SQ_VALUE_ALIGNMENT) int flags;
};

#endif
