#include <squire/token.h>
#include <squire/program.h>
#include <squire/journey.h>
#include <squire/parse.h>
#include <squire/shared.h>
#include <squire/form.h>

#include <string.h>

#define parse_error sq_throw
static struct sq_token last;
static bool rewound;

static void untake() {
	sq_assert(!rewound);
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


#define EXPECTED(kind_, iffalse) \
	do { if (take().kind != kind_) { iffalse; } } while(0)
#define EXPECT(kind_, ...) EXPECTED(kind_, sq_throw(__VA_ARGS__))
#define GUARD(kind_) EXPECTED(kind_, untake(); return NULL)

static struct expression *parse_expression(void);
static struct expression *parse_expression_no_assignment(void);

static char *token_to_identifier(struct sq_token token) {
	switch (token.kind) {
	case SQ_TK_IDENT: return last.identifier;
	case SQ_TK_EQL: return strdup("==");
	case SQ_TK_LTH: return strdup("<");
	case SQ_TK_LEQ: return strdup("<=");
	case SQ_TK_GTH: return strdup(">");
	case SQ_TK_GEQ: return strdup(">=");
	case SQ_TK_CMP: return strdup("<=>");
	case SQ_TK_ADD: return strdup("+");
	case SQ_TK_SUB: return strdup("-");
	case SQ_TK_NEG: return strdup("-@");
	case SQ_TK_MUL: return strdup("*");
	case SQ_TK_POW: return strdup("^");
	case SQ_TK_DIV: return strdup("/");
	case SQ_TK_MOD: return strdup("%");
	case SQ_TK_INDEX: return strdup("[]");
	case SQ_TK_INDEX_ASSIGN: return strdup("[]=");
	default: return NULL;
	}

}
static struct variable_old *parse_variable(void) {
	struct variable_old *var = sq_malloc_single(struct variable_old);

	if (!(var->name = token_to_identifier(take())))
		return untake(), free(var), NULL;

	if (take().kind == SQ_TK_DOT || last.kind == SQ_TK_COLONCOLON) {
		var->is_namespace_access = (last.kind == SQ_TK_COLONCOLON);
		var->field = parse_variable();
	} else {
		untake();
		var->field = NULL;
	}
	return var;
}

static struct function_call_old *parse_func_call_old(struct variable_old *func) {
	struct expression *args[SQ_JOURNEY_MAX_ARGC];
	unsigned arg_count = 0;

	while (take().kind != SQ_TK_RPAREN && arg_count <= SQ_JOURNEY_MAX_ARGC) {
		if (last.kind == SQ_TK_UNDEFINED) sq_throw("missing rparen for fn call");
		untake();

		if (!(args[arg_count++] = parse_expression()))
			sq_throw("invalid argument #%d found in function call", arg_count-1);

		if (take().kind != SQ_TK_COMMA) {
			if (last.kind != SQ_TK_RPAREN)
				sq_throw("missing rparen for fn call");
			break;
		}
	}

	struct function_call_old *fncall = sq_malloc_single(struct function_call_old);
	fncall->func = func;
	fncall->args = sq_memdup(args, sq_sizeof_array(struct expression *, arg_count));

	fncall->arglen = arg_count;

	return fncall;
}

static void parse_func_call(struct function_call *fncall) {
	struct expression *args[SQ_JOURNEY_MAX_ARGC];
	fncall->argc = 0;

	while (take().kind != SQ_TK_RPAREN && fncall->argc <= SQ_JOURNEY_MAX_ARGC) {
		if (last.kind == SQ_TK_UNDEFINED)
			sq_throw("missing rparen for fn call");

		untake();

		if (!(args[fncall->argc++] = parse_expression()))
			sq_throw("invalid argument #%d found in function call", fncall->argc-1);

		if (take().kind != SQ_TK_COMMA) {
			if (last.kind != SQ_TK_RPAREN)
				sq_throw("missing rparen for fn call");
			break;
		}
	}

	fncall->args = sq_memdup(args, sq_sizeof_array(struct expression *, fncall->argc));
}

// static struct index *parse_index(struct primary *primary) {
// 	GUARD(SQ_TK_LBRACKET);

// 	struct index *index = sq_malloc_single(sruct index));
// 	index->into = primary;
// 	if (!(index->index = parse_expression())) sq_throw("Cant compile index");
// 	EXPECT(SQ_TK_RBRACKET, "expected a ']' at end of index");
// 	return index;
// }

static struct book *parse_book() {
	unsigned len = 0, cap = 8;
	struct expression **pages = sq_malloc_vec(struct expression *, cap);

	while ((take(),untake(),last.kind != SQ_TK_RBRACKET)) {
		if (last.kind == SQ_TK_UNDEFINED)
			sq_throw("missing rparen for book initialization");

		if (len == cap)
			pages = sq_realloc_vec(struct expression *, pages, cap *= 2);

		pages[len++] = parse_expression();

		if (take().kind != SQ_TK_COMMA) {
			untake();
			break;
		}
	}

	EXPECT(SQ_TK_RBRACKET, "expected a ']' at end of book");

	struct book *book = sq_malloc_single(struct book);
	book->npages = len;
	book->pages = pages;
	return book;
}

static struct dict *parse_codex() {
	unsigned len = 0, cap = 8;
	struct expression **keys = sq_malloc_vec(struct expression *, cap);
	struct expression **vals = sq_malloc_vec(struct expression *, cap);

	while ((take(),untake(),last.kind != SQ_TK_RBRACE)) {
		if (last.kind == SQ_TK_UNDEFINED)
			sq_throw("missing rparen for codex call");

		if (len == cap) {
			cap *= 2;
			keys = sq_realloc_vec(struct expression *, keys, cap);
			vals = sq_realloc_vec(struct expression *, vals, cap);
		}

		bool was_label;
		if (take(), untake(), was_label = (last.kind == SQ_TK_LABEL))
			last.kind = SQ_TK_IDENT;

		keys[len] = parse_expression();

		if (!was_label)
			EXPECT(SQ_TK_COLON, "expected a ':' after codex key");

		vals[len] = parse_expression();
		++len;

		if (take().kind != SQ_TK_COMMA) {
			untake();
			break;
		}
	}

	EXPECT(SQ_TK_RBRACE, "expected a '}' at end of codex");

	struct dict *dict = sq_malloc_single(struct dict);
	dict->neles = len;
	dict->keys = keys;
	dict->vals = vals;
	return dict;
}

static struct journey_declaration *parse_journey_declaration(bool, bool, bool);
static struct primary *parse_primary() {
	struct primary primary;
	int kind;

	switch (kind = take().kind) {
	case SQ_TK_LPAREN:
		primary.kind = SQ_PS_PPAREN;
		primary.expr = parse_expression();
		EXPECT(SQ_TK_RPAREN, "expected a ')' at end of paren expr");
		break;

	case SQ_TK_CITE:
		primary.kind = SQ_PS_PCITE;
		primary.expr = parse_expression();
		break;

	case SQ_TK_BABEL:
		primary.kind = SQ_PS_PBABEL;
		if (!(primary.babel.executable = parse_primary()))
			parse_error("expected a primary after `babel`");

		if (!(primary.babel.nargs = (take().kind == SQ_TK_LBRACE)))
			untake();
		else {
			primary.babel.nargs = 0;
			while (take().kind != SQ_TK_RBRACE) {
				untake();
				if (SQ_BABEL_MAX_ARGC == primary.babel.nargs)
					parse_error("too many arguments in babel expr (%d max)", SQ_BABEL_MAX_ARGC);

				if (!(primary.babel.args[primary.babel.nargs++] = parse_expression()))
						parse_error("expected a expression within args.");

				if (take().kind == SQ_TK_COMMA) 
					continue;

				untake();
				EXPECT(SQ_TK_RBRACE, "expected `}` or a text within args.");
				break;
			}
		}
		if (!(primary.babel.stdin = parse_primary()))
			parse_error("expected stdin after `babel`'s excecutable (and possibly args)");
		break;

	case SQ_TK_INDEX:
		primary.kind = SQ_PS_PBOOK;
		primary.book = sq_malloc_single(struct book);
		primary.book->npages = 0;
		primary.book->pages = NULL;
		break;
	case SQ_TK_LBRACKET:
		primary.kind = SQ_PS_PBOOK;
		primary.book = parse_book();
		break;
	case SQ_TK_LBRACE:
		primary.kind = SQ_PS_PCODEX;
		primary.dict = parse_codex();
		break;
	case SQ_TK_NUMERAL:
		primary.kind = SQ_PS_PNUMERAL;
		primary.numeral = last.numeral;
		break;
	case SQ_TK_TEXT:
		primary.kind = SQ_PS_PTEXT;
		primary.text = last.text;
		break;
	case SQ_TK_YAY:
		primary.kind = SQ_PS_PVERACITY;
		primary.veracity = true;
		break;
	case SQ_TK_NAY:
		primary.kind = SQ_PS_PVERACITY;
		primary.veracity = false;
		break;
	case SQ_TK_NI:
		primary.kind = SQ_PS_PNI;
		break;
	case SQ_TK_LABEL: {
		untake();
		last.kind = SQ_TK_IDENT;
		struct variable_old *var = sq_malloc_single(struct variable_old);

		if (!(var->name = token_to_identifier(take())))
			return untake(), free(var), NULL;
		var->field = NULL;
		primary.kind = SQ_PS_PVARIABLE;
		primary.variable = var->name;
		free(var);
		untake();
		last.kind = SQ_TK_COLON;
		break;
	}

	case SQ_TK_IDENT: {
		untake();
		struct variable_old *var = parse_variable();

		if (take().kind == SQ_TK_LPAREN) {
			primary.kind = SQ_PS_PPAREN;
			primary.expr = sq_malloc_single(struct expression);
			primary.expr->kind = SQ_PS_EFNCALL;
			primary.expr->fncall = parse_func_call_old(var);
		} else {
			untake();
			if (!var->field) {
				primary.kind = SQ_PS_PVARIABLE;
				primary.variable = var->name;
			} else {
				primary.kind = SQ_PS_PVARIABLE_OLD;
				primary.variable_old = var;
			}
		}
		break;
	}
	case SQ_TK_FUNC:
	case SQ_TK_LAMBDA: {
		primary.kind = SQ_PS_PLAMBDA;
		primary.lambda = parse_journey_declaration(false, false, kind == SQ_TK_FUNC);
		break;
	}
	default:
		untake();
		return NULL;
	}

	struct primary *prim_ptr;

reparse_primary:
	prim_ptr = sq_memdup(&primary, sizeof(struct primary));

	switch (take().kind) {
	case SQ_TK_LPAREN:
		primary.kind = SQ_PS_PFNCALL;

		if (prim_ptr->kind == SQ_PS_PFACCESS) {
			primary.fncall.soul = prim_ptr->faccess.soul;
			primary.fncall.field = prim_ptr->faccess.field;
			free(prim_ptr);
		} else {
			primary.fncall.soul = prim_ptr;
			primary.fncall.field = NULL;
		}

		parse_func_call(&primary.fncall);
		goto reparse_primary;

	case SQ_TK_LBRACKET:
		primary.kind = SQ_PS_PINDEX;
		primary.index.into = prim_ptr;
		if (!(primary.index.index = parse_expression()))
			sq_throw("Cant parse index expression");
		EXPECT(SQ_TK_RBRACKET, "expected a ']' at end of index");
		goto reparse_primary;

	case SQ_TK_DOT:
		primary.kind = SQ_PS_PFACCESS;
		primary.faccess.soul = prim_ptr;
		EXPECT(SQ_TK_IDENT, "expected an identifier after '.' for field access");
		primary.faccess.field = last.identifier;
		goto reparse_primary;

	default:
		untake();
	}

	return prim_ptr;
}

static struct unary_expression *parse_unary_expression() {
	struct unary_expression unary;

	switch (take().kind) {
	case SQ_TK_NOT:
		unary.kind = SQ_PS_UNOT;
		break;
	case SQ_TK_PAT_NOT:
		unary.kind = SQ_PS_UPAT_NOT;
		break;
	case SQ_TK_NEG:
	case SQ_TK_SUB:
		unary.kind = SQ_PS_UNEG;
		break;
	default:
		unary.kind = SQ_PS_UPRIMARY;
		untake();
	}

	if (!(unary.rhs = parse_primary()))
		return NULL;

	return sq_memdup(&unary, sizeof(struct unary_expression));
}

static struct pow_expression *parse_pow_expression() {
	struct pow_expression pow;

	if (!(pow.lhs = parse_unary_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_POW:
		pow.kind = SQ_PS_PPOW;
		break;
	default:
		pow.kind = SQ_PS_PUNARY;
		untake();
	}

	if (pow.kind != SQ_PS_PUNARY) {
		if (!(pow.rhs = parse_pow_expression()))
			sq_throw("missing right-hand side for pow-like operation");
	}

	return sq_memdup(&pow, sizeof(struct pow_expression));
}

static struct mul_expression *parse_mul_expression() {
	struct mul_expression mul;

	if (!(mul.lhs = parse_pow_expression()))
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
		mul.kind = SQ_PS_MPOW;
		untake();
	}

	if (mul.kind != SQ_PS_MPOW) {
		if (!(mul.rhs = parse_mul_expression()))
			sq_throw("missing right-hand side for mul-like operation");
	}

	return sq_memdup(&mul, sizeof(struct mul_expression));
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
			sq_throw("missing right-hand side for add-like operation");
	}

	return sq_memdup(&add, sizeof(struct add_expression));
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
	case SQ_TK_CMP:
		cmp.kind = SQ_PS_CCMP;
		break;
	default:
		cmp.kind = SQ_PS_CADD;
		untake();
	}

	if (cmp.kind != SQ_PS_CADD) {
		if (!(cmp.rhs = parse_cmp_expression()))
			sq_throw("missing right-hand side for cmp-like operation");
	}

	return sq_memdup(&cmp, sizeof(struct cmp_expression));
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
	case SQ_TK_MATCHES:
		eql.kind = SQ_PS_EMATCHES;
		break;
	case SQ_TK_PAT_AND:
		eql.kind = SQ_PS_EAND_PAT;
		break;
	case SQ_TK_PAT_OR:
		eql.kind = SQ_PS_EOR_PAT;
		break;
	default:
		eql.kind = SQ_PS_ECMP;
		untake();
	}

	if (eql.kind != SQ_PS_ECMP) {
		if (!(eql.rhs = parse_eql_expression()))
			sq_throw("missing right-hand side for eql-like operation");
	}

	return sq_memdup(&eql, sizeof(struct eql_expression));
}

