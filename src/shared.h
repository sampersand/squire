#pragma once

#include <stdio.h>
#include <stdlib.h>

#define die(...) (fprintf(stderr,__VA_ARGS__),exit(1)) 
#define bug(msg, ...) (fprint("bug at " __FILE__ ":%s:%d: " msg, __func__, __LINE__, __VA_ARGS__),abort())

void *xmalloc(size_t length);
void *xrealloc(void *ptr, size_t length);
void *memdup(void *ptr, size_t length);
