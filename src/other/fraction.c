#include <squire/other/fraction.h>
#include <squire/numeral.h>
#include <squire/text.h>

// struct sq_fraction {
// 	int numer;
// 	unsigned short denom;
// };

sq_numeral sq_fraction_to_numeral(struct sq_fraction fraction) {
	return fraction.numer / fraction.denom * (fraction.positive ? 1 : -1);
}

sq_veracity sq_fraction_to_veracity(struct sq_fraction fraction) {
	return fraction.numer != 0;
}

sq_value sq_fraction_get_attr(struct sq_fraction fraction, const char *attr) {
	if (!strcmp(attr, "numer")) return sq_value_new_numeral(fraction.numer);
	if (!strcmp(attr, "denom")) return sq_value_new_numeral(fraction.denom);
	if (!strcmp(attr, "sign")) return sq_value_new_numeral(fraction.positive ? 1 : -1);
	return SQ_UNDEFINED;
}

struct sq_text *sq_fraction_to_arabic(struct sq_fraction fraction) {
	// largest value is `-2147483648/1728`
	struct sq_text *text = sq_text_allocate(16);
	char *ptr = text->ptr;

	if (!fraction.positive) *ptr++ = '-';
	ptr += sprintf(ptr, "%u", fraction.numer);
	*ptr++ = '/';
	ptr += sprintf(ptr, "%u", fraction.denom);
	*ptr = '\0';
	text->length = ptr - text->ptr;

	return text;

}

struct sq_text *sq_fraction_to_roman(struct sq_fraction fraction) {
	
}


// struct sq_text *sq_fraction_to_text(const struct sq_fraction *other);
// sq_numeral sq_fraction_to_numeral(const struct sq_fraction *other);
// sq_veracity sq_fraction_to_veracity(const struct sq_fraction *other);
// sq_value sq_fraction_get_attr(const struct sq_fraction *other, const char *attr);
// bool sq_fraction_set_attr(struct sq_fraction *other, const char *attr, sq_value value);
// bool sq_fraction_matches(const struct sq_fraction *formlike, sq_value to_check);