static struct bool_expression *parse_bool_expression() {
	struct bool_expression eql;

	if (!(eql.lhs = parse_eql_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_AND: eql.kind = SQ_PS_BAND; break;
	case SQ_TK_OR:  eql.kind = SQ_PS_BOR; break;
	default: eql.kind = SQ_PS_BEQL; untake();
	}

	if (eql.kind != SQ_PS_BEQL) {
		if (!(eql.rhs = parse_bool_expression()))
			sq_throw("missing right-hand side for bool-like operation");
	}

	return sq_memdup(&eql, sizeof(struct bool_expression));
}

static struct assignment *parse_assignment(struct variable_old *var) {
	GUARD(SQ_TK_ASSIGN);

	struct assignment *asgn = sq_malloc_single(struct assignment);
	asgn->var = var;
	if (!(asgn->expr = parse_expression()))
		sq_throw("missing rhs for assignment");
	return asgn;
}

static struct index_assign *parse_index_assign(struct index *aidx) {
	GUARD(SQ_TK_ASSIGN);

	struct index_assign *ary_asgn = sq_malloc_single(struct index_assign);

	ary_asgn->into = aidx->into;
	ary_asgn->index = aidx->index;

	if (!(ary_asgn->value = parse_expression()))
		sq_throw("cannot parse value for ary assignment");

