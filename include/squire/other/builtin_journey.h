#ifndef SQ_BUILTIN_JOURNEY_H
#define SQ_BUILTIN_JOURNEY_H

#include <squire/value.h>
#include <squire/journey.h>

struct sq_builtin_journey {
	char *name;
	unsigned nargs;
	sq_value (*func)(struct sq_args args);
};

void sq_builtin_journey_dump(FILE *out, const struct sq_builtin_journey *builtin_journey);
void sq_builtin_journey_deallocate(struct sq_builtin_journey *builtin_journey);

static inline sq_value sq_builtin_journey_call(const struct sq_builtin_journey *builtin_journey, struct sq_args args) {
	return builtin_journey->func(args);
}

#endif /* !SQ_BUILTIN_JOURNEY_H */
