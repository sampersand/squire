#include <string.h>
#include <assert.h>
#include "class.h"
#include "shared.h"

struct sq_class *sq_class_new(char *name) {
	struct sq_class *class = xmalloc(sizeof(struct sq_class));

	class->refcount = 1;
	class->name = name;

	class->nfields = 0;
	class->nmeths = 0;
	class->nfuncs = 0;
	class->nparents = 0;

	class->fields = NULL;
	class->funcs = NULL;
	class->meths = NULL;
	class->parents = NULL;

	return class;
}

struct sq_class *sq_class_clone(struct sq_class *class) {
	assert(class->refcount);

	if (0 < class->refcount)
		++class->refcount;

	return class;
}

void sq_class_free(struct sq_class *class) {
	assert(class->refcount);

	if (class->refcount < 0 || --class->refcount)
		return;

	for (unsigned i = 0; i < class->nfields; ++i)
		free(class->fields[i]);

	for (unsigned i = 0; i < class->nparents; ++i)
		sq_class_free(class->parents[i]);

	free(class->name);
	free(class);
}

void sq_class_dump(FILE *out, const struct sq_class *class) {
	fprintf(out, "Form(%s:", class->name);

	for (unsigned i = 0; i < class->nfields; ++i) {
		if (i != 0)
			putc(',', out);

		fprintf(out, " %s", class->fields[i]);
	}

	if (!class->nfields)
		fprintf(out, " <none>");

	putc(')', out);
}

sq_value sq_class_field(struct sq_class *class, const char *name) {
	for (unsigned i = 0; i < class->nfuncs; ++i)
		if (!strcmp(class->funcs[i]->name, name))
			return sq_value_new_function(sq_function_clone(class->funcs[i]));

	sq_value value = SQ_UNDEFINED;

	for (unsigned i = 0; i < class->nparents; ++i)
		if ((value = sq_class_field(class, name)) != SQ_UNDEFINED)
			break;

	return value;
}

struct sq_instance *sq_instance_new(struct sq_class *class, sq_value *fields) {
	assert(class != NULL);
	assert(class->nfields == 0 || fields != NULL);

	struct sq_instance *instance = xmalloc(sizeof(struct sq_instance));

	++class->refcount;

	instance->refcount = 1;
	instance->class = class;
	instance->fields = fields;

	return instance;
}

sq_value *sq_instance_field(struct sq_instance *instance, const char *name) {
	for (unsigned i = 0; i < instance->class->nfields; ++i)
		if (!strcmp(name, instance->class->fields[i]))
			return &instance->fields[i];

	return NULL;
}


static struct sq_function *instance_method(struct sq_class *class, const char *name) {
	struct sq_function *meth;

	for (unsigned i = 0; i < class->nmeths; ++i)
		if (!strcmp((meth=class->meths[i])->name, name)) return meth;

	for (unsigned i = 0; i < class->nparents; ++i)
		if ((meth = instance_method(class, name)) != NULL) return meth;

	return NULL;
}

struct sq_function *sq_instance_method(struct sq_instance *instance, const char *name) {
	return instance_method(instance->class, name);
}

struct sq_instance *sq_instance_clone(struct sq_instance *instance) {
	assert(instance->refcount);

	if (0 < instance->refcount)
		++instance->refcount;

	return instance;
}

void sq_instance_free(struct sq_instance *instance) {
	assert(instance->refcount);

	if (0 < instance->refcount || --instance->refcount)
		return;

	for (unsigned i = 0; i < instance->class->nfields; ++i)
		sq_value_free(instance->fields[i]);

	sq_class_free(instance->class);
}

void sq_instance_dump(FILE *out, const struct sq_instance *instance) {
	fprintf(out, "%s(", instance->class->name);

	for (unsigned i = 0; i < instance->class->nfields; ++i) {
		if (i != 0)
			fprintf(out, ", ");

		fprintf(out, "%s=", instance->class->fields[i]);
		sq_value_dump(instance->fields[i]);
	}

	if (!instance->class->nfields)
		fprintf(out, "<none>");

	putc(')', out);
}