	return ary_asgn;
}

static struct expression *parse_expression_inner(bool include_assignment, struct expression *expr) {
	take();
	untake();

	if (expr->math->kind != SQ_PS_BEQL
		|| expr->math->lhs->kind != SQ_PS_ECMP
		|| expr->math->lhs->lhs->kind != SQ_PS_CADD
		|| expr->math->lhs->lhs->lhs->kind != SQ_PS_AMUL
		|| expr->math->lhs->lhs->lhs->lhs->kind != SQ_PS_MPOW
		|| expr->math->lhs->lhs->lhs->lhs->lhs->kind != SQ_PS_PUNARY
		|| expr->math->lhs->lhs->lhs->lhs->lhs->lhs->kind != SQ_PS_UPRIMARY
	) goto end; // it's cursed enough, why not throw a `goto` in anyways

	struct primary *prim = expr->math->lhs->lhs->lhs->lhs->lhs->lhs->rhs;

	if (prim->kind == SQ_PS_PINDEX && last.kind == SQ_TK_ASSIGN) {
		expr->kind = SQ_PS_EARRAY_ASSIGN;
		expr->ary_asgn = parse_index_assign(&prim->index);
		return expr;
	}

	if (last.kind == SQ_TK_LBRACKET) {
		// expr->kind = SQ_PS_EINDEX;
		// expr->index = parse_index(prim);
		// return parse_expression_inner(expr);
	}

