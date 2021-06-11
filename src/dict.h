#ifndef SQ_DICT_H

#include "value.h"

struct sq_dict;
struct sq_dict_entry { sq_value key, value; };

struct sq_dict *sq_dict_new(unsigned len, struct sq_dict_entry *eles);
void sq_dixct_dump(FILE *, const struct sq_dict *dict);
struct sq_dict *sq_dict_clone(struct sq_dict *dict);
void sq_dict_free(struct sq_dict *dict);

struct sq_string *sq_dict_to_string(const struct sq_dict *dict);
unsigned sq_dict_length(const struct sq_dict *dict);
sq_value sq_dict_delete(struct sq_dict *dict, sq_value key);
sq_value sq_dict_index(struct sq_dict *dict, sq_value key);
void sq_dict_index_assign(struct sq_dict *dict, sq_value key, sq_value value);

#endif /* !SQ_DICT_H */
