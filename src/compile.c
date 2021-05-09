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
// struct variable {
// 	unsigned idx;
// 	char *name;
// };

struct var {
	char *name;
	unsigned index;
};

struct sq_code {
	unsigned codecap, codelen;
	union sq_bytecode *bytecode;

	unsigned nlocals;

	unsigned varcap, varlen;
	struct var *variables;

	unsigned constcap, constlen;
	sq_value *consts;
};

#define RESIZE(cap, len, pos, type) \
	if (code->cap == code->len) \
		code->pos = xrealloc(code->pos, sizeof(type [code->cap*=2]));



static void set_opcode(struct sq_code *code, enum sq_opcode opcode) {
	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	// printf("bytecode[%d].opcode=%x\n", code->codelen, opcode);
	code->bytecode[code->codelen++].opcode = opcode;
}

static unsigned set_index(struct sq_code *code, sq_index index) {
	RESIZE(codecap, codelen, bytecode, union sq_bytecode);
	// printf("bytecode[%d].index=%d\n", code->codelen, index);
	code->bytecode[code->codelen++].index = index;
	return index;
}

static unsigned new_constant(struct sq_code *code, sq_value value) {
	RESIZE(constcap, constlen, consts, sq_value);
	// printf("consts[%d]=", code->constlen); sq_value_dump(value); puts("");
	code->consts[code->constlen++] = value;
	return code->constlen - 1;
}
static unsigned next_local(struct sq_code *code) {
	return code->nlocals++;
}

static unsigned new_variable(struct sq_code *code, char *var) {
	for (unsigned i = 0; i < code->varlen; ++i)
		if (!(strcmp(var, code->variables[i].name))) {
			free(var);
			return code->variables[i].index;
		}
	// printf("vars[%d]=%s\n", code->varlen, var);

	RESIZE(varcap, varlen, variables, struct var);
	code->variables[code->varlen].name = var;
	return code->variables[code->varlen++].index = next_local(code);
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr);
static void compile_statements(struct sq_code *code, struct statements *stmts);

static void compile_struct(struct sq_code *code, struct struct_declaration *sdecl) {
	(void) code; (void) sdecl;
	die("todo: compile_struct");
}

static void compile_func(struct sq_code *code, struct func_declaration *fdecl) {
	(void) code; (void) fdecl;
	die("todo: compile_func");
}

static void compile_if(struct sq_code *code, struct if_statement *ifstmt) {
	unsigned cond, *iffalse_dst, *done_dst;

	cond = compile_expr(code, ifstmt->cond);
	set_opcode(code, SQ_OC_JMP_FALSE);
	set_index(code, cond);
	iffalse_dst = &code->bytecode[code->codelen].index;
	set_index(code, 0);

	compile_statements(code, ifstmt->iftrue);

	if (ifstmt->iffalse) {
		set_opcode(code, SQ_OC_JMP);
		done_dst = &code->bytecode[code->codelen].index;
		set_index(code, 0);
		*iffalse_dst = code->codelen;
		compile_statements(code, ifstmt->iffalse);
		*done_dst = code->codelen;
	} else {
		set_opcode(code, SQ_OC_NOOP);
		*iffalse_dst = code->codelen;
	}
}

static void compile_while(struct sq_code *code, struct while_statement *wstmt) {
	unsigned cond, condlbl, *donelbl;

	condlbl = code->codelen;
	cond = compile_expr(code, wstmt->cond);
	set_opcode(code, SQ_OC_JMP_FALSE);
	set_index(code, cond);
	donelbl = &code->bytecode[code->codelen].index;
	set_index(code, 0);
	compile_statements(code, wstmt->body);
	set_opcode(code, SQ_OC_JMP);
	set_index(code, condlbl);
	*donelbl = code->codelen;
}

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
	switch (primary->kind) {
	case SQ_PS_PPAREN:
		return compile_expr(code, primary->expr);

	case SQ_PS_PNUMBER:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_number(primary->number)));
		return set_index(code, next_local(code));

	case SQ_PS_PSTRING:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_string(primary->string)));
		return set_index(code, next_local(code));

	case SQ_PS_PBOOLEAN:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, sq_value_new_boolean(primary->boolean)));
		return set_index(code, next_local(code));

	case SQ_PS_PNULL:
		set_opcode(code, SQ_OC_CLOAD);
		set_index(code, new_constant(code, SQ_NULL));
		return set_index(code, next_local(code));

	case SQ_PS_PVARIABLE:
		if (primary->variable->field) die("doesnt support setting fields yet");
		return new_variable(code, primary->variable->name);

	default:
		bug("unknown primary kind '%d'", primary->kind);
	}
}

static unsigned compile_unary(struct sq_code *code, struct unary_expression *unary) {
	unsigned rhs;
	switch (unary->kind) {
	case SQ_PS_UNEG:
		rhs = compile_primary(code, unary->rhs);
		set_opcode(code, SQ_OC_NEG);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_UNOT:
		rhs = compile_primary(code, unary->rhs);
		set_opcode(code, SQ_OC_NOT);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_UPRIMARY:
		return compile_primary(code, unary->rhs);

	default:
		bug("unknown unary kind '%d'", unary->kind);
	}
}

