#ifndef SQ_CITATION_H
#define SQ_CITATION_H

#include <squire/value.h>

typedef sq_value *sq_citation;

static inline void sq_citation_dump(FILE *out, sq_citation citation) {
	fprintf(out, "<%p>", (void*) citation);
}

#endif
