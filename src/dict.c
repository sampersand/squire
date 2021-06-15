#include "dict.h"
#include "shared.h"

struct sq_dict {
	unsigned len, cap, refcount;
	struct sq_dict_entry *eles;
};

struct sq_dict *sq_dict_new(unsigned len, struct sq_dict_entry *eles) {
	struct sq_dict *dict = xmalloc(sizeof(struct sq_dict));
	dict->len = dict->cap = len;
	dict->refcount = 1;
	dict->eles = eles;
	return dict;
}

void sq_dict_dump(FILE *out, const struct sq_dict *dict) {
	fprintf(out, "Codex(");

	for (unsigned i = 0; i < dict->len; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(dict->eles[i].key);
		fprintf(out, ": ");
		sq_value_dump(dict->eles[i].value);
	}

	putc(')', out);
}

struct sq_dict *sq_dict_clone(struct sq_dict *dict) {
	++dict->refcount;
	return dict;
}

void sq_dict_free(struct sq_dict *dict) {
	if (--dict->refcount) return;

	for (unsigned i = 0; i < dict->len; ++i) {
		sq_value_free(dict->eles[i].key);
		sq_value_free(dict->eles[i].value);
	}

	free(dict->eles);
	free(dict);
}

struct sq_string *sq_dict_to_string(const struct sq_dict *dict) {
	(void) dict;
	die("todo: codex to string");
}

unsigned sq_dict_length(const struct sq_dict *dict) {
	return dict->len;
}

struct sq_dict_entry *sq_dict_fetch_entry(struct sq_dict *dict, sq_value key) {
	for (unsigned i = 0; i < dict->len; ++i)
		if (sq_value_eql(dict->eles[i].key, key))
			return &dict->eles[i];

	return NULL;
}

sq_value sq_dict_delete(struct sq_dict *dict, sq_value key) {
	struct sq_dict_entry *entry = sq_dict_fetch_entry(dict, key);

	if (entry) {
		sq_value_free(entry->key);
		entry->key = SQ_UNDEFINED;
		return entry->value;
	}

	return SQ_NULL;
}

sq_value sq_dict_index(struct sq_dict *dict, sq_value key) {
	struct sq_dict_entry *entry = sq_dict_fetch_entry(dict, key);
	
	if (entry == NULL)
		return SQ_NULL;

	return sq_value_clone(entry->value);
}


struct sq_dict_entry *sq_dict_entry_index(struct sq_dict *dict, unsigned index) {
	assert(index <= sq_dict_length(dict));

	return &dict->eles[index];
}

void sq_dict_index_assign(struct sq_dict *dict, sq_value key, sq_value value) {
	struct sq_dict_entry *entry = sq_dict_fetch_entry(dict, key);

	if (!entry) {
		// `+1` in case it starts out with 0 len
		if (dict->cap == dict->len)
			dict->eles = xrealloc(dict->eles, sizeof(struct sq_dict[dict->cap = dict->cap * 2 + 1]));
		entry = &dict->eles[dict->len++];
		entry->key = key;
		return;
	} else {
		sq_value_free(entry->value);
	}

	entry->value = value;
}
