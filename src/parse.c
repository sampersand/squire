#include "token.h"
#include "program.h"
#include "function.h"
#include "parse.h"
#include "shared.h"
#include "class.h"
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

static void parse_field_names(bool is_method) {
	field_name_count = 0;

	if (is_method)
		field_names[field_name_count++] = strdup("my");

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


static char *token_to_identifier(struct sq_token token) {
	switch (token.kind) {
	case SQ_TK_IDENT: return last.identifier;
	case SQ_TK_EQL: return strdup("==");
	case SQ_TK_LTH: return strdup("<");
	case SQ_TK_LEQ: return strdup("<=");
	case SQ_TK_GTH: return strdup(">");
	case SQ_TK_GEQ: return strdup(">=");
	case SQ_TK_ADD: return strdup("+");
	case SQ_TK_SUB: return strdup("-");
	case SQ_TK_MUL: return strdup("*");
	case SQ_TK_DIV: return strdup("/");
	case SQ_TK_MOD: return strdup("%");
	default: return NULL;
	}

}
static struct variable *parse_variable(void) {
	struct variable *var = xmalloc(sizeof(struct variable));

	if (!(var->name = token_to_identifier(take())))
		return untake(), free(var), NULL;

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

static struct array_index *parse_array_index(struct variable *array) {
	struct array_index *array_index = xmalloc(sizeof(struct array_index));
	array_index->array = array;
	if (!(array_index->index = parse_expression())) die("Cant compile array index");
	EXPECT(SQ_TK_RBRACKET, "expected a ']' at end of index");
	return array_index;
}

static struct array *parse_array() {
	unsigned len = 0, cap = 8;
	struct expression **args = xmalloc(sizeof(struct expression[cap]));

	while ((take(),untake(),last.kind != SQ_TK_RBRACKET)) {
		if (last.kind == SQ_TK_UNDEFINED)
			die("missing rparen for array call");

		if (len == cap)
			args = xrealloc(args, sizeof(struct expression[cap *= 2]));

		args[len++] = parse_expression();

		if (take().kind != SQ_TK_COMMA) {
			untake();
			break;
		}
	}

	EXPECT(SQ_TK_RBRACKET, "expected a ']' at end of array");

	struct array *array = xmalloc(sizeof(struct array));
	array->nargs = len;
	array->args = args;
	return array;
}

static struct func_declaration *parse_func_declaration(bool, bool);
static struct primary *parse_primary() {
	struct primary primary;

	switch (take().kind) {
	case SQ_TK_LPAREN:
		primary.kind = SQ_PS_PPAREN;
		primary.expr = parse_expression();
		EXPECT(SQ_TK_RPAREN, "expected a ')' at end of paren expr");
		break;
	case SQ_TK_LBRACKET:
		primary.kind = SQ_PS_PARRAY;
		primary.array = parse_array();
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
		} else if (last.kind == SQ_TK_LBRACKET) {
			primary.kind = SQ_PS_PARRAY_INDEX;
			primary.array_index = parse_array_index(var);
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
		primary.lambda = parse_func_declaration(true, false);
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

static struct array_assignment *parse_array_assignment(struct array_index *aidx) {
	GUARD(SQ_TK_ASSIGN);

	struct array_assignment *ary_asgn = xmalloc(sizeof(struct array_assignment));

	ary_asgn->var = aidx->array;
	ary_asgn->index = aidx->index;
	free(aidx);

	if (!(ary_asgn->value = parse_expression()))
		die("cannot parse value for ary assignment");

	return ary_asgn;
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
		|| (expr.math->lhs->lhs->lhs->lhs->lhs->rhs->kind != SQ_PS_PVARIABLE
		&& expr.math->lhs->lhs->lhs->lhs->lhs->rhs->kind != SQ_PS_PARRAY_INDEX)
	) goto done;

	struct primary *prim = expr.math->lhs->lhs->lhs->lhs->lhs->rhs;

	take();
	untake();

	if (last.kind == SQ_TK_ASSIGN) {
		if (prim->kind == SQ_PS_PVARIABLE) {
			expr.kind = SQ_PS_EASSIGN;
			expr.asgn = parse_assignment(prim->variable);
		} else {
			expr.kind = SQ_PS_EARRAY_ASSIGN;
			expr.ary_asgn = parse_array_assignment(prim->array_index);
		}
	}

done:

	return memdup(&expr, sizeof(struct expression));
}

static struct scope_declaration *parse_global_declaration() {
	GUARD(SQ_TK_GLOBAL);
	struct scope_declaration *global = xmalloc(sizeof(struct scope_declaration));

	EXPECT(SQ_TK_IDENT, "expected an identifier after 'renowned'");
	global->name = last.identifier;
	if (take().kind == SQ_TK_ASSIGN) {
		global->value = parse_expression();
	} else {
		untake();
		global->value = NULL;
	}

	return global;
}
static struct scope_declaration *parse_local_declaration() {
	GUARD(SQ_TK_LOCAL);
	struct scope_declaration *local = xmalloc(sizeof(struct scope_declaration));

	EXPECT(SQ_TK_IDENT, "expected an identifier after 'nigh'");
	local->name = last.identifier;
	if (take().kind == SQ_TK_ASSIGN) {
		local->value = parse_expression();
	} else {
		untake();
		local->value = NULL;
	}

	return local;
}

static char *parse_import_declaration() {
	GUARD(SQ_TK_IMPORT);
	EXPECT(SQ_TK_STRING, "expected a string after 'import'");
	return last.string->ptr;
}

static struct class_declaration *parse_class_declaration() {
	GUARD(SQ_TK_CLASS);
	struct class_declaration *cdecl = xmalloc(sizeof(struct class_declaration));

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		cdecl->name = last.identifier;
	} else {
		untake();
		cdecl->name = strdup("<anonymous>");
	}

	// require a lparen.
	EXPECT(SQ_TK_LBRACE, "expected '{' before 'form' contents");

#define MAX_LEN 256 // having more than this is a god object anyways.
	cdecl->fields = xmalloc(sizeof(char *[MAX_LEN]));
	cdecl->meths = xmalloc(sizeof(struct sq_function *[MAX_LEN]));
	cdecl->funcs = xmalloc(sizeof(struct sq_function *[MAX_LEN]));
	cdecl->constructor = NULL;
	cdecl->nfields = 0;
	cdecl->nfuncs = 0;
	cdecl->nmeths = 0;

	while (take().kind != SQ_TK_RBRACE) {
		struct sq_token tkn = last;

		switch (tkn.kind) {
		case SQ_TK_METHOD:
		case SQ_TK_CLASSFN:
		case SQ_TK_CONSTRUCTOR: {
			struct func_declaration *fn = parse_func_declaration(false, true);
			if (tkn.kind == SQ_TK_CONSTRUCTOR) {
				if (cdecl->constructor != NULL)
					die("cannot have two constructors.");
				cdecl->constructor = fn;
			} else if (tkn.kind == SQ_TK_METHOD) {
				if (cdecl->nmeths >= MAX_LEN)
					die("too many methods!");
				cdecl->meths[cdecl->nmeths++] = fn;
			} else if (tkn.kind == SQ_TK_CLASSFN) {
				if (cdecl->nfuncs >= MAX_LEN)
					die("too many class methods!");
				cdecl->funcs[cdecl->nfuncs++] = fn;
			} else {
				die("[bug] its not a constructor, func, or classfn?");
			}

			break;
		}

		case SQ_TK_FIELD:
			while (take().kind == SQ_TK_IDENT) {
				if (cdecl->nfields > MAX_LEN) die("too many fields!");
				cdecl->fields[cdecl->nfields++] = last.identifier;
				if (take().kind != SQ_TK_COMMA) {
					untake();
					break;
				}
			}

			break;

		case SQ_TK_ENDL:
		case SQ_TK_SOFT_ENDL:
			continue;

		default:
			die("unknown token encountered when parsing 'form'.");
		}
	}

	untake();

#undef MAX_LEN

	cdecl->fields = xrealloc(cdecl->fields, sizeof(char *[cdecl->nfields]));
	cdecl->meths = xrealloc(cdecl->meths, sizeof(struct sq_function *[cdecl->nmeths]));
	cdecl->funcs = xrealloc(cdecl->funcs, sizeof(struct sq_function *[cdecl->nfuncs]));

	EXPECT(SQ_TK_RBRACE, "expected '}' after 'form' fields");
	return cdecl;
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

static struct func_declaration *parse_func_declaration(bool guard, bool is_method) {
	if (guard)
		GUARD(SQ_TK_FUNC);
	struct func_declaration *fdecl = xmalloc(sizeof(struct func_declaration));

	// optional name
	if (take().kind == SQ_TK_LPAREN) {
		untake();
		fdecl->name = strdup("<anonymous>");
	} else if (!(fdecl->name = token_to_identifier(last))) {
		die("unexpected token in func declaration list");
	}

	// require a lparen.
	EXPECT(SQ_TK_LPAREN, "expected '(' before func fields");
	parse_field_names(is_method);
	EXPECT(SQ_TK_RPAREN, "expected ')' after func fields");

	fdecl->nargs = field_name_count;
	fdecl->args = memdup(field_names, sizeof(char *[field_name_count]));
	if (!(fdecl->body = parse_brace_statements("journey")))
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
			if_stmt->iffalse->len = 1;
			if_stmt->iffalse->stmts = xmalloc(sizeof(struct statement *[2]));
			if_stmt->iffalse->stmts[0] = xmalloc(sizeof(struct statement));
			if_stmt->iffalse->stmts[0]->kind = SQ_PS_SIF;
			if_stmt->iffalse->stmts[0]->ifstmt = parse_if_statement();
			if_stmt->iffalse->stmts[1] = NULL;
		} else {
			if_stmt->iffalse = parse_brace_statements("alas");
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
		die("missing condition for 'whilst'");

	while_stmt->body = parse_brace_statements("whilst");
	return while_stmt;
}

static struct return_statement *parse_return_statement() {
	GUARD(SQ_TK_RETURN);
	struct return_statement *ret_stmt = xmalloc(sizeof(struct return_statement));

	ret_stmt->value = parse_expression();

	return ret_stmt;
}

static struct expression *parse_throw_statement() {
	GUARD(SQ_TK_THROW);
	struct expression *expression = parse_expression();
	if (!expression) die("expected expression after 'hark'");
	return expression;
}

static bool parse_undo_statement() {
	GUARD(SQ_TK_THROW);
	return true;
}

static struct trycatch_statement *parse_trycatch_statement() {
	GUARD(SQ_TK_TRY);

	struct trycatch_statement *tc = xmalloc(sizeof(struct trycatch_statement));
	tc->try = parse_brace_statements("attempt");
	EXPECT(SQ_TK_CATCH, "expected 'retreat' after 'attempt'");
	EXPECT(SQ_TK_IDENT, "expected an identifier after 'retreat'");
	tc->exception = last.identifier;
	tc->catch = parse_brace_statements("retreat");

	return tc;
}

static char *parse_label_declaration()  {
	if (take().kind == SQ_TK_LABEL)
		return last.identifier;

	untake();
	return NULL;
}

static char *parse_comefrom_declaration() {
	GUARD(SQ_TK_COMEFROM);

	if (take().kind != SQ_TK_IDENT)
		die("expecting an identifier");

	return last.identifier;
}

static struct statement *parse_statement() {
	struct statement stmt;

	while (take().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL) {}
	untake();

	if ((stmt.gdecl = parse_global_declaration())) stmt.kind = SQ_PS_SGLOBAL;
	else if ((stmt.ldecl = parse_local_declaration())) stmt.kind = SQ_PS_SLOCAL;
	else if ((stmt.import = parse_import_declaration())) stmt.kind = SQ_PS_SIMPORT;
	else if ((stmt.label = parse_label_declaration())) stmt.kind = SQ_PS_SLABEL;
	else if ((stmt.comefrom = parse_comefrom_declaration())) stmt.kind = SQ_PS_SCOMEFROM;
	else if ((stmt.cdecl = parse_class_declaration())) stmt.kind = SQ_PS_SCLASS;
	else if ((stmt.fdecl = parse_func_declaration(true, false))) stmt.kind = SQ_PS_SFUNC;
	else if ((stmt.ifstmt = parse_if_statement())) stmt.kind = SQ_PS_SIF;
	else if ((stmt.wstmt = parse_while_statement())) stmt.kind = SQ_PS_SWHILE;
	else if ((stmt.rstmt = parse_return_statement())) stmt.kind = SQ_PS_SRETURN;
	else if ((stmt.tcstmt = parse_trycatch_statement())) stmt.kind = SQ_PS_STRYCATCH;
	else if ((stmt.throwstmt = parse_throw_statement())) stmt.kind = SQ_PS_STHROW;
	else if (parse_undo_statement()) stmt.kind = SQ_PS_SUNDO;
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

	while (take().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL) {
		// do nothing
	}
	untake();

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

