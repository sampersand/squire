#ifndef SQ_EXTERNAL_H
#define SQ_EXTERNAL_H

#include <stdio.h>
#include <squire/value.h>

struct sq_external;

struct sq_external_form {
	const char *name;
	void (*deallocate)(struct sq_external *);
	void (*dump)(FILE *, const struct sq_external *);	
	sq_value (*get_attr)(const struct sq_external *, const char *);
	bool (*set_attr)(struct sq_external *, const char *, sq_value);
	bool (*matches)(const struct sq_external *, sq_value);

	struct sq_text *(*to_text)(const struct sq_external *);
	sq_veracity *(*to_veracity)(const struct sq_external *);
	sq_numeral (*to_numeral)(const struct sq_external *);
};

struct sq_external {
	const struct sq_external_form *form;
	void *data;
};


static inline const char *sq_external_typename(const struct sq_external *external) {
	return external->form->name;
}

void sq_external_dump(FILE *out, const struct sq_external *external);
void sq_external_deallocate(struct sq_external *external);
sq_value sq_external_genus(struct sq_external *external);
struct sq_text *sq_external_to_text(const struct sq_external *external);
sq_numeral sq_external_to_numeral(const struct sq_external *external);
sq_veracity sq_external_to_veracity(const struct sq_external *external);
sq_value sq_external_get_attr(const struct sq_external *external, const char *attr);
bool sq_external_set_attr(struct sq_external *external, const char *attr, sq_value value);
bool sq_external_matches(const struct sq_external *external, sq_value tocheck);

#endif /* !SQ_EXTERNAL_H */
