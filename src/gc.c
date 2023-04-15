#include <squire/gc.h>
#include <squire/basic.h>
#include <squire/shared.h>
#include <sys/mman.h>

struct anyvalue {
	struct sq_basic basic;
	SQ_ALIGNAS(SQ_VALUE_ALIGNMENT) char _ignored[SQ_VALUE_SIZE - SQ_VALUE_ALIGNMENT];
};
_Static_assert(sizeof(struct anyvalue) == SQ_VALUE_SIZE, "size isnt equal");

struct sq_program *program;
struct anyvalue *heap_start, *heap;
long long heap_size;


void sq_gc_init(long long heap_size_, struct sq_program *program_) {
	heap_size = heap_size_ * SQ_VALUE_SIZE;
	program = program_;
	heap_start = heap = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, 0, 0);

	if (heap == MAP_FAILED)
		sq_throw_io("unable to mmap %zu bytes for the heap", heap_size);
}

void sq_gc_teardown(void) {
	if (munmap(heap_start, heap_size))
		sq_throw_io("unable to un mmap %zu bytes for the heap", heap_size);
}

void *sq_gc_malloc(enum sq_genus_tag genus) {
	if (SQ_UNLIKELY((heap_size / SQ_VALUE_SIZE) <= (heap - heap_start))) {
		sq_gc_start();

		if (SQ_UNLIKELY((heap_size / SQ_VALUE_SIZE) <= heap - heap_start))
			sq_throw("heap exhausted.");
	}

	while (heap->basic.in_use) ++heap;
	heap->basic.genus = genus;
	heap->basic.in_use = 1;
	return heap++;
}

void sq_gc_start(void) {
	// for (struct anyvalue *ptr = heap_start; ptr < heap; ++ptr) {
	// 	if (!ptr->basic.in_use) continue;
	// 	sq_value_mark(sq_value_new_ptr_unchecked((void *) ptr, ptr->basic.genus));
	// }

	sq_program_mark(program);

	for (struct anyvalue *ptr = heap_start; ptr < heap; ++ptr) {
		if (!ptr->basic.in_use) continue;
		if (ptr->basic.marked) {
			ptr->basic.marked = 0;
			continue;
		}

		sq_value_deallocate(sq_value_new_ptr_unchecked((void *) ptr, ptr->basic.genus));
		ptr->basic.in_use = 0;
	}

	heap = heap_start;
}