	if (include_assignment) {
		if (last.kind == SQ_TK_ASSIGN && prim->kind == SQ_PS_PVARIABLE) {
			struct variable_old *var = sq_malloc_single(struct variable_old);
			var->name = prim->variable;
			var->field = NULL;
			prim->kind = SQ_PS_PVARIABLE_OLD;
			prim->variable_old = var;
		}

		if (last.kind == SQ_TK_ASSIGN && prim->kind == SQ_PS_PVARIABLE_OLD) {
			expr->kind = SQ_PS_EASSIGN;
			expr->asgn = parse_assignment(prim->variable_old);
		}
	}

end:
	// free_expression(expr);
	return expr;
}

static struct expression *parse_expression() {
	struct expression expr;
	expr.kind = SQ_PS_EMATH;

	if (!(expr.math = parse_bool_expression()))
		return NULL;

	return parse_expression_inner(true, sq_memdup(&expr, sizeof(struct expression)));
}

static struct expression *parse_expression_no_assignment() {
	struct expression expr;
	expr.kind = SQ_PS_EMATH;

	if (!(expr.math = parse_bool_expression()))
		return NULL;

	return parse_expression_inner(false, sq_memdup(&expr, sizeof(struct expression)));
}

static struct variable_old *parse_variable(void);
static struct statements *parse_brace_statements(char *);

#define MAX_KINGDOMS 255

static char *kingdoms[MAX_KINGDOMS];
static unsigned current_kingdom;

static struct kingdom_declaration *parse_kingdom_declaration() {
	GUARD(SQ_TK_KINGDOM);

	if (MAX_KINGDOMS <= current_kingdom)
		sq_throw("too many nested kingdoms");

	// this is terrible
	struct variable_old *var = parse_variable();
	struct kingdom_declaration *kingdom = sq_malloc_single(struct kingdom_declaration);
	kingdom->name = var->name;
	bool is_first = true;

