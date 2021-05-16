#include "token.h"
#include "program.h"
#include "function.h"
#include "parse.h"
#include "shared.h"
#include "struct.h"
#include <string.h>

struct sq_token last;
bool rewound;

static void untake() {
	assert(!rewound);
	rewound = true;
}

static struct sq_token take_endline() {
	if (rewound) {
		rewound = false;
	} else {
		last = sq_next_token();
	}

	return last;
}

static struct sq_token take() {
	struct sq_token token;
	while ((token = take_endline()).kind == SQ_TK_SOFT_ENDL);
	return token;
}

static unsigned field_name_count;
static char *field_names[256];

static void parse_field_names() {
	field_name_count = 0;

	while (take().kind == SQ_TK_IDENT) {
		if (field_name_count > 255) die("too many fields!");
		field_names[field_name_count++] = last.identifier;

		if (take().kind != SQ_TK_COMMA)
			break;
	}

	untake();
}

#define EXPECTED(kind_, iffalse) \
	do { if (take().kind != kind_) { iffalse; } } while(0)
#define EXPECT(kind_, ...) EXPECTED(kind_, die(__VA_ARGS__))
#define GUARD(kind_) EXPECTED(kind_, untake(); return NULL)

static struct expression *parse_expression(void);


static struct variable *parse_variable(void) {
	GUARD(SQ_TK_IDENT);

	struct variable *var = xmalloc(sizeof(struct variable));
	var->name = last.identifier;
	if (take().kind == SQ_TK_DOT) {
		var->field = parse_variable();
	} else {
		untake();
		var->field = NULL;
	}
	return var;
}


static struct function_call *parse_func_call(struct variable *func) {
	struct expression *args[MAX_ARGC];
	unsigned arg_count = 0;

	while (take().kind != SQ_TK_RPAREN && arg_count <= MAX_ARGC) {
		if (last.kind == SQ_TK_UNDEFINED) die("missing rparen for fn call");
		untake();

		if (!(args[arg_count++] = parse_expression()))
			die("invalid argument #%d found in function call", arg_count-1);

		if (take().kind != SQ_TK_COMMA) {
			if (last.kind != SQ_TK_RPAREN)
				die("missing rparen for fn call");
			break;
		}
	}

	struct function_call *fncall = xmalloc(sizeof(struct function_call));
	fncall->func = func;
	fncall->args = memdup(args, sizeof(struct expression *[arg_count]));

	fncall->arglen = arg_count;

	return fncall;
}

static struct func_declaration *parse_func_declaration();
static struct primary *parse_primary() {
	struct primary primary;

	switch (take().kind) {
	case SQ_TK_LPAREN:
		primary.kind = SQ_PS_PPAREN;
		primary.expr = parse_expression();
		EXPECT(SQ_TK_RPAREN, "expected a ')' at end of paren expr");
		break;
	case SQ_TK_NUMBER:
		primary.kind = SQ_PS_PNUMBER;
		primary.number = last.number;
		break;
	case SQ_TK_STRING:
		primary.kind = SQ_PS_PSTRING;
		primary.string = last.string;
		break;
	case SQ_TK_TRUE:
		primary.kind = SQ_PS_PBOOLEAN;
		primary.boolean = true;
		break;
	case SQ_TK_FALSE:
		primary.kind = SQ_PS_PBOOLEAN;
		primary.boolean = false;
		break;
	case SQ_TK_NULL:
		primary.kind = SQ_PS_PNULL;
		break;
	case SQ_TK_IDENT: {
		untake();
		struct variable *var = parse_variable();

		if (take().kind == SQ_TK_LPAREN) {
			primary.kind = SQ_PS_PPAREN;
			primary.expr = xmalloc(sizeof(struct expression));
			primary.expr->kind = SQ_PS_EFNCALL;
			primary.expr->fncall = parse_func_call(var);
		} else {
			untake();
			primary.kind = SQ_PS_PVARIABLE;
			primary.variable = var;
		}
		break;
	}
	case SQ_TK_FUNC: {
		untake();
		primary.kind = SQ_PS_PLAMBDA;
		primary.lambda = parse_func_declaration();
		break;
	}
	default:
		untake();
		return NULL;
	}

