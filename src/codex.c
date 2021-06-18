#include "codex.h"
#include "shared.h"

struct sq_codex *sq_codex_new(unsigned length, unsigned capacity, struct sq_codex_page *pages) {
	struct sq_codex *codex = xmalloc(sizeof(struct sq_codex));

	codex->length = length;
	codex->capacity = capacity;
	codex->refcount = 1;

	codex->pages = pages;
	return codex;
}

void sq_codex_dump(FILE *out, const struct sq_codex *codex) {
	fprintf(out, "Codex(");

	for (unsigned i = 0; i < codex->length; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(codex->pages[i].key);
		fprintf(out, ": ");
		sq_value_dump(codex->pages[i].value);
	}

	putc(')', out);
}

void sq_codex_deallocate(struct sq_codex *codex) {
	assert(!codex->refcount);

	for (unsigned i = 0; i < codex->length; ++i) {
		sq_value_free(codex->pages[i].key);
		sq_value_free(codex->pages[i].value);
	}

	free(codex->pages);
	free(codex);
}

struct sq_string *sq_codex_to_string(const struct sq_codex *codex) {
	(void) codex;
	die("todo: codex to string");
}

struct sq_codex_page *sq_codex_fetch_page(struct sq_codex *codex, sq_value key) {
	for (unsigned i = 0; i < codex->length; ++i)
		if (sq_value_eql(codex->pages[i].key, key))
			return &codex->pages[i];

	return NULL;
}

sq_value sq_codex_delete(struct sq_codex *codex, sq_value key) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);

	if (page) {
		sq_value_free(page->key);
		page->key = SQ_UNDEFINED;
		return page->value;
	}

	return SQ_NULL;
}

sq_value sq_codex_index(struct sq_codex *codex, sq_value key) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);
	
	if (page == NULL)
		return SQ_NULL;

	return sq_value_clone(page->value);
}

void sq_codex_index_assign(struct sq_codex *codex, sq_value key, sq_value value) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);

	if (!page) {
		// `+1` in case it starts out with 0 length
		if (codex->capacity == codex->length)
			codex->pages = xrealloc(codex->pages, sizeof(struct sq_codex[codex->capacity = codex->capacity * 2 + 1]));
		page = &codex->pages[codex->length++];
		page->key = key;
	} else {
		sq_value_free(page->value);
	}

	page->value = value;
}
