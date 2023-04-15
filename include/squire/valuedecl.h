#ifndef SQ_VALUEDECL_H
#define SQ_VALUEDECL_H

#include <stdbool.h>
#include <stdint.h>
#include <squire/basic.h>
#include <squire/attributes.h>

struct sq_text;
struct sq_form;
struct sq_imitation;
struct sq_args;
struct sq_journey;
struct sq_book;
struct sq_codex;
struct sq_other;

typedef uint64_t sq_value;
typedef bool sq_veracity;

enum sq_genus_tag {
	SQ_G_OTHER     = 0,
	SQ_G_NUMERAL   = 1,
	SQ_G_TEXT      = 2,
	SQ_G_FORM      = 3,
	SQ_G_IMITATION = 4,
	SQ_G_JOURNEY   = 5,
	SQ_G_BOOK      = 6,
	SQ_G_CODEX     = 7,
} SQ_CLOSED_ENUM;

#define SQ_TAG_FOR(kind) (_Generic(&(kind){0}, \
	struct sq_other *: SQ_G_OTHER, \
	struct sq_text *: SQ_G_TEXT, \
	struct sq_form *: SQ_G_FORM, \
	struct sq_imitation *: SQ_G_IMITATION, \
	struct sq_journey *: SQ_G_JOURNEY, \
	struct sq_book *: SQ_G_BOOK, \
	struct sq_codex *: SQ_G_CODEX \
))


#define SQ_GENUS_TAG_BITS 3
#define SQ_VSHIFT ((SQ_GENUS_TAG_BITS) + 1)
#define SQ_VMASK_BITS ((1<<SQ_VSHIFT)-1)
SQ_STATIC_ASSERT((SQ_VMASK_BITS & SQ_G_CODEX) == SQ_G_CODEX, "oops?");

#define SQ_VMASK(value, kind) ((value) | (kind))
#define SQ_VUNMASK(value) ((value) & ~SQ_VMASK_BITS)

#define SQ_VTAG(value) ((value) & SQ_VMASK_BITS) // deprecated

static inline enum sq_genus_tag sq_value_genus_tag(sq_value value) {
	return value & SQ_VMASK_BITS;
}

#define SQ_YEA SQ_VMASK((1 << SQ_VSHIFT), SQ_G_OTHER)
#define SQ_NAY SQ_VMASK((2 << SQ_VSHIFT), SQ_G_OTHER)
#define SQ_NI SQ_VMASK((0 << SQ_VSHIFT), SQ_G_OTHER)
#define SQ_UNDEFINED SQ_VMASK((3 << SQ_VSHIFT), SQ_G_OTHER)

#endif /* SQ_VALUEDECL_H */
