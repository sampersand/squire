#include "codex.h"
#include "shared.h"

struct sq_codex *sq_codex_new(unsigned length, struct sq_codex_entry *entries) {
	struct sq_codex *codex = xmalloc(sizeof(struct sq_codex));

	codex->length = codex->capacity = length;
	codex->refcount = 1;

	codex->entries = entries;
	return codex;
}

void sq_codex_dump(FILE *out, const struct sq_codex *codex) {
	fprintf(out, "Codex(");

	for (unsigned i = 0; i < codex->length; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(codex->entries[i].key);
		fprintf(out, ": ");
		sq_value_dump(codex->entries[i].value);
	}

	putc(')', out);
}

void sq_codex_deallocate(struct sq_codex *codex) {
	assert(!codex->refcount);

	for (unsigned i = 0; i < codex->length; ++i) {
		sq_value_free(codex->entries[i].key);
		sq_value_free(codex->entries[i].value);
	}

	free(codex->entries);
	free(codex);
}

struct sq_string *sq_codex_to_string(const struct sq_codex *codex) {
	(void) codex;
	die("todo: codex to string");
}

struct sq_codex_entry *sq_codex_fetch_entry(struct sq_codex *codex, sq_value key) {
	for (unsigned i = 0; i < codex->length; ++i)
		if (sq_value_eql(codex->entries[i].key, key))
			return &codex->entries[i];

	return NULL;
}

sq_value sq_codex_delete(struct sq_codex *codex, sq_value key) {
	struct sq_codex_entry *entry = sq_codex_fetch_entry(codex, key);

	if (entry) {
		sq_value_free(entry->key);
		entry->key = SQ_UNDEFINED;
		return entry->value;
	}

	return SQ_NULL;
}

sq_value sq_codex_index(struct sq_codex *codex, sq_value key) {
	struct sq_codex_entry *entry = sq_codex_fetch_entry(codex, key);
	
	if (entry == NULL)
		return SQ_NULL;

	return sq_value_clone(entry->value);
}

void sq_codex_index_assign(struct sq_codex *codex, sq_value key, sq_value value) {
	struct sq_codex_entry *entry = sq_codex_fetch_entry(codex, key);

	if (!entry) {
		// `+1` in case it starts out with 0 length
		if (codex->capacity == codex->length)
			codex->entries = xrealloc(codex->entries, sizeof(struct sq_codex[codex->capacity = codex->capacity * 2 + 1]));
		entry = &codex->entries[codex->length++];
		entry->key = key;
		return;
	} else {
		sq_value_free(entry->value);
	}

	entry->value = value;
}
