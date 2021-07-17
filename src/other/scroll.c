#include "scroll.h"
#include <stdlib.h>
#include <string.h>
#include "text.h"

void sq_scroll_dump(FILE *out, const struct sq_scroll *scroll) {
	fprintf(out, "Scroll(%s, mode=%s)", scroll->filename, scroll->mode);
}

void sq_scroll_deallocate(struct sq_scroll *scroll) {
	free(scroll->filename);
	free(scroll->mode);
	fclose(scroll->file);
}

sq_value sq_scroll_get_attr(const struct sq_scroll *scroll, const char *attr) {
	if (!strcmp(attr, "filename"))
		return sq_value_new(sq_text_new(strdup(scroll->filename)));

	if (!strcmp(attr, "mode"))
		return sq_value_new(sq_text_new(strdup(scroll->mode)));

	return SQ_UNDEFINED;
}
