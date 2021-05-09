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
	die("todo: compile_struct");
}

static void compile_func(struct sq_code *code, struct func_declaration *fdecl) {
	(void) code; (void) fdecl;
	die("todo: compile_func");
}

static void compile_if(struct sq_code *code, struct if_statement *ifstmt) {
	(void) code; (void) ifstmt;
	die("todo: compile_if");
}

static void compile_while(struct sq_code *code, struct while_statement *wstmt) {
	(void) code; (void) wstmt;
	die("todo: compile_while");
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr);

static void compile_return(struct sq_code *code, struct return_statement *rstmt) {
	unsigned result;

	if (!rstmt->value) {
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, SQ_NULL));
		set_index(code, result = next_local(code));
		set_opcode(code, SQ_OC_RETURN);
		set_index(code, result);
	} else {
		result = compile_expr(code, rstmt->value);
		set_opcode(code, SQ_OC_RETURN);
		set_index(code, result);
	}
}

static unsigned compile_primary(struct sq_code *code, struct primary *primary) {
	unsigned result;

	switch (primary->kind) {
	case SQ_PS_PPAREN:
		return compile_expr(code, primary->expr);

	case SQ_PS_PNUMBER:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_number(primary->number)));
		set_index(code, result = next_local(code));
		return result;

	case SQ_PS_PSTRING:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_string(primary->string)));
		set_index(code, result = next_local(code));
		return result;

	case SQ_PS_PBOOLEAN:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_boolean(primary->boolean)));
		set_index(code, result = next_local(code));
		return result;

	case SQ_PS_PNULL:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, SQ_NULL));
		set_index(code, result = next_local(code));
		return result;

	case SQ_PS_PVARIABLE:
		die("todo: SQ_PS_PVARIABLE");

	default:
		bug("unknown primary kind '%d'", primary->kind);
	}
}

static unsigned compile_unary(struct sq_code *code, struct unary_expression *unary) {
	unsigned result, rhs;
	switch (unary->kind) {
	case SQ_PS_UNEG:
		rhs = compile_primary(code, unary->rhs);
		set_opcode(code, SQ_OC_NEG);
		set_index(code, rhs);
		set_index(code, result = next_local(code));
		return result;
	case SQ_PS_UNOT:
		rhs = compile_primary(code, unary->rhs);
		set_opcode(code, SQ_OC_NOT);
		set_index(code, rhs);
		set_index(code, result = next_local(code));
		return result;
	case SQ_PS_UPRIMARY:
		return compile_primary(code, unary->rhs);
	default:
		bug("unknown unary kind '%d'", unary->kind);
	}
}

static unsigned compile_mul(struct sq_code *code, struct mul_expression *mul) {
	switch (mul->kind) {
	case SQ_PS_MMUL:
		die("todo: SQ_PS_MMUL");
	case SQ_PS_MDIV:
		die("todo: SQ_PS_MDIV");
	case SQ_PS_MMOD:
		die("todo: SQ_PS_AMOD");
	case SQ_PS_MUNARY:
		return compile_unary(code, mul->lhs);
	default:
		bug("unknown mul kind '%d'", mul->kind);
	}
}

static unsigned compile_add(struct sq_code *code, struct add_expression *add) {
	switch (add->kind) {
	case SQ_PS_AADD:
		die("todo: SQ_PS_AADD");
	case SQ_PS_ASUB:
		die("todo: SQ_PS_ASUB");
	case SQ_PS_AMUL:
		return compile_mul(code, add->lhs);
	default:
		bug("unknown add kind '%d'", add->kind);
	}
}

static unsigned compile_cmp(struct sq_code *code, struct cmp_expression *cmp) {
	switch (cmp->kind) {
	case SQ_PS_CLTH:
		die("todo: SQ_PS_CLTH");
	case SQ_PS_CGTH:
		die("todo: SQ_PS_CGTH");
	case SQ_PS_CADD:
		return compile_add(code, cmp->lhs);
	default:
		bug("unknown cmp kind '%d'", cmp->kind);
	}
}

static unsigned compile_eql(struct sq_code *code, struct eql_expression *eql) {
	switch (eql->kind) {
	case SQ_PS_EEQL:
		die("todo: SQ_PS_EEQL");
	case SQ_PS_ENEQ:
		die("todo: SQ_PS_ENEQ");
	case SQ_PS_ECMP:
		return compile_cmp(code, eql->lhs);
	default:
		bug("unknown eql kind '%d'", eql->kind);
	}
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr) {
	switch (expr->kind) {
	case SQ_PS_EFNCALL:
		die("todo: SQ_PS_EFNCALL");
	case SQ_PS_EASSIGN:
		die("todo: SQ_PS_EASSIGN");
	case SQ_PS_EMATH:
		return compile_eql(code, expr->math);
	default:
		bug("unknown expr kind '%d'", expr->kind);
	}
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
