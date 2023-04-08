#include <squire/other/kingdom.h>
#include <squire/shared.h>

#include <stdlib.h>
#include <string.h>

void sq_kingdom_initialize(struct sq_kingdom *kingdom, unsigned capacity) {
	kingdom->nsubjects = 0;
	kingdom->subject_cap = capacity;
	kingdom->subjects = sq_malloc(sq_sizeof_array(struct sq_kingdom_subject, capacity));
}

void sq_kingdom_dump(FILE *out, const struct sq_kingdom *kingdom) {
	fprintf(out, "Kingdom(%s, {", kingdom->name);

	for (unsigned i = 0; i < kingdom->nsubjects; ++i) {
		if (i != 0) fprintf(out, ", ");
		fprintf(out, "%s", kingdom->subjects[i].name);
	}

	fprintf(out, "})");
}

void sq_kingdom_deallocate(struct sq_kingdom *kingdom) {
	free(kingdom->name);

	for (unsigned i = 0; i < kingdom->nsubjects; ++i) {
		free(kingdom->subjects[i].name);
		sq_value_free(kingdom->subjects[i].person);
	}

	free(kingdom->subjects);
}

// todo: hashmaps lol.
static struct sq_kingdom_subject *get_subject(struct sq_kingdom *kingdom, const char *name) {
	for (unsigned i = 0; i < kingdom->nsubjects; ++i)
		if (!strcmp(kingdom->subjects[i].name, name))
			return &kingdom->subjects[i];

	return NULL;
}

sq_value sq_kingdom_get_attr(const struct sq_kingdom *kingdom, const char *name) {
	struct sq_kingdom_subject *subject = get_subject((struct sq_kingdom *) kingdom, name);

	if (subject == NULL) return SQ_UNDEFINED;

	assert(subject->person != SQ_UNDEFINED);
	return sq_value_clone(subject->person);
}

bool sq_kingdom_set_attr(struct sq_kingdom *kingdom, const char *name, sq_value value) {
	struct sq_kingdom_subject *subject = get_subject(kingdom, name);

	if (subject != NULL) {
		sq_value_free(subject->person);
		subject->person = value;
		return true;
	}

	if (kingdom->subject_cap == kingdom->nsubjects) {
		kingdom->subject_cap *= 2;
		kingdom->subjects =
			sq_realloc(
				kingdom->subjects,
				sq_sizeof_array(struct sq_kingdom_subject, kingdom->subject_cap)
			);
	}

	subject = &kingdom->subjects[kingdom->nsubjects++];
	subject->name = strdup(name);
	subject->person = value;
	return true;
}