	return memdup(&primary, sizeof(struct primary));
}

static struct unary_expression *parse_unary_expression() {
	struct unary_expression unary;

	switch (take().kind) {
	case SQ_TK_NOT:
		unary.kind = SQ_PS_UNOT;
		break;
	case SQ_TK_SUB:
		unary.kind = SQ_PS_UNEG;
		break;
	default:
		unary.kind = SQ_PS_UPRIMARY;
		untake();
	}

	if (!(unary.rhs = parse_primary()))
		return NULL;

	return memdup(&unary, sizeof(struct unary_expression));
}

static struct mul_expression *parse_mul_expression() {
	struct mul_expression mul;

	if (!(mul.lhs = parse_unary_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_MUL:
		mul.kind = SQ_PS_MMUL;
		break;
	case SQ_TK_DIV:
		mul.kind = SQ_PS_MDIV;
		break;
	case SQ_TK_MOD:
		mul.kind = SQ_PS_MMOD;
		break;
	default:
		mul.kind = SQ_PS_MUNARY;
		untake();
	}

	if (mul.kind != SQ_PS_MUNARY) {
		if (!(mul.rhs = parse_mul_expression()))
			die("missing right-hand side for mul-like operation");
	}

	return memdup(&mul, sizeof(struct mul_expression));
}

static struct add_expression *parse_add_expression() {
	struct add_expression add;

	if (!(add.lhs = parse_mul_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_ADD:
		add.kind = SQ_PS_AADD;
		break;
	case SQ_TK_SUB:
		add.kind = SQ_PS_ASUB;
		break;
	default:
		add.kind = SQ_PS_AMUL;
		untake();
	}

	if (add.kind != SQ_PS_AMUL) {
		if (!(add.rhs = parse_add_expression()))
			die("missing right-hand side for add-like operation");
	}

	return memdup(&add, sizeof(struct add_expression));
}

static struct cmp_expression *parse_cmp_expression() {
	struct cmp_expression cmp;

	if (!(cmp.lhs = parse_add_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_LTH:
		cmp.kind = SQ_PS_CLTH;
		break;
	case SQ_TK_LEQ:
		cmp.kind = SQ_PS_CLEQ;
		break;
	case SQ_TK_GTH:
		cmp.kind = SQ_PS_CGTH;
		break;
	case SQ_TK_GEQ:
		cmp.kind = SQ_PS_CGEQ;
		break;
	default:
		cmp.kind = SQ_PS_CADD;
		untake();
	}

	if (cmp.kind != SQ_PS_CADD) {
		if (!(cmp.rhs = parse_cmp_expression()))
			die("missing right-hand side for cmp-like operation");
	}

	return memdup(&cmp, sizeof(struct cmp_expression));
}

static struct eql_expression *parse_eql_expression() {
	struct eql_expression eql;

	if (!(eql.lhs = parse_cmp_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_EQL:
		eql.kind = SQ_PS_EEQL;
		break;
	case SQ_TK_NEQ:
		eql.kind = SQ_PS_ENEQ;
		break;
	default:
		eql.kind = SQ_PS_ECMP;
		untake();
	}

	if (eql.kind != SQ_PS_ECMP) {
		if (!(eql.rhs = parse_eql_expression()))
			die("missing right-hand side for eql-like operation");
	}

	return memdup(&eql, sizeof(struct eql_expression));
}

static struct bool_expression *parse_bool_expression() {
	struct bool_expression eql;

