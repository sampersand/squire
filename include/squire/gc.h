#ifndef SQ_GC_H
#define SQ_GC_H

#include <squire/program.h>
#include <squire/valuedecl.h>

void sq_gc_init(long long heap_size, struct sq_program *program);
void sq_gc_start(void);
void sq_gc_teardown(void);
void *sq_gc_malloc(enum sq_genus_tag genus); // allocates enough to store one value

#endif 
