#ifndef SQ_FRACTION_H
#define SQ_FRACTION_H

#include <stdio.h>
#include <squire/value.h>

struct sq_fraction {
	unsigned numer;
	unsigned short denom; // max denom is 1728.
	bool positive;
};

void sq_fraction_dump(FILE *out, struct sq_fraction fraction);
sq_numeral sq_fraction_to_numeral(struct sq_fraction fraction);
sq_veracity sq_fraction_to_veracity(struct sq_fraction fraction);
sq_value sq_fraction_get_attr(struct sq_fraction fraction, const char *attr);
struct sq_text *sq_fraction_to_roman(struct sq_fraction fraction);
struct sq_text *sq_fraction_to_arabic(struct sq_fraction fraction);

#ifdef SQ_NUMERAL_TO_ARABIC
# define sq_fraction_to_text sq_fraction_to_arabic
#else
# define sq_fraction_to_text sq_fraction_to_roman
#endif /* SQ_NUMERAL_TO_ARABIC */

#endif /* !SQ_FRACTION_H */