	if (!(eql.lhs = parse_eql_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_AND:
		eql.kind = SQ_PS_BAND;
		break;
	case SQ_TK_OR:
		eql.kind = SQ_PS_BOR;
		break;
	default:
		eql.kind = SQ_PS_BEQL;
		untake();
	}

	if (eql.kind != SQ_PS_BEQL) {
		if (!(eql.rhs = parse_bool_expression()))
			die("missing right-hand side for boolean-like operation");
	}

	return memdup(&eql, sizeof(struct bool_expression));
}

static struct assignment *parse_assignment(struct variable *var) {
	GUARD(SQ_TK_ASSIGN);

	struct assignment *asgn = xmalloc(sizeof(struct assignment));
	asgn->var = var;
	if (!(asgn->expr = parse_expression()))
		die("missing rhs for assignment");
	return asgn;
}

static struct expression *parse_expression() {
	struct expression expr;
	expr.kind = SQ_PS_EMATH;

	if (!(expr.math = parse_bool_expression()))
		return NULL;

	if (expr.math->kind != SQ_PS_BEQL
		|| expr.math->lhs->kind != SQ_PS_ECMP
		|| expr.math->lhs->lhs->kind != SQ_PS_CADD
		|| expr.math->lhs->lhs->lhs->kind != SQ_PS_AMUL
		|| expr.math->lhs->lhs->lhs->lhs->kind != SQ_PS_MUNARY
		|| expr.math->lhs->lhs->lhs->lhs->lhs->kind != SQ_PS_UPRIMARY
		|| expr.math->lhs->lhs->lhs->lhs->lhs->rhs->kind != SQ_PS_PVARIABLE
	) goto done;

	struct variable *var = expr.math->lhs->lhs->lhs->lhs->lhs->rhs->variable;
	take();
	untake();

	if (last.kind == SQ_TK_ASSIGN) {
		expr.kind = SQ_PS_EASSIGN;
		expr.asgn = parse_assignment(var);
	}

done:

	return memdup(&expr, sizeof(struct expression));
}

static struct global_declaration *parse_global_declaration() {
	GUARD(SQ_TK_GLOBAL);
	struct global_declaration *global = xmalloc(sizeof(struct global_declaration));

	EXPECT(SQ_TK_IDENT, "expected an identifier after 'global'");
	global->name = last.identifier;
	if (take().kind == SQ_TK_ASSIGN) {
		global->value = parse_expression();
	} else {
		untake();
		global->value = NULL;
	}

	return global;
}

static char *parse_import_declaration() {
	GUARD(SQ_TK_IMPORT);
	EXPECT(SQ_TK_STRING, "expected a string after 'import1'");
	return last.string->ptr;
}

static struct struct_declaration *parse_struct_declaration() {
	GUARD(SQ_TK_STRUCT);
	struct struct_declaration *sdecl = xmalloc(sizeof(struct struct_declaration));

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		sdecl->name = last.identifier;
	} else {
		untake();
		sdecl->name = strdup("<anonymous>");
	}

	// require a lparen.
	EXPECT(SQ_TK_LBRACE, "expected '{' before struct fields");
	parse_field_names();
	EXPECT(SQ_TK_RBRACE, "expected '}' after struct fields");

	sdecl->nfields = field_name_count;
	sdecl->fields = memdup(field_names, sizeof(char *[field_name_count]));
	return sdecl;
}

static struct statements *parse_statements(void);

static struct statements *parse_brace_statements(char *what) {
	struct statements *stmts;
	EXPECT(SQ_TK_LBRACE, "missing '{' for '%s' body", what);
	if (!(stmts = parse_statements()))
		die("missing body for '%s'", what);
	EXPECT(SQ_TK_RBRACE, "missing '}' for '%s' body", what);
	return stmts;
}

static struct func_declaration *parse_func_declaration() {
	GUARD(SQ_TK_FUNC);
	struct func_declaration *fdecl = xmalloc(sizeof(struct func_declaration));

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		fdecl->name = last.identifier;
	} else {
		untake();
		fdecl->name = strdup("<anonymous>");
	}

	// require a lparen.
	EXPECT(SQ_TK_LPAREN, "expected '(' before func fields");
	parse_field_names();
	EXPECT(SQ_TK_RPAREN, "expected ')' after func fields");

