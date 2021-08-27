#ifndef SQ_COMPILER_H
#define SQ_COMPILER_H

#include <squire/bytecode.h>
#include <squire/value.h>

#ifndef SQ_COMPILER_MAX_COMEFROMS
# define SQ_COMPILER_MAX_COMEFROMS 16
#endif /* !SQ_COMPILER_MAX_COMEFROMS */

struct sq_compiler {
	struct sq_globals {
		unsigned len, cap;
		struct sq_global {
			char *name;
			sq_value value;
		} *ary;
	} *globals;

	struct {
		unsigned cap, len;
		union sq_bytecode *ary;
	} code;

	unsigned nlocals;

	struct {
		unsigned cap, len;
		struct label {
			char *name;
			int *length, indices[SQ_COMPILER_MAX_COMEFROMS];
		} *ary;
	} labels;

	struct {
		unsigned cap, len;

		struct sq_local {
			char *name;
			unsigned index;
		} *ary;
	} variables;

	struct {
		unsigned cap, len;
		sq_value *ary;
	} consts;
};

void sq_compiler_set_opcode(struct sq_compiler *compiler, enum sq_opcode opcode);
void sq_compiler_set_index(struct sq_compiler *compiler, unsigned index);
void sq_compiler_set_interrupt(struct sq_compiler *compiler, enum sq_interrupt interrupt);
void sq_compiler_set_count(struct sq_compiler *compiler, unsigned count);

static inline unsigned sq_compiler_next_local(struct sq_compiler *compiler) {
	return ++compiler->nlocals;
}

#define SQ_COMPILER_NOT_FOUND (-1)

int sq_compiler_constant_lookup(struct sq_compiler *compiler, sq_value constant);
unsigned sq_compiler_constant_declare(struct sq_compiler *compiler, sq_value constant);
unsigned sq_compiler_constant_new(struct sq_compiler *compiler, sq_value constant);
unsigned sq_compiler_constant_load(struct sq_compiler *compiler, sq_value constant);

int sq_compiler_global_lookup(struct sq_globals *globals, const char *name);
unsigned sq_compiler_global_declare(struct sq_globals *globals, char *name, sq_value value);
unsigned sq_compiler_global_new(struct sq_globals *globals, char *name, sq_value value);

int sq_compiler_variable_lookup(struct sq_compiler *compiler, const char *name);
unsigned sq_compiler_variable_declare(struct sq_compiler *compiler, char *name);
unsigned sq_compiler_variable_new(struct sq_compiler *compiler, char *name);

#define SQ_COMPILER_IS_GLOBAL(x) ((x) < 0)
#define SQ_COMPILER_GLOBAL_INDEX(x) (~(x))

int sq_compiler_identifier_lookup(struct sq_compiler *compiler, char *name);
unsigned sq_compiler_identifier_load(struct sq_compiler *compiler, char *name);

#endif /* !SQ_COMPILER_H */
