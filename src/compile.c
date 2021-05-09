#include "compile.h"
#include "parse.h"

struct sq_program *sq_program_compile(const char *stream) {
	struct statements *statements = sq_parse_statements(stream);

	
	return 0;
}

