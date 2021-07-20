#ifndef SQ_SCROLL_H
#define SQ_SCROLL_H

#include <stdio.h>
#include "value.h"

struct sq_scroll {
	char *filename, *mode;
	FILE *file;
};

void sq_scroll_init(struct sq_scroll *scroll, const char *filename, const char *mode);
void sq_scroll_dump(FILE *out, const struct sq_scroll *scroll);
void sq_scroll_deallocate(struct sq_scroll *scroll);
sq_value sq_scroll_get_attr(const struct sq_scroll *scroll, const char *attr);

void sq_scroll_close(struct sq_scroll *scroll);
struct sq_text *sq_scroll_read(struct sq_scroll *scroll, size_t length); // negative means to end.
struct sq_text *sq_scroll_read_all(struct sq_scroll *scroll);
void sq_scroll_write(struct sq_scroll *scroll, const char *ptr, size_t length);
size_t sq_scroll_tell(struct sq_scroll *scroll);
void sq_scroll_seek(struct sq_scroll *scroll, long offset, int whence);


#endif /* !SQ_SCROLL_H */
