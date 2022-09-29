#include <squire/other/pattern_helper.h>


void sq_pattern_helper_dump(FILE *out, const struct sq_pattern_helper *helper) {
	if (helper->kind == SQ_PH_NOT) {
		fputc('~', out);
		sq_value_dump(helper->left);
		return;
	}

	sq_value_dump(helper->left);
	fprintf(out, " %c ", helper->kind == SQ_PH_AND ? '&' : '|');
	sq_value_dump(helper->right);
}

void sq_pattern_helper_deallocate(struct sq_pattern_helper *helper) {
	sq_value_free(helper->left);
	
	if (helper->kind != SQ_PH_NOT)
		sq_value_free(helper->right);
}

bool sq_pattern_helper_matches(const struct sq_pattern_helper *helper, sq_value to_check) {
	bool left_matches = sq_value_matches(helper->left, to_check);

	if (helper->kind == SQ_PH_NOT)
		return !left_matches;

	if (helper->kind == SQ_PH_AND && !left_matches)
		return false;

	if (helper->kind == SQ_PH_OR && left_matches)
		return true;

	return sq_value_matches(helper->right, to_check);
}
