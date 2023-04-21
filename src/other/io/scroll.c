#include <squire/other/other.h>
#include <squire/other/scroll.h>
#include <squire/other/kingdom.h>
#include <squire/text.h>
#include <squire/exception.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

// static struct sq_kingdom scroll_kingdom = {
// 	.name = "IO",
// 	.nsubjects = 1
// };
// struct sq_kingdom *sq_scroll_kingdom = &scroll_kingdom;

static sq_value write_journey, write_func(struct sq_args);
static sq_value read_journey, read_func(struct sq_args);
static sq_value readall_journey, readall_func(struct sq_args);
static sq_value seek_journey, seek_func(struct sq_args);
static sq_value close_journey, close_func(struct sq_args);
static sq_value tell_journey, tell_func(struct sq_args);

void sq_scroll_init(struct sq_scroll *scroll, const char *filename, const char *mode) {
	if (!(scroll->file = fopen(filename, mode)))
		sq_throw_io("cannot open file '%s'", filename);

	scroll->filename = strdup(filename);
	scroll->mode = strdup(mode);

	struct sq_other *other;
#define NEW_JOURNEY(_name, _nargs) \
	_name##_journey = sq_value_new_other(other = sq_mallocv(struct sq_other)); \
	other->kind = SQ_OK_BUILTIN_JOURNEY; \
	other->builtin_journey.name = "Scroll."#_name; \
	other->builtin_journey.nargs = _nargs; \
	other->builtin_journey.func = _name##_func; 

	NEW_JOURNEY(write, 2);
	NEW_JOURNEY(read, 2);
	NEW_JOURNEY(readall, 2);
	NEW_JOURNEY(seek, 3);
	NEW_JOURNEY(close, 1);
	NEW_JOURNEY(tell, 1);
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
		return sq_value_new_text(sq_text_new(strdup(scroll->filename)));

	if (!strcmp(attr, "mode"))
		return sq_value_new_text(sq_text_new(strdup(scroll->mode)));

	if (!strcmp(attr, "write")) return write_journey;
	if (!strcmp(attr, "read")) return read_journey;
	if (!strcmp(attr, "seek")) return seek_journey;
	if (!strcmp(attr, "tell")) return tell_journey;
	if (!strcmp(attr, "close")) return close_journey;
	if (!strcmp(attr, "readall")) return readall_journey;

	return SQ_UNDEFINED;
}

void sq_scroll_close(struct sq_scroll *scroll) {
	if (fclose(scroll->file))
		sq_throw_io("unable to close scroll '%s'");
}

struct sq_text *sq_scroll_read(struct sq_scroll *scroll, size_t length) {
	if (length == 0)
		return sq_scroll_read_all(scroll);

	struct sq_text *text = sq_text_allocate(length);
	unsigned nread;
	size_t position = 0;

	while ((nread = fread(&text->ptr[position], 1, length - position, scroll->file)))
		position += nread;

	if (ferror(scroll->file)) {
		sq_throw_io("unable to read %zu bytes from '%s'", length, scroll->filename);
	}

	text->ptr[position] = '\0';
	return text;
}

struct sq_text *sq_scroll_read_all(struct sq_scroll *scroll) {
	return sq_text_new(sq_read_file(scroll->filename));
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

static sq_value write_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 2, 0);

	struct sq_scroll *scroll = sq_other_as_scroll(sq_value_as_other(args.pargv[0]));
	struct sq_text *text = sq_value_to_text(args.pargv[1]);

	sq_scroll_write(scroll, text->ptr, text->length);
	return SQ_NI;
}

static sq_value read_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 2, 0);

	struct sq_scroll *scroll = sq_other_as_scroll(sq_value_as_other(args.pargv[0]));
	sq_value arg = args.pargv[1];

	switch (sq_value_genus_tag(arg)) {
	case SQ_G_NUMERAL:
		if (sq_value_as_numeral(arg) < 0)
			sq_throw("can only read nonnegative amounts");

		return sq_value_new_text(sq_scroll_read(scroll, sq_value_as_numeral(arg)));

	case SQ_G_TEXT:
		if (strcmp(sq_value_as_text(arg)->ptr, "\n"))
			sq_throw("todo: non-newline gets");

		{
			size_t length;
			char *result = fgetln(scroll->file, &length);

			if (!result) sq_throw(strerror(errno));

			return sq_value_new_text(sq_text_new(strndup(result, length)));
		}

	case SQ_G_OTHER:
		if (arg == SQ_NI)
			return sq_value_new_text(sq_scroll_read_all(scroll));
		SQ_FALLTHROUGH

	default:
		sq_throw("invalid read arugment kind '%s'", sq_value_typename(arg));
	}
}

static sq_value readall_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 1, 0);

	sq_value pargv2[2] = { args.pargv[0], SQ_NI };

	return read_func((struct sq_args) { 
		.pargc = 2,
		.kwargc = 0,
		.pargv = pargv2
	});
}


static sq_value seek_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 3, 0);

	struct sq_scroll *scroll = sq_other_as_scroll(sq_value_as_other(args.pargv[0]));
	sq_numeral offset = sq_value_to_numeral(args.pargv[1]);
	sq_numeral whence = sq_value_to_numeral(args.pargv[2]);

	sq_scroll_seek(scroll, offset, whence);
	return sq_value_new_numeral(sq_scroll_tell(scroll));
}

static sq_value close_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 1, 0);

	struct sq_scroll *scroll = sq_other_as_scroll(sq_value_as_other(args.pargv[0]));

	sq_scroll_close(scroll);
	return SQ_NI;
}

static sq_value tell_func(struct sq_args args) {
	sq_journey_assert_arglen(args, 3, 0);

	struct sq_scroll *scroll = sq_other_as_scroll(sq_value_as_other(args.pargv[0]));

	sq_scroll_seek(scroll, sq_value_to_numeral(args.pargv[1]), sq_value_to_numeral(args.pargv[2]));
	return SQ_NI;
}
