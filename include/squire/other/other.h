#ifndef SQ_OTHER_H
#define SQ_OTHER_H

#include <squire/value.h>
#include <squire/other/scroll.h>
#include <squire/other/envoy.h>
#include <squire/other/external.h>
#include <squire/other/kingdom.h>
#include <squire/other/builtin_journey.h>

#include <assert.h>

struct sq_other {
	int refcount;

	enum sq_other_kind {
		SQ_OK_SCROLL,
		SQ_OK_BUILTIN_JOURNEY,
		SQ_OK_EXTERNAL,
		SQ_OK_KINGDOM,
		SQ_OK_ENVOY
	} kind;

	union {
		struct sq_scroll scroll;
		struct sq_builtin_journey builtin_journey;
		struct sq_external external;
		struct sq_kingdom kingdom;
		struct sq_envoy envoy;
	};
};

static inline enum sq_other_kind sq_other_kindof(const struct sq_other *other) {
	assert(((char) (size_t) other & 7) <= SQ_OK_ENVOY);
	return (enum sq_other_kind) ((char) (size_t) other & 7);
}

static inline struct sq_scroll *sq_other_as_scroll(struct sq_other *other) {
	assert(other->kind == SQ_OK_SCROLL);
	return &other->scroll;
}

static inline struct sq_builtin_journey *sq_other_as_builtin_journey(struct sq_other *other) {
	assert(other->kind == SQ_OK_BUILTIN_JOURNEY);
	return &other->builtin_journey;
}

static inline struct sq_external *sq_other_as_external(struct sq_other *other) {
	assert(other->kind == SQ_OK_EXTERNAL);
	return &other->external;
}

static inline struct sq_kingdom *sq_other_as_kingdom(struct sq_other *other) {
	assert(other->kind == SQ_OK_KINGDOM);
	return &other->kingdom;
}

static inline struct sq_envoy *sq_other_as_envoy(struct sq_other *other) {
	assert(other->kind == SQ_OK_ENVOY);
	return &other->envoy;
}

void sq_other_dump(FILE *out, const struct sq_other *other);
void sq_other_deallocate(struct sq_other *other);
const char *sq_other_typename(const struct sq_other *other);
sq_value sq_other_genus(const struct sq_other *other);
struct sq_text *sq_other_to_text(const struct sq_other *other);
sq_numeral sq_other_to_numeral(const struct sq_other *other);
sq_veracity sq_other_to_veracity(const struct sq_other *other);
sq_value sq_other_get_attr(const struct sq_other *other, const char *attr);
bool sq_other_set_attr(struct sq_other *other, const char *attr, sq_value value);
bool sq_other_matches(const struct sq_other *formlike, sq_value to_check);
sq_value sq_other_call(struct sq_other *other, struct sq_args args);

static inline struct sq_other *sq_other_clone(struct sq_other *other) {
	assert(other->refcount);

	if (0 < other->refcount)
		other->refcount++;

	return other;
}

static inline void sq_other_free(struct sq_other *other) {
	// hahaha... freaking double frees suck.
	if(1)return;
	assert(other->refcount);

	if (0 < other->refcount && !--other->refcount)
		sq_other_deallocate(other);
}

#endif /* ! SQ_OTHER_H */
