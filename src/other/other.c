#include <squire/other/other.h>
#include <squire/text.h>
#include <squire/exception.h>
#include <squire/journey.h>

#include <stdlib.h>

void sq_other_dump(FILE *out, const struct sq_other *other) {
	switch (other->kind) {
	case SQ_OK_SCROLL:
		sq_scroll_dump(out, sq_other_as_scroll((struct sq_other *) other));
		break;

	case SQ_OK_BUILTIN_JOURNEY:
		sq_builtin_journey_dump(out, sq_other_as_builtin_journey((struct sq_other *) other));
		break;

	case SQ_OK_EXTERNAL:
		sq_external_dump(out, sq_other_as_external((struct sq_other *) other));
		break;

	case SQ_OK_KINGDOM:
		sq_kingdom_dump(out, sq_other_as_kingdom((struct sq_other *) other));
		break;

	case SQ_OK_ENVOY:
		sq_envoy_dump(out, sq_other_as_envoy((struct sq_other *) other));
		break;

	case SQ_OK_CITATION:
		sq_citation_dump(out, sq_other_as_citation((struct sq_other *) other));
		break;

	case SQ_OK_PAT_HELPER:
		sq_pattern_helper_dump(out, sq_other_as_pattern_helper((struct sq_other *) other));
		break;
	}
}

void sq_other_deallocate(struct sq_other *other) {
	assert(!other->refcount);
	switch (other->kind) {
	case SQ_OK_SCROLL:
		sq_scroll_deallocate(sq_other_as_scroll(other));
		break;

	case SQ_OK_BUILTIN_JOURNEY:
		sq_builtin_journey_deallocate(sq_other_as_builtin_journey(other));
		break;

	case SQ_OK_EXTERNAL:
		sq_external_deallocate(sq_other_as_external(other));
		break;

	case SQ_OK_KINGDOM:
		sq_kingdom_deallocate(sq_other_as_kingdom(other));
		break;

	case SQ_OK_ENVOY:
		sq_envoy_deallocate(sq_other_as_envoy(other));
		break;

	case SQ_OK_CITATION:
		break;

	case SQ_OK_PAT_HELPER:
		sq_pattern_helper_deallocate(sq_other_as_pattern_helper(other));
		break;
	}

	free(other);
}

const char *sq_other_typename(const struct sq_other *other) {
	switch (other->kind) {
	case SQ_OK_SCROLL:
		return "Scroll";

	case SQ_OK_BUILTIN_JOURNEY:
		return "BuiltinJourney";

	case SQ_OK_EXTERNAL:
		return sq_external_typename(sq_other_as_external((struct sq_other *) other));

	case SQ_OK_KINGDOM:
		return "Kingdom";

	case SQ_OK_ENVOY:
		return "Envoy";

	case SQ_OK_CITATION:
		return "Citation";

	case SQ_OK_PAT_HELPER:
		return "PatternHelper";
	}
}

sq_value sq_other_genus(const struct sq_other *other) {
	static struct sq_text KIND_SCROLL = SQ_TEXT_STATIC("Scroll");
	static struct sq_text KIND_KINGDOM = SQ_TEXT_STATIC("Kingdom");
	static struct sq_text KIND_ENVOY = SQ_TEXT_STATIC("Envoy");
	static struct sq_text KIND_BUILTIN_JOURNEY = SQ_TEXT_STATIC("BuiltinJourney");
	static struct sq_text KIND_CITATION = SQ_TEXT_STATIC("Citation");
	static struct sq_text KIND_PATTERN_HELPER = SQ_TEXT_STATIC("PatternHelper");

	switch (other->kind) {
	case SQ_OK_SCROLL:
		return sq_value_new_text(&KIND_SCROLL);

	case SQ_OK_EXTERNAL:
		return sq_external_genus(sq_other_as_external((struct sq_other *) other));

	case SQ_OK_KINGDOM:
		return sq_value_new_text(&KIND_KINGDOM);

	case SQ_OK_ENVOY:
		return sq_value_new_text(&KIND_ENVOY);

	case SQ_OK_BUILTIN_JOURNEY:
		return sq_value_new_text(&KIND_BUILTIN_JOURNEY);

	case SQ_OK_CITATION:
		return sq_value_new_text(&KIND_CITATION);

	case SQ_OK_PAT_HELPER:
		return sq_value_new_text(&KIND_PATTERN_HELPER);
	}
}

