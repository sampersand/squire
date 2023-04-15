#ifndef sq_codex_H

#include <squire/value.h>

struct sq_codex {
	struct sq_basic basic;
	struct sq_codex_page *pages;
	unsigned length, capacity;
};
SQ_VALUE_ASSERT_SIZE(struct sq_codex);

struct sq_codex_page {
	sq_value key, value;
};

struct sq_codex *sq_codex_allocate(unsigned capacity);
struct sq_codex *sq_codex_new(unsigned length, unsigned capacity, struct sq_codex_page *pages);

static inline struct sq_codex *sq_codex_new2(unsigned length, struct sq_codex_page *pages) {
	return sq_codex_new(length, length, pages);
}

void sq_codex_mark(struct sq_codex *codex);
void sq_codex_deallocate(struct sq_codex *codex);

struct sq_text *sq_codex_to_text(const struct sq_codex *codex);
struct sq_codex_page *sq_codex_fetch_page(struct sq_codex *codex, sq_value key) SQ_NODISCARD;

void sq_codex_dump(FILE *out, const struct sq_codex *codex);
sq_value sq_codex_delete(struct sq_codex *codex, sq_value key);
sq_value sq_codex_index(struct sq_codex *codex, sq_value key);
void sq_codex_index_assign(struct sq_codex *codex, sq_value key, sq_value value);
#endif /* !sq_codex_H */
