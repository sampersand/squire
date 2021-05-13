#include <string.h>
#include <assert.h>
#include "struct.h"
#include "shared.h"

struct sq_struct *
sq_struct_new(char *name, unsigned nfields, char **fields)
{
	assert(name != NULL);
	assert(nfields == 0 || fields != NULL);

	struct sq_struct *struct_ = xmalloc(sizeof(struct sq_struct));

	struct_->nfields = nfields;
	struct_->refcount = 1;
	struct_->name = name;
	struct_->fields = fields;

	return struct_;
}

void
sq_struct_clone(struct sq_struct *struct_)
{
	assert(struct_->refcount);

	if (0 < struct_->refcount)
		++struct_->refcount;
}

void
sq_struct_free(struct sq_struct *struct_)
{
	assert(struct_->refcount);

	if (struct_->refcount < 0 || --struct_->refcount)
		return;

	for (unsigned i = 0; i < struct_->nfields; ++i)
		free(struct_->fields[i]);

	free(struct_->name);
	free(struct_);
}

void
sq_struct_dump(const struct sq_struct *struct_) {
	printf("Struct(%s:", struct_->name);

	for (unsigned i = 0; i < struct_->nfields; ++i) {
		if (i != 0)
			putchar(',');

		printf(" %s", struct_->fields[i]);
	}

	if (!struct_->nfields)
		printf(" <no fields>");

	putchar(')');
}

struct sq_instance *
sq_instance_new(struct sq_struct *kind, sq_value *fields)
{
	assert(kind != NULL);
	assert(kind->nfields == 0 || fields != NULL);

	struct sq_instance *instance = xmalloc(sizeof(struct sq_instance));

	++kind->refcount;

	instance->refcount = 1;
	instance->kind = kind;
	instance->fields = fields;

	return instance;
}

sq_value *
sq_instance_field(struct sq_instance *instance, const char *name)
{
	for (unsigned i = 0; i < instance->kind->nfields; ++i)
		if (!strcmp(name, instance->kind->fields[i]))
			return &instance->fields[i];

	return NULL;
}

void
sq_instance_clone(struct sq_instance *instance)
{
	assert(instance->refcount);

	if (0 < instance->refcount)
		++instance->refcount;
}

void
sq_instance_free(struct sq_instance *instance)
{
	assert(instance->refcount);

	if (0 < instance->refcount || --instance->refcount)
		return;

	for (unsigned i = 0; i < instance->kind->nfields; ++i)
		sq_value_free(instance->fields[i]);

	sq_struct_free(instance->kind);
}

void
sq_instance_dump(const struct sq_instance *instance)
{
	printf("Instance(%s:", instance->kind->name);

	for (unsigned i = 0; i < instance->kind->nfields; ++i) {
		if (i != 0)
			putchar(',');

		printf(" %s=", instance->kind->fields[i]);
		sq_value_dump(instance->fields[i]);
	}

	if (!instance->kind->nfields)
		printf(" <no fields>");

	printf(")");
}
