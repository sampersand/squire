#ifndef SQ_BASIC_H
#define SQ_BASIC_H

#include <squire/attributes.h>
#include <squire/sqassert.h>
#include <squire/utils.h>
#include <squire/valuedecl.h>

#define SQ_VALUE_SIZE 32
#define SQ_VALUE_ALIGNMENT 8

#define SQ_VALUE_ASSERT_SIZE(type) \
	SQ_STATIC_ASSERT(sizeof(type) <= SQ_VALUE_SIZE, \
		"type '" #type "' is too large (" SQ_TO_STRING(SQ_VALUE_SIZE) " bytes max)")

struct sq_basic {
	unsigned marked: 1;
	unsigned in_use: 1;
	unsigned user1: 1;
	unsigned user2: 1;
	enum sq_genus_tag genus: SQ_GENUS_TAG_BITS;
};

#define SQ_BASIC_DECLARATION SQ_ALIGNAS(SQ_VALUE_ALIGNMENT) struct sq_basic

#define SQ_STATIC_BASIC(kind) ((struct sq_basic) { .marked = 0, .in_use = 1, \
	.genus = SQ_TAG_FOR(kind) })
#define SQ_GUARD_MARK(what) do { if ((what)->basic.marked) return; (what)->basic.marked = 1; } while(0)

#endif
