#ifndef SQ_PATTERN_HELPER_H
#define SQ_PATTERN_HELPER_H

#include <squire/value.h>
#include <stdio.h>

struct sq_pattern_helper {
	sq_value left, right; // right is not set if the enum isn't correct
	enum { SQ_PH_AND, SQ_PH_OR, SQ_PH_NOT } kind;
};

void sq_pattern_helper_dump(FILE *out, const struct sq_pattern_helper *pattern_helper);
void sq_pattern_helper_deallocate(struct sq_pattern_helper *pattern_helper);
bool sq_pattern_helper_matches(const struct sq_pattern_helper *pattern_helper, sq_value to_check);

#endif /* !SQ_PATTERN_HELPER_H */
