#include "program.h"
#include "function.h"
#include "shared.h"
#include "parse.h"
#include "struct.h"
#include "string.h"
#include <string.h>

struct sq_program *program;

struct var {
	char *name;
	unsigned index;
};

struct {
	unsigned len, cap;
	struct var *vars;
} globals;

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

// static unsigned new_global(struct sq_code *code, char *name, sq_value value) {
// 	for (unsigned i = 0; i < globals.len; ++i) {
// 		if (!strcmp(globals.vars[i].name, name)) {
// 			globals.vars[i]
// 		}
// 	}
// 	RESIZE(globals.len, globals.cap, globals.vars, struct var);

	// prin1tf("consts[%d]=", code->constlen); sq_value_dump(value); puts("");
// 	code->consts[code->constlen++] = value;
// 	return code->constlen - 1;
// }

static int new_variable(struct sq_code *code, char *var) {
	for (unsigned i = 0; i < code->varlen; ++i) {
		if (!(strcmp(var, code->variables[i].name))) {
			free(var);
			return code->variables[i].index;
		}
	}

	for (unsigned i = 0; i < globals.len; ++i) {
		if (!(strcmp(var, globals.vars[i].name))) {
			free(var);
			return ~i;
		}
	}

	// printf("variables[%d]=%s\n", code->varlen, var);


	RESIZE(varcap, varlen, variables, struct var);
	code->variables[code->varlen].name = var;
	return code->variables[code->varlen++].index = next_local(code);
}

static unsigned compile_expr(struct sq_code *code, struct expression *expr);
static void compile_statements(struct sq_code *code, struct statements *stmts);

static void compile_struct(struct sq_code *code, struct struct_declaration *sdecl) {
	(void) code; (void) sdecl;

	struct sq_struct *struct_ = xmalloc(sizeof(struct sq_struct));
	struct_->refcount = 1;
	struct_->nfields = sdecl->nfields;
	struct_->fields = sdecl->fields;
	struct_->name = sdecl->name;

	unsigned idx = new_constant(code, sq_value_new_struct(struct_));
	set_opcode(code, SQ_OC_CLOAD);
	set_index(code, idx);
	set_index(code, new_variable(code, strdup(struct_->name)));

}

static struct sq_function *
compile_function(struct func_declaration *fndecl);

static void compile_func(struct sq_code *code, struct func_declaration *fdecl) {
	struct sq_function *function = compile_function(fdecl);
	unsigned idx = new_constant(code, sq_value_new_function(function));

	set_opcode(code, SQ_OC_CLOAD);
	set_index(code, idx);
	set_index(code, new_variable(code, strdup(function->name)));
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

	case SQ_PS_PVARIABLE: {
		struct variable *var = primary->variable;
		int index;
		unsigned ret;
		index = new_variable(code, var->name);

		if (0 <= index) {
			set_opcode(code, SQ_OC_MOV);
			set_index(code, index);
			set_index(code, ret = next_local(code));
		} else {
			set_opcode(code, SQ_OC_GLOAD);
			set_index(code, ~index);
			set_index(code, ret = next_local(code));
		}

		while ((var = var->field)) {
			set_opcode(code, SQ_OC_ILOAD);
			set_index(code, ret);
			set_index(code, new_constant(code, sq_value_new_string(sq_string_new(strdup(var->name)))));
			set_index(code, ret = next_local(code));
		}
		return ret;
	}

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

	if (!strcmp(fncall->func->name, "length")) {
		if (fncall->arglen != 1)
			die("only one arg for length()");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_LENGTH);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	if (!strcmp(fncall->func->name, "substr")) {
		if (fncall->arglen != 3)
			die("only three args for substr()");
		unsigned idx1 = compile_expr(code, fncall->args[0]);
		unsigned idx2 = compile_expr(code, fncall->args[1]);
		unsigned idx3 = compile_expr(code, fncall->args[2]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_SUBSTR);
		set_index(code, idx1);
		set_index(code, idx2);
		set_index(code, idx3);
		return set_index(code, next_local(code));
	}

	if (!strcmp(fncall->func->name, "exit")) {
		if (fncall->arglen != 1)
			die("only one arg for exit()");
		index = compile_expr(code, fncall->args[0]);
		set_opcode(code, SQ_OC_INT);
		set_index(code, SQ_INT_EXIT);
		set_index(code, index);
		return set_index(code, next_local(code));
	}

	unsigned args[MAX_ARGC];
	for (unsigned i = 0; i < fncall->arglen; ++i)
		args[i] = compile_expr(code, fncall->args[i]);

	set_opcode(code, SQ_OC_CALL);
	set_index(code, new_variable(code, fncall->func->name));
	set_index(code, fncall->arglen);
	for (unsigned i = 0; i < fncall->arglen; ++i)
		set_index(code, args[i]);

	return set_index(code, next_local(code));

}

static unsigned compile_expr(struct sq_code *code, struct expression *expr) {
	unsigned index;
	int variable;
	switch (expr->kind) {
	case SQ_PS_EFNCALL:
		return compile_function_call(code, expr->fncall);

	case SQ_PS_EASSIGN: {
		index = compile_expr(code, expr->asgn->expr);
		variable = new_variable(code, expr->asgn->var->name);

		if (0 <= variable) {
			set_opcode(code, SQ_OC_MOV);
			set_index(code, index);
			return set_index(code, variable);
		} else {
			set_opcode(code, SQ_OC_GSTORE);
			set_index(code, ~variable);
			set_index(code, index);
			return set_index(code, next_local(code));
		}
	}

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

static struct sq_function *
compile_function(struct func_declaration *fndecl) {
	struct sq_code code;
	code.codecap = 2048;
	code.codelen = 0;
	code.bytecode = xmalloc(sizeof(union sq_bytecode[code.codecap]));

	code.nlocals = 0;
	code.constcap = 64;
	code.constlen = 0;
	code.consts = xmalloc(sizeof(sq_value [code.constcap]));

	code.varlen = fndecl->nargs;
	code.varcap = 16 + code.varlen;
	code.variables = xmalloc(sizeof(struct var[code.varcap]));

	for (unsigned i = 0; i < fndecl->nargs; ++i) {
		code.variables[i].name = fndecl->args[i];
		code.variables[i].index = next_local(&code);
		// set_opcode(&code, SQ_OC_MOV);
		// set_index(&code, i);
		// set_index(&code, code.variables[i].index = next_local(&code));
	}

	compile_statements(&code, fndecl->body);
	struct return_statement return_null = { .value = NULL };
	compile_return(&code, &return_null);

	struct sq_function *fn = xmalloc(sizeof(struct sq_function));

	fn->refcount = 1;
	fn->name = fndecl->name;
	fn->argc = fndecl->nargs;
	fn->bytecode = code.bytecode;
	fn->consts = code.consts;
	fn->nconsts = code.constlen;
	fn->nlocals = code.nlocals;
	fn->globals = 0; // todo

	return fn;
}

struct sq_program *sq_program_compile(const char *stream, unsigned argc, char **argv) {
	globals.len = globals.cap = 0;
	globals.vars = xmalloc(sizeof(struct var[globals.cap = 16]));

	program = xmalloc(sizeof(struct sq_program));
	program->nglobals = 0;
	program->globals = NULL;
	program->nfuncs = 0;
	program->funcs = NULL;

	struct func_declaration maindecl = {
		.name = strdup("main"),
		.nargs = argc, // todo: pass commandline arguments
		.args = argv,
		.body = sq_parse_statements(stream)
	};

	program->main = compile_function(&maindecl);

	return program;
}
