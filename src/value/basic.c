#include <squire/basic.h>
#include <stdalign.h>
#if 0
struct basic {
	alignas(8) char stuff;
} __attribute__((packed))
;

struct basic2 {
	struct basic b;
	char foo[3];
} __attribute__((packed));

_Static_assert(sizeof(struct basic2) == 8, "oops");
_Static_assert(alignof(struct basic2) == 4, "oops");

#endif
struct x;
