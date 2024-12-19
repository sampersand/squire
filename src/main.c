#include <squire/program.h>
#include <squire/shared.h>
#include <squire/gc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
	if (argc < 3 || (strcmp(argv[1], "-e") && strcmp(argv[1], "-f"))) {
		fprintf(stderr, "usage: %s (-e 'expr' | -f 'filename')\n", argv[0]);
		return 1;
	}

	struct sq_program program;
#ifndef SQ_GC_HEAP_SIZE
# define SQ_GC_HEAP_SIZE 100000000
#endif
	sq_gc_init(SQ_GC_HEAP_SIZE, &program);

	if (argv[1][1] == 'e') {
		sq_program_compile(&program, argv[2]);
	} else {
		char *contents = sq_read_file(argv[2]);
		sq_program_compile(&program, contents);
		free(contents);
	}
	sq_program_run(&program, argc - 3, argv + 3);
	// sq_gc_start();
	// sq_program_finish(&program);
	// sq_gc_teardown();

	return 0;
}

