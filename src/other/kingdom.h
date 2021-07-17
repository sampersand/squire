#ifndef SQ_KINGDOM_H
#define SQ_KINGDOM_H

#include "value.h"

// lol, todo better hashmaps than linked lists
struct sq_kingdom {
	char *name;
	unsigned nsubjects, subject_cap;

	struct sq_kingdom_subject {
		char *name;
		sq_value person;
	} *subjects;
};

void sq_kingdom_dump(FILE *out, const struct sq_kingdom *kingdom);
void sq_kingdom_deallocate(struct sq_kingdom *kingdom);
sq_value sq_kingdom_get_attr(const struct sq_kingdom *kingdom, const char *attr);
bool sq_kingdom_set_attr(struct sq_kingdom *kingdom, const char *attr, sq_value value);

#endif /* !SQ_KINGDOM_H */
