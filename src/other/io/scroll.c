#include <squire/other/scroll.h>
#include <squire/other/kingdom.h>
#include <squire/text.h>
#include <squire/exception.h>

#include <stdlib.h>
#include <string.h>

static struct sq_kingdom scroll_kingdom = {
	.name = "IO",
	.nsubjects = 1
};
struct sq_kingdom *sq_scroll_kingdom = &scroll_kingdom;


void sq_scroll_init(struct sq_scroll *scroll, const char *filename, const char *mode) {
	if (!(scroll->file = fopen(filename, mode)))
		sq_throw_io("cannot open file '%s'", filename);

	scroll->filename = strdup(filename);
	scroll->mode = strdup(mode);
}

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

void sq_scroll_close(struct sq_scroll *scroll) {
	if (fclose(scroll->file))
		sq_throw_io("unable to close scroll '%s'");
}

struct sq_text *sq_scroll_read(struct sq_scroll *scroll, size_t length) {
	struct sq_text *text = sq_text_allocate(length);
	unsigned nread;
	size_t position = 0;

	while ((nread = fread(&text->ptr[position], 1, length - position, scroll->file)))
		position += nread;

	if (ferror(scroll->file)) {
		sq_text_free(text);
		sq_throw_io("unable to read %zu bytes from '%s'", length, scroll->filename);
	}

	text->ptr[position] = '\0';
	return text;
}

struct sq_text *sq_scroll_read_all(struct sq_scroll *scroll) {
#ifdef __GNUC__
#else
#error todo: read entire scroll to a string on another system
#endif
	(void) scroll;
	abort();
}

void sq_scroll_write(struct sq_scroll *scroll, const char *ptr, size_t length) {
	if (fwrite(ptr, 1, length, scroll->file) < length)
		sq_throw_io("cannot write %zu bytes to '%s'", length, scroll->filename);
}

size_t sq_scroll_tell(struct sq_scroll *scroll) {
	long value = ftell(scroll->file);

	if (value < 0)
		sq_throw_io("cannot get offset for '%s'", scroll->filename);

	return value;
}

void sq_scroll_seek(struct sq_scroll *scroll, long offset, int whence) {
	if (fseek(scroll->file, offset, whence))
		sq_throw_io("cannot seek '%d' for '%s'", whence, scroll->filename);
}