static unsigned compile_mul(struct sq_code *code, struct mul_expression *mul) {
	unsigned lhs, rhs;
	switch (mul->kind) {
	case SQ_PS_MMUL:
		lhs = compile_unary(code, mul->lhs);
		rhs = compile_mul(code, mul->rhs);
		set_opcode(code, SQ_OC_MUL);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_MDIV:
		lhs = compile_unary(code, mul->lhs);
		rhs = compile_mul(code, mul->rhs);
		set_opcode(code, SQ_OC_DIV);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_MMOD:
		lhs = compile_unary(code, mul->lhs);
		rhs = compile_mul(code, mul->rhs);
		set_opcode(code, SQ_OC_MOD);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_MUNARY:
		return compile_unary(code, mul->lhs);

	default:
		bug("unknown mul kind '%d'", mul->kind);
	}
}

static unsigned compile_add(struct sq_code *code, struct add_expression *add) {
	unsigned lhs, rhs;
	switch (add->kind) {
	case SQ_PS_AADD:
		lhs = compile_mul(code, add->lhs);
		rhs = compile_add(code, add->rhs);
		set_opcode(code, SQ_OC_ADD);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_ASUB:
		lhs = compile_mul(code, add->lhs);
		rhs = compile_add(code, add->rhs);
		set_opcode(code, SQ_OC_SUB);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_AMUL:
		return compile_mul(code, add->lhs);

	default:
		bug("unknown add kind '%d'", add->kind);
	}
}

static unsigned compile_cmp(struct sq_code *code, struct cmp_expression *cmp) {
	unsigned lhs, rhs;

	switch (cmp->kind) {
	case SQ_PS_CLTH:
		lhs = compile_add(code, cmp->lhs);
		rhs = compile_cmp(code, cmp->rhs);
		set_opcode(code, SQ_OC_LTH);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_CGTH:
		lhs = compile_add(code, cmp->lhs);
		rhs = compile_cmp(code, cmp->rhs);
		set_opcode(code, SQ_OC_GTH);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_CADD:
		return compile_add(code, cmp->lhs);

	default:
		bug("unknown cmp kind '%d'", cmp->kind);
	}
}

static unsigned compile_eql(struct sq_code *code, struct eql_expression *eql) {
	unsigned lhs, rhs;

	switch (eql->kind) {
	case SQ_PS_EEQL:
		lhs = compile_cmp(code, eql->lhs);
		rhs = compile_eql(code, eql->rhs);
		set_opcode(code, SQ_OC_EQL);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_ENEQ:
		lhs = compile_cmp(code, eql->lhs);
		rhs = compile_eql(code, eql->rhs);
		set_opcode(code, SQ_OC_NEQ);
		set_index(code, lhs);
		set_index(code, rhs);
		return set_index(code, next_local(code));

	case SQ_PS_ECMP:
		return compile_cmp(code, eql->lhs);

	default:
		bug("unknown eql kind '%d'", eql->kind);
	}
}

static unsigned compile_function_call(struct sq_code *code, struct function_call *fncall) {
	unsigned index;

	if (fncall->func->field)
		die("doesnt support calling fields yet");

	if (!strcmp(fncall->func->name, "print")) {
		if (fncall->arglen != 1)
			die("only one arg for print");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_PRINT);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	if (!strcmp(fncall->func->name, "number")) {
		if (fncall->arglen != 1)
			die("only one arg for number");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_TONUMBER);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	if (!strcmp(fncall->func->name, "string")) {
		if (fncall->arglen != 1)
			die("only one arg for string()");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_TOSTRING);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	if (!strcmp(fncall->func->name, "dump")) {
		if (fncall->arglen != 1)
			die("only one arg for dump()");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_DUMP);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	die("todo: function call for arbitraries");
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr) {
	unsigned index;
	switch (expr->kind) {
	case SQ_PS_EFNCALL:
		return compile_function_call(code, expr->fncall);

	case SQ_PS_EASSIGN:
		if (expr->asgn->var->field) die("doesnt support setting fields yet");

		index = compile_expr(code, expr->asgn->expr);
		set_opcode(code, SQ_OC_MOV);
		set_index(code, index);
		return set_index(code, new_variable(code, expr->asgn->var->name));

	case SQ_PS_EMATH:

		return compile_eql(code, expr->math);

	default:
		bug("unknown expr kind '%d'", expr->kind);
	}
}

static void compile_statements(struct sq_code *code, struct statements *stmts) {
	struct statement *stmt, **stmtptr = stmts->stmts;

	while ((stmt = *stmtptr++)) {

		switch (stmt->kind) {
		case SQ_PS_SSTRUCT: compile_struct(code, stmt->sdecl); break;
		case SQ_PS_SFUNC: compile_func(code, stmt->fdecl); break;
		case SQ_PS_SIF: compile_if(code, stmt->ifstmt); break;
		case SQ_PS_SWHILE: compile_while(code, stmt->wstmt); break;
		case SQ_PS_SRETURN: compile_return(code, stmt->rstmt); break;
		case SQ_PS_SEXPR: compile_expr(code, stmt->expr); break;
		default: bug("unknown statement kind '%d'", stmt->kind);
		}
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
	code.varcap = 16;
	code.varlen = 0;
	code.variables = xmalloc(sizeof(struct var [code.varcap]));

	compile_statements(&code, stmts);
	struct return_statement return_null = { .value = NULL };
	compile_return(&code, &return_null);

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
