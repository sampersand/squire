#ifndef SQ_BASIC_H
#define SQ_BASIC_H

#include <squire/attributes.h>

#define SQ_VALUE_SIZE 64
#define SQ_VALUE_ALIGNMENT 8

#define SQ_TO_STRING_(x) #x
#define SQ_TO_STRING(x) SQ_TO_STRING_(x)

#define SQ_VALUE_ASSERT_SIZE(type) \
	SQ_STATIC_ASSERT(sizeof(type) <= SQ_VALUE_SIZE, \
		"type '" #type "' is too large (" SQ_TO_STRING_(SQ_VALUE_SIZE))

struct sq_basic {
	SQ_ALIGNAS(SQ_VALUE_ALIGNMENT) char _blank;
	bool marked: 1;
} __align;

#define SQ_BASIC_DEFAULT ((struct sq_basic) { .marked = 0 })
#define SQ_GUARD_MARK(what) do { if ((what)->basic.marked) return; (what)->basic.marked = 1; } while(0)

#endif
