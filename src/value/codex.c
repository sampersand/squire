#include <squire/codex.h>
#include <squire/shared.h>
#include <squire/text.h>

#include <string.h>

struct sq_codex *sq_codex_new(unsigned length, unsigned capacity, struct sq_codex_page *pages) {
	struct sq_codex *codex = sq_mallocv(struct sq_codex);

	codex->length = length;
	codex->capacity = capacity;

	codex->pages = pages;
	return codex;
}

struct sq_codex *sq_codex_allocate(unsigned capacity) {
	struct sq_codex *codex = sq_mallocv(struct sq_codex);

	codex->length = 0;
	codex->capacity = capacity;

	codex->pages = sq_malloc_vec(struct sq_codex_page, capacity);
	return codex;
}

void sq_codex_dump(FILE *out, const struct sq_codex *codex) {
	fputc('{', out);

	for (unsigned i = 0; i < codex->length; ++i) {
		if (i) fputs(", ", out);

		sq_value_dump(out, codex->pages[i].key);
		fputs(": ", out);
		sq_value_dump(out, codex->pages[i].value);
	}

	fputc('}', out);
}

void sq_codex_mark(struct sq_codex *codex) {
	SQ_GUARD_MARK(codex);

	for (unsigned i = 0; i < codex->length; ++i) {
		sq_value_mark(codex->pages[i].key);
		sq_value_mark(codex->pages[i].value);
	}
}

void sq_codex_deallocate(struct sq_codex *codex) {
	free(codex->pages);
	// free(codex);
}

struct sq_text *sq_codex_to_text(const struct sq_codex *codex) {
	unsigned len = 0, cap = 64;
	char *str = sq_malloc_heap(cap);
	str[len++] = '{';

	for (unsigned i = 0; i < codex->length; ++i) {
		if (i) {
			if (cap <= len + 2)
				str = sq_realloc(str, cap *= 2);
			str[len++] = ',';
			str[len++] = ' ';
		}

		struct sq_text *key = sq_value_to_text(codex->pages[i].key);
	
		if (cap <= key->length + len + 2)
			str = sq_realloc(str, cap = key->length + len * 2 + 2);
	
		memcpy(str + len, key->ptr, key->length);
		len += key->length;
		str[len++] = ':';
		str[len++] = ' ';

		struct sq_text *value = sq_value_to_text(codex->pages[i].value);
	
		if (cap <= value->length + len + 2)
			str = sq_realloc(str, cap = value->length + len * 2 + 2);
	
		memcpy(str + len, value->ptr, value->length);
		len += value->length;
	}

	str = sq_realloc(str, len + 2);
	str[len++] = '}';
	str[len] = '\0';

	return sq_text_new2(str, len);
}

struct sq_codex_page *sq_codex_fetch_page(struct sq_codex *codex, sq_value key) {
	for (unsigned i = 0; i < codex->length; ++i)
		if (sq_value_eql(codex->pages[i].key, key))
			return &codex->pages[i];

	return NULL;
}

sq_value sq_codex_delete(struct sq_codex *codex, sq_value key) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);
	sq_value result = SQ_NI;

	if (page) {
		result = page->value;

		if (codex->length != 1) {
			page->key = codex->pages[--codex->length].key;
			page->value = codex->pages[codex->length].value;
		}
		return page->value;
	}

	return result;
}

sq_value sq_codex_index(struct sq_codex *codex, sq_value key) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);
	
	if (page == NULL)
		return SQ_NI;

	return page->value;
}

void sq_codex_index_assign(struct sq_codex *codex, sq_value key, sq_value value) {
	struct sq_codex_page *page = sq_codex_fetch_page(codex, key);

	if (!page) {
		// `+1` in case it starts out with 0 length
		if (codex->capacity == codex->length) {
			codex->pages = sq_realloc_vec(
				struct sq_codex_page,
				codex->pages,
				codex->capacity = codex->capacity * 2 + 1
			);
		}
		page = &codex->pages[codex->length++];
		page->key = key;
	}

	page->value = value;
}
