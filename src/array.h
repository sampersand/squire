#ifndef SQ_ARRAY_H

#include "value.h"

struct sq_array {
	unsigned len, cap, refcount;
	sq_value *eles;
};

struct sq_array *sq_array_new(unsigned len, sq_value *eles);
void sq_array_dump(FILE *, const struct sq_array *array);
struct sq_array *sq_array_clone(struct sq_array *array);
void sq_array_free(struct sq_array *array);

void sq_array_insert(struct sq_array *array, int index, sq_value value);
sq_value sq_array_delete(struct sq_array *array, int index);
sq_value sq_array_index(struct sq_array *array, int index);
void sq_array_index_assign(struct sq_array *array, int index, sq_value value);

#endif /* !SQ_ARRAY_H */
