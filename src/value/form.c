#include <squire/form.h>
#include <squire/shared.h>
#include <squire/value.h>

#include <string.h>
#include <assert.h>

struct sq_form *sq_form_new(char *name) {
	sq_assert_nn(name);

	struct sq_form *form = sq_mallocv(struct sq_form);

	form->vt = sq_mallocz(struct sq_form_vtable);
	form->vt->name = name;

	return form;
}

void sq_form_mark(struct sq_form *form) {
	SQ_GUARD_MARK(form);

	for (unsigned i = 0; i < form->vt->nessences; ++i) {
		sq_value_mark(form->vt->essences[i].value);

		if (form->vt->essences[i].genus != SQ_UNDEFINED)
			sq_value_mark(form->vt->essences[i].genus);
	}

	for (unsigned i = 0; i < form->vt->nrecollections; ++i) 
		sq_journey_mark(form->vt->recollections[i]);

	for (unsigned i = 0; i < form->vt->nchanges; ++i)
		sq_journey_mark(form->vt->recollections[i]);

	if (form->vt->imitate != NULL)
		sq_journey_mark(form->vt->imitate);

	for (unsigned i = 0; i < form->vt->nparents; ++i)
		sq_form_mark(form->vt->parents[i]);
}

void sq_form_deallocate(struct sq_form *form) {
	for (unsigned i = 0; i < form->vt->nessences; ++i)
		free(form->vt->essences[i].name);

	for (unsigned i = 0; i < form->vt->nmatter; ++i)
		free(form->vt->matter[i].name);

	free(form->vt->name);
	free(form->vt->essences);
	free(form->vt->matter);
	free(form->vt->changes);
	free(form->vt->recollections);
	free(form->vt->parents);
	free(form->vt);
}

struct sq_journey *sq_form_lookup_recollection(struct sq_form *form, const char *name) {
	struct sq_journey *recall;

	for (unsigned i = 0; i < form->vt->nrecollections; ++i)
		if (!strcmp(name, (recall = form->vt->recollections[i])->name))
			return recall;

	for (unsigned i = 0; i < form->vt->nparents; ++i)
		if ((recall = sq_form_lookup_recollection(form->vt->parents[i], name)))
			return recall;

	return NULL;
}

struct sq_essence *sq_form_lookup_essence(struct sq_form *form, const char *name) {
	for (unsigned i = 0; i < form->vt->nessences; ++i)
		if (!strcmp(name, form->vt->essences[i].name))
			return &form->vt->essences[i];

	struct sq_essence *essence;
	for (unsigned i = 0; i < form->vt->nparents; ++i)
		if ((essence = sq_form_lookup_essence(form->vt->parents[i], name)))
			return essence;

	return NULL;
}

sq_value sq_form_get_attr(struct sq_form *form, const char *attr) {
	struct sq_journey *recall = sq_form_lookup_recollection(form, attr);

	if (recall != NULL)
		return sq_value_new_journey(recall);

	struct sq_essence *essence = sq_form_lookup_essence(form, attr);
	if (essence != NULL)
		return essence->value;

	return SQ_UNDEFINED;
}

bool sq_form_set_attr(struct sq_form *form, const char *attr, sq_value value) {
	struct sq_essence *essence = sq_form_lookup_essence(form, attr);

	if (essence == NULL)
		return false;

	if (essence->genus != SQ_UNDEFINED && !sq_value_matches(essence->genus, value))
		sq_throw("essence didnt match!");

	essence->value = value;

	return true;
}

static bool is_form_a_parent_of(const struct sq_form *parent, const struct sq_form *child) {
	if (parent == child) return true;

	for (unsigned i = 0; i < child->vt->nparents; ++i)
		if (is_form_a_parent_of(parent, child->vt->parents[i]))
			return true;

	return false;
}

bool sq_form_is_parent_of(const struct sq_form *form, sq_value value) {
	if (!sq_value_is_imitation(value))
		return false;

	return is_form_a_parent_of(form, sq_value_as_imitation(value)->form);
}


void sq_form_dump(FILE *out, const struct sq_form *form) {
	fprintf(out, "Form(%s:", form->vt->name);

	for (unsigned i = 0; i < form->vt->nmatter; ++i) {
		if (i != 0)
			fputc(',', out);

		fprintf(out, " %s", form->vt->matter[i].name);
		if (form->vt->matter[i].genus != SQ_UNDEFINED) {
			fprintf(out, " (");
			sq_value_dump(out, form->vt->matter[i].genus);
			fputc(')', out);
		}
	}

	if (!form->vt->nmatter)
		fprintf(out, " <none>");

	fputc(')', out);
}

