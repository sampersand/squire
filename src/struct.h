#pragma once
#include "value.h"

struct sq_struct {
	int refcount;
	unsigned nfields;
	char *name, **fields;
};

struct sq_instance {
	int refcount;
	struct sq_struct *kind;
	sq_value *fields; 
};

struct sq_struct *sq_struct_new(char *name, unsigned nfields, char **fields);
const char *sq_struct_name(const struct sq_struct *struct_);
void sq_struct_clone(struct sq_struct *);
void sq_struct_free(struct sq_struct *);

struct sq_instance *sq_instance_new(struct sq_struct *kind, sq_value *fields);
const struct sq_struct *sq_instance_kind(const struct sq_instance *instance);
const char *sq_instance_name(const struct sq_instance *instance);
sq_value *sq_instance_field(struct sq_instance *instance, const char *name);

void sq_instance_clone(struct sq_instance *instance);
void sq_instance_free(struct sq_instance *instance);

