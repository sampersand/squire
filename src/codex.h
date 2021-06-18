#ifndef sq_codex_H

#include "value.h"

struct sq_codex {
	unsigned length, capacity, refcount;
	struct sq_codex_entry *entries;
};

struct sq_codex_entry {
	sq_value key, value;
};

struct sq_codex *sq_codex_new(unsigned length, struct sq_codex_entry *entries);
void sq_dixct_dump(FILE *, const struct sq_codex *codex);

static inline struct sq_codex *sq_codex_clone(struct sq_codex *codex) {
	++codex->refcount;
	return codex;
}

void sq_codex_deallocate(struct sq_codex *codex);

static inline void sq_codex_free(struct sq_codex *codex) {
	assert(codex->refcount);

	if (!--codex->refcount)
		sq_codex_deallocate(codex);
}

struct sq_string *sq_codex_to_string(const struct sq_codex *codex);
unsigned sq_codex_length(const struct sq_codex *codex);
struct sq_codex_entry *sq_codex_fetch_entry(struct sq_codex *codex, sq_value key);

void sq_codex_dump(FILE *out, const struct sq_codex *codex);
sq_value sq_codex_delete(struct sq_codex *codex, sq_value key);
sq_value sq_codex_index(struct sq_codex *codex, sq_value key);
void sq_codex_index_assign(struct sq_codex *codex, sq_value key, sq_value value);
#endif /* !sq_codex_H */