	while (var->field) {
		if (!var->is_namespace_access)
			sq_throw("expected a namespace access, not %s.%s", kingdom->name, var->field);

		kingdom->name = sq_realloc(kingdom->name, strlen(kingdom->name) + strlen(var->field->name) + 3);
		strcat(kingdom->name, "::");
		strcat(kingdom->name, var->field->name);
		if (!is_first) free(var->name);
		is_first = true;
		var = var->field;
	}

	kingdoms[current_kingdom++] = kingdom->name;
	// kingdom->statements = parse_brace_statements("kingdom");
	--current_kingdom;

	return kingdom;
}

static struct scope_declaration *parse_global_declaration() {
	GUARD(SQ_TK_GLOBAL);
	struct scope_declaration *global = sq_malloc_single(struct scope_declaration);

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
	struct scope_declaration *local = sq_malloc_single(struct scope_declaration);

	if (take().kind != SQ_TK_IDENT && last.kind != SQ_TK_LABEL)
		sq_throw("expected an identifier after 'nigh'");

	local->name = last.identifier;
	local->genus = last.kind != SQ_TK_LABEL ? NULL : parse_expression_no_assignment();

	if (take().kind == SQ_TK_ASSIGN) {
		local->value = parse_expression();
	} else {
		untake();
		local->value = NULL;
	}

	return local;
}

static struct form_declaration *parse_form_declaration() {
	GUARD(SQ_TK_CLASS);
	struct form_declaration *fdecl = sq_malloc_single(struct form_declaration);
	fdecl->nparents = 0;

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		fdecl->name = last.identifier;
	} else if (last.kind == SQ_TK_LABEL) {
		fdecl->name = last.identifier;
		unsigned cap = 4;
		fdecl->parents = sq_malloc_vec(char *, cap);
		while (true) {
			if (cap == fdecl->nparents)
				fdecl->parents = sq_realloc_vec(char *, fdecl->parents, cap *= 2);

			if (take().kind == SQ_TK_IDENT) {
				fdecl->parents[fdecl->nparents++] = last.identifier;
			} else {
				untake();
				break;
			}

			if (take().kind != SQ_TK_COMMA) {
				untake();
				break;
			}
		}
	} else {
		untake();
		fdecl->name = strdup("<anonymous>");
	}

	// require a lparen.
	EXPECT(SQ_TK_LBRACE, "expected '{' before 'form' contents");

#define MAX_LEN 256 // having more than this is a god object anyways.
	fdecl->matter = sq_malloc_vec(struct matter_declaration, MAX_LEN);
	fdecl->meths = sq_malloc_vec(struct journey_declaration *, MAX_LEN);
	fdecl->funcs = sq_malloc_vec(struct journey_declaration *, MAX_LEN);
	fdecl->essences = sq_malloc_vec(struct essence_declaration, MAX_LEN);
	fdecl->constructor = NULL;
	fdecl->nmatter = 0;
	fdecl->nfuncs = 0;
	fdecl->nmeths = 0;
	fdecl->nessences = 0;

	while (take().kind != SQ_TK_RBRACE) {
		struct sq_token tkn = last;

		switch (tkn.kind) {
		case SQ_TK_METHOD:
		case SQ_TK_CLASSFN:
		case SQ_TK_CONSTRUCTOR: {
			struct journey_declaration *fn = parse_journey_declaration(false, 1, true);
			if (tkn.kind == SQ_TK_CONSTRUCTOR) {
				if (fdecl->constructor != NULL)
					sq_throw("cannot have two constructors.");
				fdecl->constructor = fn;
			} else if (tkn.kind == SQ_TK_METHOD) {
				if (fdecl->nmeths >= MAX_LEN)
					sq_throw("too many methods!");
				fdecl->meths[fdecl->nmeths++] = fn;
			} else if (tkn.kind == SQ_TK_CLASSFN) {
				if (fdecl->nfuncs >= MAX_LEN)
					sq_throw("too many form methods!");
				fdecl->funcs[fdecl->nfuncs++] = fn;
			} else {
				sq_throw("[bug] its not a constructor, func, or formfn?");
			}

			break;
		}

		case SQ_TK_ESSENCE:
			while (take().kind == SQ_TK_IDENT || last.kind == SQ_TK_LABEL) {
				if (fdecl->nessences > MAX_LEN)
					sq_throw("too many essences!");

				fdecl->essences[fdecl->nessences++].name = last.identifier;
				fdecl->essences[fdecl->nessences - 1].value = NULL;
				fdecl->essences[fdecl->nessences - 1].genus =
					(last.kind == SQ_TK_IDENT) ? NULL : parse_expression_no_assignment();

				if (take().kind == SQ_TK_COMMA)
					continue;

				if (last.kind != SQ_TK_ASSIGN) {
					untake();
					break;
				}

				fdecl->essences[fdecl->nessences - 1].value = parse_expression();

				if (take().kind != SQ_TK_COMMA) {
					untake();
					break;
				}
			}

			break;

		case SQ_TK_FIELD: {
			while (true) {
				if (take().kind != SQ_TK_IDENT && last.kind != SQ_TK_LABEL) {
					untake();
					break;
				}

				if (fdecl->nmatter > MAX_LEN)
					sq_throw("too many fields!");

				fdecl->matter[fdecl->nmatter].name = last.identifier;
				fdecl->matter[fdecl->nmatter].genus =
					(last.kind == SQ_TK_IDENT) ? NULL : parse_expression_no_assignment();
				++fdecl->nmatter;

				if (take().kind != SQ_TK_COMMA) {
					untake();
					break;
				}
			}

			break;
		}

		case SQ_TK_ENDL:
		case SQ_TK_SOFT_ENDL:
			continue;

		default:
			sq_throw("unknown token encountered when parsing 'form'.");
		}
	}

