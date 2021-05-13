#ifndef SQ_STRUCT_H
#define SQ_STRUCT_H

#include "value.h"
#define MAX_FIELDS 256

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
void sq_struct_clone(struct sq_struct *);
void sq_struct_free(struct sq_struct *);
void sq_struct_dump(const struct sq_struct *);

struct sq_instance *sq_instance_new(struct sq_struct *kind, sq_value *fields);
sq_value *sq_instance_field(struct sq_instance *instance, const char *name);

void sq_instance_clone(struct sq_instance *instance);
void sq_instance_free(struct sq_instance *instance);
void sq_instance_dump(const struct sq_instance *instance);

#endif /* !SQ_STRUCT_H */
