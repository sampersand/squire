#include <squire/other/builtin_journey.h>
#include <stdlib.h>

void sq_builtin_journey_dump(FILE *out, const struct sq_builtin_journey *builtin_journey) {
	fprintf(out, "BuiltinJourney(%s, nargs=%d)", builtin_journey->name, builtin_journey->nargs);
}

void sq_builtin_journey_deallocate(struct sq_builtin_journey *builtin_journey) {
	free(builtin_journey->name);
}