	untake();

#undef MAX_LEN

	fdecl->matter = sq_realloc_vec(struct matter_declaration, fdecl->matter, fdecl->nmatter);
	fdecl->meths = sq_realloc_vec(struct journey_declaration *, fdecl->meths, fdecl->nmeths);
	fdecl->funcs = sq_realloc_vec(struct journey_declaration *, fdecl->funcs, fdecl->nfuncs);

	EXPECT(SQ_TK_RBRACE, "expected '}' after 'form' body");

	return fdecl;
}

static struct statements *parse_statements(void);

static struct statements *parse_brace_statements(char *what) {
	struct statements *stmts;
	EXPECT(SQ_TK_LBRACE, "missing '{' for '%s' body", what);
	if (!(stmts = parse_statements()))
		sq_throw("missing body for '%s'", what);

	EXPECT(SQ_TK_RBRACE, "missing '}' for '%s' body", what);
	return stmts;
}

static void parse_journey_pattern(bool is_method, struct journey_pattern *jp) {
	struct journey_argument *current;

	if (is_method) 
		jp->pargv[jp->pargc++].name = strdup("soul"); // other two values are NULL b/c of calloc.

	enum {
		STAGE_POSITIONAL,
		STAGE_DEFAULT,
		STAGE_KW_ONLY,
	} stage = STAGE_POSITIONAL;

	// we assume the name has already been parsed.
	EXPECT(SQ_TK_LPAREN, "expected '(' for pattern");

	while (true) {
		switch (take().kind) {
		case SQ_TK_RPAREN:
			goto done_with_arguments;

		case SQ_TK_MUL:
			if (stage == STAGE_KW_ONLY) {
				sq_throw("duplicate splat argument encountered");
			} else if (take().kind == SQ_TK_COMMA || last.kind == SQ_TK_RPAREN) {
				sq_assert_n(jp->splat);
				jp->splat = strdup(""); // make it empty, so it still registers, but isn't accessible
				untake();
			} else if (last.kind != SQ_TK_IDENT) {
				sq_throw("expected name (or nothing) after '*'");
			} else {
				sq_assert_n(jp->splat);
				jp->splat = last.identifier;
			}

			stage = STAGE_KW_ONLY;
			break;

		case SQ_TK_POW:
			if (take().kind == SQ_TK_COMMA || last.kind == SQ_TK_RPAREN) {
				untake();
				jp->splatsplat = strdup(""); // make it empty, so it still registers, but isn't accessible
			} else if (last.kind == SQ_TK_IDENT) {
				sq_assert_n(jp->splatsplat);
				jp->splatsplat = last.identifier;
			} else {
				sq_throw("expected name after '**'");
			}

			if (take().kind != SQ_TK_COMMA) untake(); // allow trailing comma
			if (take().kind != SQ_TK_RPAREN) sq_throw("missing closing paren");
			goto done_with_arguments;

		case SQ_TK_IDENT:
		case SQ_TK_LABEL:
			// are we a keyword argument or a normal positional one?
			if (stage == STAGE_KW_ONLY) {
				if (jp->kwargc == SQ_JOURNEY_MAX_ARGC)
					sq_throw("too many keyword arguments!");
				current = &jp->kwargv[jp->kwargc++];
			} else {
				if (jp->pargc == SQ_JOURNEY_MAX_ARGC)
					sq_throw("too many positional arguments!");
				current = &jp->pargv[jp->pargc++];
			}

			// set the name of the argument
			current->name = last.identifier;

			// if we were a label (ie had a colon after it), parse a genus.
			if (last.kind == SQ_TK_LABEL && !(current->genus = parse_expression_no_assignment()))
				sq_throw("missing genus for argument '%s'", current->name);

			// if the next symbol's an `=`, then parse the default value.
			if (take().kind == SQ_TK_ASSIGN) {
				if (!(current->default_ = parse_expression()))
					sq_throw("missing default for argument '%s'", current->name);
				stage = STAGE_DEFAULT;
			} else if (stage == STAGE_DEFAULT) {
				sq_throw("positional parameter after default ones");
			} else {
				untake();
			}

			break;

		default:
			sq_throw("unexpected token encountered when parsing arguments: 0x%x", last.kind);
		}

		// the next symbol after an argument either be a comma or a rparen
		switch (take().kind) {
		case SQ_TK_COMMA:
			break;

		case SQ_TK_RPAREN:
			goto done_with_arguments;

		default:
			sq_throw("unknown token within variable declaration list: %d", last.kind);
		}
	}

done_with_arguments:

	if (take().kind == SQ_TK_COLON)  {
		_sq_do_not_allow_space_if_in_identifiers = true;
		if (!(jp->return_genus = parse_expression_no_assignment()))
			sq_throw("unable to parse return genus");
		_sq_do_not_allow_space_if_in_identifiers = false;
	} else {
		untake();
	}

	if (take().kind == SQ_TK_IF) {
		if (!(jp->condition = parse_expression())) sq_throw("unable to parse if condition");
	} else {
		untake();
	}

	// shorthand notation
	if (take().kind == SQ_TK_ARROW) {
		jp->body = sq_malloc_single(struct statements);
		jp->body->len = 1;
		jp->body->stmts = sq_malloc_single(struct statement *);
		jp->body->stmts[0] = sq_malloc_single(struct statement);
		jp->body->stmts[0]->kind = SQ_PS_SRETURN;
		jp->body->stmts[0]->rstmt = sq_malloc_single(struct return_statement);

		if ((jp->body->stmts[0]->rstmt->value = parse_expression()))
			return;
	} else if (untake(), (jp->body = parse_brace_statements("journey"))) {
		return;
	}

	sq_throw("no body given for function");
}

