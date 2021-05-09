#include "program.h"
#include "function.h"
#include "shared.h"
#include "parse.h"
#include <string.h>

struct sq_program *program;

struct sq_variables {
	unsigned len, cap;
	char *names;
};

struct sq_variables globals;

/*
struct sq_function {
	char *name;
	int refcount; // negative indicates a global function.

	unsigned argc, nlocals, nconsts;
	struct sq_program *program;

	sq_value *consts;
	union sq_bytecode *code;
};

*/

struct sq_code {
	unsigned codecap, codelen;
	union sq_bytecode *bytecode;

	unsigned nlocals;

	unsigned constcap, constlen;
	sq_value *consts;
};

#define RESIZE(cap, len, pos, type) \
	if (code->cap == code->len) \
		code->pos = xrealloc(code->pos, sizeof(type [code->cap*=2]));



static void set_opcode(struct sq_code *code, enum sq_opcode opcode) {
	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	printf("bytecode[%d].opcode=%d\n", code->codelen, opcode);
	code->bytecode[code->codelen++].opcode = opcode;
}

static void set_index(struct sq_code *code, sq_index index) {
	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	printf("bytecode[%d].index=%d\n", code->codelen, index);
	code->bytecode[code->codelen++].index = index;
}

static unsigned new_constant(struct sq_code *code, sq_value value) {
	RESIZE(constcap, constlen, consts, sq_value);
	printf("consts[%d]=", code->constlen);
	sq_value_dump(value);
	puts("");
	code->consts[code->constlen++] = value;
	return code->constlen - 1;
}
static unsigned next_local(struct sq_code *code) {
	return code->nlocals++;
}


static void compile_struct(struct sq_code *code, struct struct_declaration *sdecl) {
	(void) code; (void) sdecl;
}

static void compile_func(struct sq_code *code, struct func_declaration *fdecl) {
	(void) code; (void) fdecl;
}

static void compile_if(struct sq_code *code, struct if_statement *ifstmt) {
	(void) code; (void) ifstmt;
}

static void compile_while(struct sq_code *code, struct while_statement *wstmt) {
	(void) code; (void) wstmt;
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr);

static void compile_return(struct sq_code *code, struct return_statement *rstmt) {
	unsigned local;

	if (!rstmt->value) {
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, SQ_NULL));
		set_index(code, local = next_local(code));
		set_opcode(code, SQ_OC_RETURN);
		set_index(code, local);
	} else {
		local = compile_expr(code, rstmt->value);
		set_opcode(code, SQ_OC_RETURN);
		set_index(code, local);
	}
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr) {
	(void) code; (void) expr;
	return 0;
}

static void compile_function(struct sq_function *fn, struct statements *stmts) {
	struct sq_code code;
	code.codecap = 2048;
	code.codelen = 0;
	code.bytecode = xmalloc(sizeof(union sq_bytecode[code.codecap]));
	code.nlocals = 0;
	code.constcap = 64;
	code.constlen = 0;
	code.consts = xmalloc(sizeof(sq_value [code.constcap]));

	struct statement *stmt, **stmtptr = stmts->stmts;

	while ((stmt = *stmtptr++)) {
		switch (stmt->kind) {
		case SQ_PS_SSTRUCT: compile_struct(&code, stmt->sdecl); break;
		case SQ_PS_SFUNC: compile_func(&code, stmt->fdecl); break;
		case SQ_PS_SIF: compile_if(&code, stmt->ifstmt); break;
		case SQ_PS_SWHILE: compile_while(&code, stmt->wstmt); break;
		case SQ_PS_SRETURN: compile_return(&code, stmt->rstmt); break;
		case SQ_PS_SEXPR: compile_expr(&code, stmt->expr); break;
		default: bug("unknown statement kind '%d'", stmt->kind);
		}
	}

	fn->bytecode = code.bytecode;
	fn->consts = code.consts;
	fn->nconsts = code.constlen;
	fn->nlocals = code.nlocals;
}

struct sq_program *sq_program_compile(const char *stream) {
	struct statements *statements = sq_parse_statements(stream);

	globals.len = 0;
	globals.names = xmalloc(sizeof(char *[globals.cap = 16]));

	program = xmalloc(sizeof(struct sq_program));
	program->nglobals = 0;
	program->globals = NULL;
	program->nfuncs = 0;
	program->funcs = NULL;

	struct sq_function *main = program->main = xmalloc(sizeof(struct sq_function));
	main->name = strdup("main");
	main->refcount = 1;
	main->argc = 0;
	main->nlocals = 0;
	main->nconsts = 0;

	compile_function(main, statements);

	return program;
}
