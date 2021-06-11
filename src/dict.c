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
}

void sq_dixct_dump(FILE *out, const struct sq_dict *dict) {
	putc('{', out);

	for (unsigned i = 0; i < dict->len; ++i) {
		if (i) fprintf(out, ", ");

		sq_value_dump(dict->eles[i].key);
		fprintf(out, ": ");
		sq_value_dump(dict->eles[i].value);
	}

	putc('}', out);
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

struct sq_string *sq_dict_to_string(const struct sq_dict *dict);
unsigned sq_dict_length(const struct sq_dict *dict);
sq_value sq_dict_delete(struct sq_dict *dict, sq_value key);
sq_value sq_dict_index(struct sq_dict *dict, sq_value key);
void sq_dict_index_assign(struct sq_dict *dict, sq_value key, sq_value value);