static struct journey_declaration *parse_journey_declaration(bool guard, bool is_method, bool multiple_patterns) {
	if (guard)
		GUARD(SQ_TK_FUNC);

	struct journey_declaration *jd = sq_mallocz(struct journey_declaration);

	// optional name
	if (take().kind == SQ_TK_LPAREN) {
		untake();
		jd->name = strdup("<anonymous>");
	} else if (!(jd->name = token_to_identifier(last))) {
		sq_throw("unexpected token in func declaration list");
	}

	jd->npatterns = 0;

	do {
		if (SQ_JOURNEY_MAX_PATTERNS <= jd->npatterns)
			sq_throw("too many patterns encountered");

		parse_journey_pattern(is_method, &jd->patterns[jd->npatterns++]);

		if (!multiple_patterns) return jd;
	} while (take().kind == SQ_TK_COMMA);
	untake(); // to undo the last take.

	return jd;
}

static struct if_statement *parse_if_statement() {
	GUARD(SQ_TK_IF);
	struct if_statement *if_stmt = sq_malloc_single(struct if_statement);
	if (!(if_stmt->cond = parse_expression()))
		sq_throw("missing condition for 'if'");

	if_stmt->iftrue = parse_brace_statements("if");

	if (take().kind == SQ_TK_ELSE) {
		take();
		untake();
		if (last.kind == SQ_TK_IF) {
			if_stmt->iffalse = sq_malloc_single(struct statements);
			if_stmt->iffalse->len = 1;
			if_stmt->iffalse->stmts = sq_malloc_vec(struct statement *, 2);
			if_stmt->iffalse->stmts[0] = sq_malloc_single(struct statement);
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

static struct switch_statement *parse_switch_statement() {
	GUARD(SQ_TK_SWITCH);
	struct switch_statement *sw_stmt = sq_malloc_single(struct switch_statement);
	sw_stmt->alas = NULL;
	sw_stmt->ncases = 0;
	unsigned capacity = 8;
	sw_stmt->cases = sq_malloc_vec(struct case_statement, capacity);

	if (!(sw_stmt->cond = parse_expression()))
		sq_throw("missing condition for 'fork'");

	EXPECT(SQ_TK_LBRACE, "expected a '{' after fork condition");

	while (true) {
		switch (take().kind) {
		case SQ_TK_ELSE:
			EXPECT(SQ_TK_COLON, "expected a ':' after alas");
			if (sw_stmt->alas) sq_throw("cannot declare 'alas' case twice.");
			sw_stmt->alas = parse_statements();
			break;

		case SQ_TK_CASE:
			if (sw_stmt->ncases == capacity)
				sw_stmt->cases = sq_realloc_vec(struct case_statement, sw_stmt->cases, capacity *= 2);

			if (!(sw_stmt->cases[sw_stmt->ncases].expr = parse_expression()))
				sq_throw("unable to parse condition for case");

				EXPECT(SQ_TK_COLON, "expected a ':' after path description");

			if (!(sw_stmt->cases[sw_stmt->ncases].body = parse_statements())->len) {
				free(sw_stmt->cases[sw_stmt->ncases].body);
				sw_stmt->cases[sw_stmt->ncases].body = NULL;
			}

			if (take().kind == SQ_TK_REJOIN) {
				EXPECT(SQ_TK_ENDL, "expected ';' after rejoin");
				sw_stmt->cases[sw_stmt->ncases].fallthru = true;
			} else {
				untake();
				sw_stmt->cases[sw_stmt->ncases].fallthru = false;
			}

			sw_stmt->ncases++;
			break;

		case SQ_TK_RBRACE:
			return sw_stmt;

		default:
			sq_throw("unexpected token; expecting `path` or `}` after fork body");
		}
	}
}

static struct while_statement *parse_while_statement() {
	GUARD(SQ_TK_WHILE);
	struct while_statement *while_stmt = sq_malloc_single(struct while_statement);
	if (!(while_stmt->cond = parse_expression()))
		sq_throw("missing condition for 'whilst'");

	while_stmt->body = parse_brace_statements("whilst");
	return while_stmt;
}

static struct return_statement *parse_return_statement() {
	GUARD(SQ_TK_RETURN);
	struct return_statement *ret_stmt = sq_malloc_single(struct return_statement);

	ret_stmt->value = parse_expression();

	return ret_stmt;
}

static struct expression *parse_throw_statement() {
	GUARD(SQ_TK_THROW);
	struct expression *expression = parse_expression();
	if (!expression) sq_throw("expected expression after 'catapult'");
	return expression;
}

static struct trycatch_statement *parse_trycatch_statement() {
	GUARD(SQ_TK_TRY);

	struct trycatch_statement *tc = sq_malloc_single(struct trycatch_statement);
	tc->try = parse_brace_statements("attempt");
	EXPECT(SQ_TK_ELSE, "expected 'alas' after 'attempt'");
	EXPECT(SQ_TK_IDENT, "expected an identifier after 'alas'");
	tc->exception = last.identifier;
	tc->catch = parse_brace_statements("alas");

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
		sq_throw("expecting an identifier");

	return last.identifier;
}

static char *parse_thence_declaration() {
	GUARD(SQ_TK_THENCE);

	if (take().kind != SQ_TK_IDENT)
		sq_throw("expecting an identifier");

	return last.identifier;
}

static struct statement *parse_statement() {
	struct statement stmt;

	while (take().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL) {}
	untake();

	if ((stmt.kdecl = parse_kingdom_declaration())) stmt.kind = SQ_PS_SKINGDOM;
	else if ((stmt.gdecl = parse_global_declaration())) stmt.kind = SQ_PS_SGLOBAL;
	else if ((stmt.ldecl = parse_local_declaration())) stmt.kind = SQ_PS_SLOCAL;
	else if ((stmt.label = parse_label_declaration())) stmt.kind = SQ_PS_SLABEL;
	else if ((stmt.comefrom = parse_comefrom_declaration())) stmt.kind = SQ_PS_SCOMEFROM;
	else if ((stmt.thence = parse_thence_declaration())) stmt.kind = SQ_PS_STHENCE;
	else if ((stmt.cdecl = parse_form_declaration())) stmt.kind = SQ_PS_SCLASS;
	else if ((stmt.jdecl = parse_journey_declaration(true, false, true))) stmt.kind = SQ_PS_SJOURNEY;
	else if ((stmt.ifstmt = parse_if_statement())) stmt.kind = SQ_PS_SIF;
	else if ((stmt.sw_stmt = parse_switch_statement())) stmt.kind = SQ_PS_SSWITCH;
	else if ((stmt.wstmt = parse_while_statement())) stmt.kind = SQ_PS_SWHILE;
	else if ((stmt.rstmt = parse_return_statement())) stmt.kind = SQ_PS_SRETURN;
	else if ((stmt.tcstmt = parse_trycatch_statement())) stmt.kind = SQ_PS_STRYCATCH;
	else if ((stmt.throwstmt = parse_throw_statement())) stmt.kind = SQ_PS_STHROW;
	else if ((stmt.expr = parse_expression())) stmt.kind = SQ_PS_SEXPR;
	else return NULL;

	return sq_memdup(&stmt, sizeof(struct statement));
}

static struct statements *parse_statements() {
	unsigned cap = 256, len=0;
	struct statement **list = sq_malloc_vec(struct statement *, cap);

	bool endl = true;
	while ((list[len] = parse_statement())) {
		// if (!endl && list[len-1]->kind == SQ_PS_SEXPR) sq_throw("missing `;` between statements");
		if (++len == cap - 1)
			list = sq_realloc_vec(struct statement *, list, cap*=2);

		endl = false;
		while (take_endline().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL)
			endl = true;
		untake(); // as the while statement broke it.
	}
	(void) endl;

	while (take().kind == SQ_TK_ENDL || last.kind == SQ_TK_SOFT_ENDL) {
		// do nothing
	}
	untake();

	struct statements *stmts = sq_malloc_single(struct statements);
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

