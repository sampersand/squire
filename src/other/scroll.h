#ifndef SQ_SCROLL_H
#define SQ_SCROLL_H

#include <stdio.h>
#include "value.h"

struct sq_scroll {
	char *filename, *mode;
	FILE *file;
};

void sq_scroll_dump(FILE *out, const struct sq_scroll *scroll);
void sq_scroll_deallocate(struct sq_scroll *scroll);
sq_value sq_scroll_get_attr(const struct sq_scroll *scroll, const char *attr);

#endif /* !SQ_SCROLL_H */
