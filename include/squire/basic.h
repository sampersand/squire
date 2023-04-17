#ifndef SQ_BASIC_H
#define SQ_BASIC_H

#include <squire/attributes.h>
#include <squire/valuedecl.h>

#define SQ_VALUE_SIZE 64
#define SQ_VALUE_ALIGNMENT 8

#define SQ_TO_STRING_(x) #x
#define SQ_TO_STRING(x) SQ_TO_STRING_(x)

#define SQ_VALUE_ASSERT_SIZE(type) \
	SQ_STATIC_ASSERT(sizeof(type) <= SQ_VALUE_SIZE, \
		"type '" #type "' is too large (" SQ_TO_STRING_(SQ_VALUE_SIZE) "max)")

struct sq_basic {
	// this field's here since we need alignment but c doesn't let you align bitfields
	SQ_ALIGNAS(SQ_VALUE_ALIGNMENT) char _blank;
	int marked: 1;
	int in_use: 1;
	enum sq_genus_tag genus: SQ_GENUS_TAG_BITS;
};

#define SQ_STATIC_BASIC(kind) ((struct sq_basic) { .marked = 0, .in_use = 1, \
	.genus = SQ_TAG_FOR(kind) })
#define SQ_GUARD_MARK(what) do { if ((what)->basic.marked) return; (what)->basic.marked = 1; } while(0)

#endif