struct sq_imitation *sq_form_imitate(struct sq_form *form, struct sq_args args) {
	struct sq_imitation *imitation = sq_mallocv(struct sq_imitation);

	imitation->form = form;

	if (!form->vt->imitate) {
		if (args.pargc != form->vt->nmatter)
			sq_throw("argument count mismatch: expected %u, given %u", form->vt->nmatter, args.pargc);

		for (unsigned i = 0; i < args.pargc; ++i)
			if (form->vt->matter[i].genus != SQ_UNDEFINED && !sq_value_matches(form->vt->matter[i].genus, args.pargv[i]))
				sq_throw("type error in constructor");

		imitation->matter = sq_memdup(args.pargv, sq_sizeof_array(sq_value, form->vt->nmatter));
	} else {
		imitation->matter = sq_malloc_vec(sq_value, form->vt->nmatter);

		for (unsigned i = 0; i < form->vt->nmatter; ++i)
			imitation->matter[i]=  SQ_NI;

		sq_value *fn_args = sq_malloc_vec(sq_value, args.pargc + 1);
		fn_args[0] = sq_value_new_imitation(imitation);
		memcpy(fn_args + 1, args.pargv, sq_sizeof_array(sq_value, args.pargc));

		sq_journey_run_deprecated(form->vt->imitate, args.pargc + 1, fn_args);
		free(fn_args);
	}

	return imitation;
}

struct sq_imitation *sq_imitation_new(struct sq_form *form, sq_value *matter) {
	sq_assert_n(form);
	sq_assert(form->vt->nmatter == 0 || matter != NULL);

	struct sq_imitation *imitation = sq_mallocv(struct sq_imitation);

	imitation->form = form;
	imitation->matter = matter;

	return imitation;
}

static struct sq_journey *sq_form_lookup_change(struct sq_form *form, const char *name) {
	struct sq_journey *change;

	for (unsigned i = 0; i < form->vt->nchanges; ++i)
		if (!strcmp(name, (change = form->vt->changes[i])->name))
			return change;

	for (unsigned i = 0; i < form->vt->nparents; ++i)
		if ((change = sq_form_lookup_change(form->vt->parents[i], name)))
			return change;

	return NULL;
}

struct sq_journey *sq_imitation_lookup_change(struct sq_imitation *imitation, const char *name) {
	return sq_form_lookup_change(imitation->form, name);
}

static int sq_imitation_lookup_matter_index(struct sq_imitation *imitation, const char *name) {
	// note that we don't ask parents for matter. this is intentional, as only the base form can
	// have matter.
	for (unsigned i = 0; i < imitation->form->vt->nmatter; ++i)
		if (!strcmp(name, imitation->form->vt->matter[i].name))
			return i;

	return -1;
}

sq_value *sq_imitation_lookup_matter(struct sq_imitation *imitation, const char *name) {
	int index = sq_imitation_lookup_matter_index(imitation, name);

	return index < 0 ? NULL : &imitation->matter[index];
}

sq_value sq_imitation_get_attr(struct sq_imitation *imitation, const char *name) {
	struct sq_journey *change = sq_imitation_lookup_change(imitation, name);

	if (change != NULL)
		return sq_value_new_journey(change);

	sq_value *matter = sq_imitation_lookup_matter(imitation, name);
	if (matter != NULL)
		return *matter;

	return SQ_UNDEFINED;
}

bool sq_imitation_set_attr(struct sq_imitation *imitation, const char *attr, sq_value value) {
	int index = sq_imitation_lookup_matter_index(imitation, attr);

	if (index < 0) return false;

	if (imitation->form->vt->matter[index].genus != SQ_UNDEFINED && !sq_value_matches(imitation->form->vt->matter[index].genus, value))
		sq_throw("matter didnt match!");

	imitation->matter[index] = value;

	return true;
}

void sq_imitation_mark(struct sq_imitation *imitation) {
	SQ_GUARD_MARK(imitation);

	sq_form_mark(imitation->form);
	for (unsigned i = 0; i < imitation->form->vt->nmatter; ++i)
		sq_value_mark(imitation->matter[i]);
}

void sq_imitation_deallocate(struct sq_imitation *imitation) {
	free(imitation->matter);
}

void sq_imitation_dump(FILE *out, const struct sq_imitation *imitation) {
	fprintf(out, "%s(", imitation->form->vt->name);

	for (unsigned i = 0; i < imitation->form->vt->nmatter; ++i) {
		if (i != 0)
			fprintf(out, ", ");

		fprintf(out, "%s=", imitation->form->vt->matter[i].name);
		sq_value_dump(out, imitation->matter[i]);
	}

	if (!imitation->form->vt->nmatter)
		fprintf(out, "<none>");

	fputc(')', out);
}
