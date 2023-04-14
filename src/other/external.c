#include <squire/other/external.h>
#include <squire/other/other.h>
#include <squire/text.h>
#include <squire/exception.h>

#include <string.h>

void sq_external_dump(FILE *out, const struct sq_external *external) {
	external->form->dump(out, external);
}

void sq_external_deallocate(struct sq_external *external) {
	external->form->deallocate(external);
}

sq_value sq_external_genus(struct sq_external *external) {
	// todo: actually return a form and not the name lol.
	return sq_value_new_text(sq_text_new(strdup(external->form->name)));
}

struct sq_text *sq_external_to_text(const struct sq_external *external) {
	if (!external->form->to_text)
		sq_throw("cannot convert a '%s' to text", external->form->name);

	return external->form->to_text(external);
}

sq_numeral sq_external_to_numeral(const struct sq_external *external) {
	if (!external->form->to_numeral)
		sq_throw("cannot convert a '%s' to a numeral", external->form->name);

	return external->form->to_numeral(external);
}

sq_veracity sq_external_to_veracity(const struct sq_external *external) {
	if (!external->form->to_veracity)
		sq_throw("cannot convert a '%s' to veracity", external->form->name);

	return external->form->to_veracity(external);
}

sq_value sq_external_get_attr(const struct sq_external *external, const char *attr) {
	if (!external->form->get_attr)
		return SQ_UNDEFINED;

	return external->form->get_attr(external, attr);
}

bool sq_external_set_attr(struct sq_external *external, const char *attr, sq_value value) {
	if (!external->form->set_attr)
		return SQ_UNDEFINED;

	return external->form->set_attr(external, attr, value);
}

bool sq_external_matches(const struct sq_external *external, sq_value tocheck) {
	if (!external->form->matches)
		return sq_value_is_other(tocheck) && external == sq_other_as_external(sq_value_as_other(tocheck));

	return external->form->matches(external, tocheck);
}