	fdecl->nargs = field_name_count;
	fdecl->args = memdup(field_names, sizeof(char *[field_name_count]));
	if (!(fdecl->body = parse_brace_statements("func")))
		die("no body given for function");
	return fdecl;
}

static struct if_statement *parse_if_statement() {
	GUARD(SQ_TK_IF);
	struct if_statement *if_stmt = xmalloc(sizeof(struct if_statement));
	if (!(if_stmt->cond = parse_expression()))
		die("missing condition for 'if'");

	if_stmt->iftrue = parse_brace_statements("if");

	if (take().kind == SQ_TK_ELSE) {
		take();
		untake();
		if (last.kind == SQ_TK_IF) {
			if_stmt->iffalse = xmalloc(sizeof(struct statements));
			if_stmt->iffalse->len = 2;
			if_stmt->iffalse->stmts = xmalloc(sizeof(struct statement *[2]));
			if_stmt->iffalse->stmts[0] = xmalloc(sizeof(struct statement));
			if_stmt->iffalse->stmts[0]->kind = SQ_PS_SIF;
			if_stmt->iffalse->stmts[0]->ifstmt = parse_if_statement();
			if_stmt->iffalse->stmts[1] = NULL;
		} else {
			if_stmt->iffalse = parse_brace_statements("else");
		}
	} else {
		untake();
		if_stmt->iffalse = NULL;
	}

	return if_stmt;
}

static struct while_statement *parse_while_statement() {
	GUARD(SQ_TK_WHILE);
	struct while_statement *while_stmt = xmalloc(sizeof(struct while_statement));
	if (!(while_stmt->cond = parse_expression()))
		die("missing condition for 'while'");

	while_stmt->body = parse_brace_statements("while");
	return while_stmt;
}

static struct return_statement *parse_return_statement() {
	GUARD(SQ_TK_RETURN);
	struct return_statement *ret_stmt = xmalloc(sizeof(struct return_statement));

	ret_stmt->value = parse_expression();

	return ret_stmt;
}

static struct statement *parse_statement() {
	struct statement stmt;
	if ((stmt.gdecl = parse_global_declaration())) stmt.kind = SQ_PS_SGLOBAL;
	else if ((stmt.import = parse_import_declaration())) stmt.kind = SQ_PS_SIMPORT;
	else if ((stmt.sdecl = parse_struct_declaration())) stmt.kind = SQ_PS_SSTRUCT;
	else if ((stmt.fdecl = parse_func_declaration())) stmt.kind = SQ_PS_SFUNC;
	else if ((stmt.ifstmt = parse_if_statement())) stmt.kind = SQ_PS_SIF;
	else if ((stmt.wstmt = parse_while_statement())) stmt.kind = SQ_PS_SWHILE;
	else if ((stmt.rstmt = parse_return_statement())) stmt.kind = SQ_PS_SRETURN;
	else if ((stmt.expr = parse_expression())) stmt.kind = SQ_PS_SEXPR;
	else return NULL;

	return memdup(&stmt, sizeof(struct statement));
}

static struct statements *parse_statements() {
	unsigned cap = 256, len=0;
	struct statement **list = xmalloc(sizeof(struct statement *[cap]));

	bool endl = true;
	while ((list[len] = parse_statement())) {
		// if (!endl && list[len-1]->kind == SQ_PS_SEXPR) die("missing `;` between statements");
		if (++len == cap - 1)
			list = xrealloc(list, sizeof(struct statement *[cap*=2]));

		endl = false;
		while (take_endline().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL)
			endl = true;
		untake(); // as the while statement broke it.
	}

	struct statements *stmts = xmalloc(sizeof(struct statements));
	stmts->len = len;
	stmts->stmts = list;
	return stmts;
}

struct statements *sq_parse_statements(const char *stream) {
	last.kind = SQ_TK_UNDEFINED;
	rewound = false;
	sq_stream = stream;
	return parse_statements();
}

