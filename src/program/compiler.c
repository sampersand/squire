#include <squire/program/compiler.h>
#include <squire/shared.h>
#include <string.h>

// NOTE: this is an experiment. the actual code for compiling is in `compile.c`

static void extend_bytecode_cap(struct sq_compiler *compiler) {
	if (compiler->code.len >= compiler->code.cap) {
		compiler->code.cap *= 2;
		compiler->code.ary = sq_realloc_vec(union sq_bytecode, compiler->code.ary, compiler->code.cap);
	}
}

void sq_compiler_set_opcode(struct sq_compiler *compiler, enum sq_opcode opcode) {
	sq_log_old("bytecode[%d].opcode=%s\n", compiler->code.len, sq_opcode_repr(opcode));

	extend_bytecode_cap(compiler);

	compiler->code.ary[compiler->code.len++].opcode = opcode;
}

void sq_compiler_set_index(struct sq_compiler *compiler, unsigned index) {
	sq_log_old("bytecode[%d].index=%d\n", compiler->code.len, index);

	extend_bytecode_cap(compiler);

	compiler->code.ary[compiler->code.len++].index = index;
}

void sq_compiler_set_interrupt(struct sq_compiler *compiler, enum sq_interrupt interrupt) {
	sq_log_old("bytecode[%d].interrupt=%s\n", compiler->code.len, sq_interrupt_repr(interrupt));

	extend_bytecode_cap(compiler);

	compiler->code.ary[compiler->code.len++].interrupt = interrupt;
}

void sq_compiler_set_count(struct sq_compiler *compiler, unsigned count) {
	sq_log_old("bytecode[%d].count=%d\n", compiler->code.len, count);

	extend_bytecode_cap(compiler);

	compiler->code.ary[compiler->code.len++].count = count;
}


int sq_compiler_constant_lookup(struct sq_compiler *compiler, sq_value constant) {
	// check to see if we've declared the constant before. if so, reuse that.
	for (unsigned i = 0; i < compiler->consts.len; ++i)
		if (sq_value_eql(compiler->consts.ary[i], constant))
			return i;

	return SQ_COMPILER_NOT_FOUND;
}

unsigned sq_compiler_constant_declare(struct sq_compiler *compiler, sq_value constant) {
	// make sure the constant doesnt already exist
	sq_assert_eq(sq_compiler_constant_lookup(compiler, constant), SQ_COMPILER_NOT_FOUND);

	if (compiler->consts.len >= compiler->consts.cap) {
		compiler->consts.cap *= 2;
		compiler->consts.ary = sq_realloc_vec(sq_value, compiler->consts.ary, compiler->consts.cap);
	}

	unsigned index = compiler->consts.len++;

#ifdef SQ_LOG
	printf("consts[%d]=", index); 
	sq_value_dump(stdout, constant);
	putchar('\n');
#endif /* sq_log_old */

	compiler->consts.ary[index] = constant;

	return index;
}

unsigned sq_compiler_constant_new(struct sq_compiler *compiler, sq_value constant) {
	int index = sq_compiler_constant_lookup(compiler, constant);

	if (index == SQ_COMPILER_NOT_FOUND)
		return sq_compiler_constant_declare(compiler, constant);

	return index;
}

unsigned sq_compiler_constant_load(struct sq_compiler *compiler, sq_value constant) {
	int index = sq_compiler_constant_lookup(compiler, constant);

	// if we haven't encountered the value before, load it.
	if (index == SQ_COMPILER_NOT_FOUND) {
		sq_compiler_set_opcode(compiler, SQ_OC_CLOAD);
		sq_compiler_set_index(compiler, sq_compiler_constant_declare(compiler, constant));
		sq_compiler_set_index(compiler, index = sq_compiler_next_local(compiler));
	}

	return index;
}

int sq_compiler_global_lookup(struct sq_globals *globals, const char *name) {
	// check to see if we've declared the global before
	for (unsigned i = 0; i < globals->len; ++i)
		if (!strcmp(globals->ary[i].name, name))
			return i;

	return SQ_COMPILER_NOT_FOUND;
}

unsigned sq_compiler_global_declare(struct sq_globals *globals, char *name, sq_value value) {
	int index = sq_compiler_global_lookup(globals, name);

	if (index != SQ_COMPILER_NOT_FOUND) {
		if (globals->ary[index].value != SQ_NI && value != SQ_NI)
			sq_throw("attempted to redefine global variable '%s'", name);

		goto found;
	}

	// reallocate if necessary
	if (globals->len == globals->cap) {
		globals->cap *= 2;
		globals->ary = sq_realloc_vec(struct sq_global, globals->ary, globals->cap);
	}

	index = globals->len++;

	sq_log_old("global[%d]: %s\n", index, name);

	// initialize the global
	globals->ary[index].name = strdup(name);

found:

	globals->ary[index].value = value;
	return index;
}

unsigned sq_compiler_global_new(struct sq_globals *globals, char *name, sq_value value) {
	int index = sq_compiler_global_lookup(globals, name);

	if (index == SQ_COMPILER_NOT_FOUND)
		return sq_compiler_global_declare(globals, name, value);

	// we own the name, so we need to free it.
	free(name);
	return index;
}

int sq_compiler_variable_lookup(struct sq_compiler *compiler, const char *name) {
	// check to see if we've declared the local before
	for (unsigned i = 0; i < compiler->variables.len; ++i)
		if (!strcmp(name, compiler->variables.ary[i].name))
			return compiler->variables.ary[i].index;

	return SQ_COMPILER_NOT_FOUND;
}

unsigned sq_compiler_variable_declare(struct sq_compiler *compiler, char *name) {
	sq_assert_eq(sq_compiler_variable_lookup(compiler, name), SQ_COMPILER_NOT_FOUND);

	if (compiler->variables.cap <= compiler->variables.len) {
		compiler->variables.cap *= 2;
		compiler->variables.ary = sq_realloc_vec(
			struct sq_local, compiler->variables.ary, compiler->variables.cap
		);
	}

	unsigned variable_index = compiler->variables.len++;
	unsigned local_index = sq_compiler_next_local(compiler);

	sq_log_old("locals[%d]: %s (variables[%d])\n", local_index, name, variable_index);

	compiler->variables.ary[variable_index].name = name;
	compiler->variables.ary[variable_index].index = local_index;

	return local_index;
}

unsigned sq_compiler_variable_new(struct sq_compiler *compiler, char *name) {
	int index = sq_compiler_variable_lookup(compiler, name);

	return index == SQ_COMPILER_NOT_FOUND ? sq_compiler_variable_declare(compiler, name) : index;
}

int sq_compiler_identifier_lookup(struct sq_compiler *compiler, char *name) {
	int index;

	if ((index = sq_compiler_variable_lookup(compiler, name)) != SQ_COMPILER_NOT_FOUND)
		return index;

	if ((index = sq_compiler_global_lookup(compiler->globals, name)) != SQ_COMPILER_NOT_FOUND)
		return ~index;

	return sq_compiler_variable_declare(compiler, name);
}

unsigned sq_compiler_identifier_load(struct sq_compiler *compiler, char *name) {
	int index = sq_compiler_identifier_lookup(compiler, name);

	if (SQ_COMPILER_IS_GLOBAL(index)) {
		sq_compiler_set_opcode(compiler, SQ_OC_GLOAD);
		sq_compiler_set_index(compiler, SQ_COMPILER_GLOBAL_INDEX(index));
		sq_compiler_set_index(compiler, index = sq_compiler_next_local(compiler));
	}

	return index;
}
