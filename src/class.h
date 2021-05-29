#ifndef SQ_CLASS_H
#define SQ_CLASS_H

#include "value.h"
#include "function.h"
#define MAX_FIELDS 256

struct sq_class {
	int refcount;
	unsigned nfields;
	char *name, **fields;

	unsigned nfuncs, nmeths;
	struct sq_function **funcs, **meths;
};

struct sq_instance {
	int refcount;
	struct sq_class *class;
	sq_value *fields; 
};

struct sq_class *sq_class_new(char *name);
struct sq_class *sq_class_clone(struct sq_class *class);
void sq_class_free(struct sq_class *class);
void sq_class_dump(const struct sq_class *class);
const struct sq_function *sq_class_func(const struct sq_class *, const char *);

struct sq_instance *sq_instance_new(struct sq_class *class, sq_value *fields);
sq_value *sq_instance_field(struct sq_instance *instance, const char *name);
const struct sq_function *sq_instance_meth(const struct sq_instance *, const char *);

struct sq_instance *sq_instance_clone(struct sq_instance *instance);
void sq_instance_free(struct sq_instance *instance);
void sq_instance_dump(const struct sq_instance *instance);

#endif /* !SQ_CLASS_H */
