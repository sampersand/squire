#include <string.h>
#include <assert.h>
#include "form.h"
#include "shared.h"

struct sq_form *sq_form_new(char *name) {
	assert(name != NULL);

	struct sq_form *form = xcalloc(1, sizeof(struct sq_form));

	form->refcount = 1;
	form->name = name;

	return form;
}

void sq_form_deallocate(struct sq_form *form) {
	assert(!form->refcount);

	for (unsigned i = 0; i < form->nessences; ++i) {
		free(form->essences[i].name);
		sq_value_free(form->essences[i].value);
	}
	free(form->essences);

	for (unsigned i = 0; i < form->nrecollections; ++i)
		sq_function_free(form->recollections[i]);
	free(form->recollections);

	for (unsigned i = 0; i < form->nmatter; ++i)
		free(form->matter_names[i]);
	free(form->matter_names);

	for (unsigned i = 0; i < form->nchanges; ++i)
		sq_function_free(form->changes[i]);

	for (unsigned i = 0; i < form->nparents; ++i)
		sq_form_free(form->parents[i]);

	sq_function_free(form->imitate);

	free(form->changes);
	free(form->parents);
	free(form->name);
	free(form);
}

struct sq_function *sq_form_lookup_recollection(struct sq_form *form, const char *name) {
	struct sq_function *recall;

	for (unsigned i = 0; i < form->nrecollections; ++i)
		if (!strcmp(name, (recall = form->recollections[i])->name))
			return recall;

	for (unsigned i = 0; i < form->nparents; ++i)
		if ((recall = sq_form_lookup_recollection(form, name)))
			return recall;

	return NULL;
}

sq_value *sq_form_lookup_essence(struct sq_form *form, const char *name) {
	for (unsigned i = 0; i < form->nessences; ++i)
		if (!strcmp(name, form->essences[i].name))
			return &form->essences[i].value;

	return NULL;
}

sq_value sq_form_lookup(struct sq_form *form, const char *name) {
	struct sq_function *recollection = sq_form_lookup_recollection(form, name);

	if (recollection != NULL)
		return sq_value_new_function(sq_function_clone(recollection));

	sq_value *essence = sq_form_lookup_essence(form, name);
	if (essence != NULL)
		return sq_value_clone(*essence);

	return SQ_UNDEFINED;
}

void sq_form_dump(FILE *out, const struct sq_form *form) {
	fprintf(out, "Form(%s:", form->name);

	for (unsigned i = 0; i < form->nmatter; ++i) {
		if (i != 0)
			putc(',', out);

		fprintf(out, " %s", form->matter_names[i]);
	}

	if (!form->nmatter)
		fprintf(out, " <none>");

	putc(')', out);
}

struct sq_imitation *sq_imitation_new(struct sq_form *form, sq_value *matter) {
	assert(form != NULL);
	assert(form->nmatter == 0 || matter != NULL);

	struct sq_imitation *imitation = xmalloc(sizeof(struct sq_imitation));

	imitation->refcount = 1;
	imitation->form = form;
	imitation->matter = matter;

	return imitation;
}

struct sq_function *sq_imitation_lookup_change(struct sq_imitation *imitation, const char *name) {
	for (unsigned i = 0; i < imitation->form->nchanges; ++i)
		if (!strcmp(name, imitation->form->changes[i]->name))
			return imitation->form->changes[i];

	return NULL;
}

sq_value *sq_imitation_lookup_matter(struct sq_imitation *imitation, const char *name) {
	for (unsigned i = 0; i < imitation->form->nmatter; ++i)
		if (!strcmp(name, imitation->form->matter_names[i]))
			return &imitation->matter[i];

	return NULL;
}

sq_value sq_imitation_lookup(struct sq_imitation *imitation, const char *name) {
	struct sq_function *change = sq_imitation_lookup_change(imitation, name);

	if (change != NULL)
		return sq_value_new_function(sq_function_clone(change));

	sq_value *matter = sq_imitation_lookup_matter(imitation, name);

	if (matter != NULL)
		return sq_value_clone(*matter);

	return SQ_UNDEFINED;
}

void sq_imitation_deallocate(struct sq_imitation *imitation) {
	assert(!imitation->refcount);

	for (unsigned i = 0; i < imitation->form->nmatter; ++i)
		sq_value_free(imitation->matter[i]);
	free(imitation->matter);

	sq_form_free(imitation->form);
	free(imitation);
}

void sq_imitation_dump(FILE *out, const struct sq_imitation *imitation) {
	fprintf(out, "%s(", imitation->form->name);

	for (unsigned i = 0; i < imitation->form->nmatter; ++i) {
		if (i != 0)
			fprintf(out, ", ");

		fprintf(out, "%s=", imitation->form->matter_names[i]);
		sq_value_dump(imitation->matter[i]);
	}

	if (!imitation->form->nmatter)
		fprintf(out, "<none>");

	putc(')', out);
}
