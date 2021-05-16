#pragma once
#include "value.h"
#include "bytecode.h"

struct sq_array {
	unsigned len, cap, refcount;
	sq_value *data;
};

void sq_array_dump(const struct sq_array *array);
struct sq_array *sq_array_clone(struct sq_array *array);
void sq_array_free(struct sq_array *array);

// void sq_array