struct sq_text *sq_other_to_text(const struct sq_other *other) {
	switch (other->kind) {
	case SQ_OK_SCROLL:
		return sq_text_new(strdup(sq_other_as_scroll((struct sq_other *) other)->filename));

	case SQ_OK_EXTERNAL:
		return sq_external_to_text(sq_other_as_external((struct sq_other *) other));

	case SQ_OK_KINGDOM:
		return sq_text_new(strdup(sq_other_as_kingdom((struct sq_other *) other)->name));

	case SQ_OK_ENVOY:
	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_CITATION:
	case SQ_OK_PAT_HELPER:
		sq_todo("others to text");
	}
}

sq_numeral sq_other_to_numeral(const struct sq_other *other) {
	switch (other->kind) {
	case SQ_OK_EXTERNAL:
		return sq_external_to_numeral(sq_other_as_external((struct sq_other *) other));

	case SQ_OK_KINGDOM:
		if (!strcmp(sq_other_as_kingdom((struct sq_other *) other)->name, "Samperland"))
			return 1; // Samperland's #1 LOL.
		// else fallthrough

	case SQ_OK_CITATION:
		return (sq_numeral) sq_other_as_citation((struct sq_other *) other);

	case SQ_OK_SCROLL:
	case SQ_OK_ENVOY:
	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_PAT_HELPER:
		sq_throw("cannot convert '%s' to a numeral", sq_other_typename(other));
	}
}

sq_veracity sq_other_to_veracity(const struct sq_other *other) {
	switch (other->kind) {
	case SQ_OK_EXTERNAL:
		return sq_external_to_veracity(sq_other_as_external((struct sq_other *) other));

	case SQ_OK_CITATION:
		return sq_other_as_citation((struct sq_other *) other);

	case SQ_OK_SCROLL:
	case SQ_OK_KINGDOM:
	case SQ_OK_ENVOY:
	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_PAT_HELPER:
		sq_throw("cannot get veracity of '%s'", sq_other_typename(other));
	}
}

sq_value sq_other_get_attr(const struct sq_other *other, const char *attr) {
	switch (other->kind) {
	case SQ_OK_SCROLL:
		return sq_scroll_get_attr(sq_other_as_scroll((struct sq_other *) other), attr);

	case SQ_OK_EXTERNAL:
		return sq_external_get_attr(sq_other_as_external((struct sq_other *) other), attr);

	case SQ_OK_KINGDOM:
		return sq_kingdom_get_attr(sq_other_as_kingdom((struct sq_other *) other), attr);

	case SQ_OK_ENVOY:
		return sq_envoy_get_attr(sq_other_as_envoy((struct sq_other *) other), attr);

	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_CITATION:
	case SQ_OK_PAT_HELPER:
		return SQ_UNDEFINED;
	}
}

bool sq_other_set_attr(struct sq_other *other, const char *attr, sq_value value) {
	switch (other->kind) {
	case SQ_OK_EXTERNAL:
		return sq_external_set_attr(sq_other_as_external(other), attr, value);

	case SQ_OK_KINGDOM:
		return sq_kingdom_set_attr(sq_other_as_kingdom(other), attr, value);

	case SQ_OK_ENVOY:
		return sq_envoy_set_attr(sq_other_as_envoy(other), attr, value);

	case SQ_OK_SCROLL:
	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_CITATION:
	case SQ_OK_PAT_HELPER:
		return false;
	}
}


bool sq_other_matches(const struct sq_other *formlike, sq_value to_check) {
	switch (formlike->kind) {
	case SQ_OK_EXTERNAL:
		return sq_external_matches(sq_other_as_external((struct sq_other *) formlike), to_check);

	case SQ_OK_SCROLL:
	case SQ_OK_KINGDOM:
	case SQ_OK_ENVOY:
	case SQ_OK_BUILTIN_JOURNEY:
	case SQ_OK_CITATION:
		return sq_value_eql(sq_value_new_other((struct sq_other *) formlike), to_check);

	case SQ_OK_PAT_HELPER:
		return sq_pattern_helper_matches(sq_other_as_pattern_helper((struct sq_other *) formlike), to_check);
	}
}

sq_value sq_other_call(struct sq_other *tocall, struct sq_args args) {
	switch (tocall->kind) {
	case SQ_OK_BUILTIN_JOURNEY:
		return sq_builtin_journey_call(&tocall->builtin_journey, args);

	default:
		return SQ_UNDEFINED;
	}
}


