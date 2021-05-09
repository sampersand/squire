#include "token.h"
#include "program.h"
#include "function.h"
#include "shared.h"
#include "struct.h"
#include <string.h>

struct sq_program *program;
const char *stream;
struct sq_token last = { .kind = SQ_TK_UNDEFINED };
bool rewound;

#define MAX_ARGC 255
#define MAX_FIELDS 256

struct sq_program *sq_program_parse(const char *stream_) {
	stream = stream_;
	program = xmalloc(sizeof(struct sq_program));
	program->nglobals = 0;
	program->globals = NULL;
	program->nfuncs = 0;
	program->funcs = NULL;

	// todo

	return program;
}

static void untake() {
	assert(!rewound);
	rewound = true;
}

static struct sq_token take_endline() {
	if (rewound) {
		rewound = false;
	} else {
		last = sq_next_token(&stream);
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


struct sq_struct *parse_struct() {
	GUARD(SQ_TK_STRUCT);

	char *name;

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		name = last.identifier;
	} else {
		untake();
		name = strdup("<anonymous>");
	}

	// require a lparen.
	EXPECT(SQ_TK_LBRACE, "expected '{' before struct fields");
	parse_field_names();
	EXPECT(SQ_TK_RBRACE, "expected '}' after struct fields");

	char **fields = memdup(field_names, sizeof(char *[field_name_count]));

	return sq_struct_new(name, field_name_count, fields);
}

static void parse_function_body(struct sq_function *fn);

struct sq_function *parse_func() {
	// 'struct' prefix.
	GUARD(SQ_TK_FUNC);

	char *name;

	// optional name
	if (take().kind == SQ_TK_IDENT) {
		name = last.identifier;
	} else {
		untake();
		name = strdup("<anonymous>");
	}

	EXPECT(SQ_TK_LPAREN, "expected '(' before func args");
	parse_field_names();
	EXPECT(SQ_TK_RPAREN, "expected ')' after func args");

	char **args = memdup(field_names, sizeof(char *[field_name_count]));

	struct sq_function *fn = xmalloc(sizeof(struct sq_function));
	fn->name = name;
	fn->refcount = 1;
	fn->argc = field_name_count;
	fn->nlocals = 0;
	fn->nconsts = 0;
	fn->program = NULL;
	fn->consts = NULL;
	fn->code = NULL;

	// parse_function_body(fn);

	return fn;
}

#if 0
struct statements {
	unsigned stmtlen, stmtcap;
	struct statement *stmts;
};

struct program {
	struct statements stmts;
};

struct statement {
	enum {
		SQ_PS_SSTRUCT,
		SQ_PS_SFUNC,
		SQ_PS_SIF,
		SQ_PS_SWHILE,
		SQ_PS_SRETURN,
		SQ_PS_SEXPR,
	} kind;
	union {
		struct struct_declaration *sdecl;
		struct func_declaration *fdecl;
		struct if_statement *ifstmt;
		struct while_statement *wstmt;
		struct return_statement *rstmt;
		struct expression *expr;
	};
};

struct struct_declaration {
	char *name;
	unsigned fieldlen;
	char **fields;
};

struct func_declaration {
	char *name;
	unsigned arglen;
	char **args;
	struct statements *body;
};

struct if_statement {
	struct expression *cond;
	struct statements *ifbody;
	struct if_statement *elseif;
	struct statements *elsebody;
};

struct while_statement {
	struct expression *cond;
	struct statements *body;
};

struct return_statement {
	struct expression *value;
	bool has_value;
};

struct expression {
	enum {
		SQ_PS_EFNCALL,
		SQ_PS_EASSIGN,
		SQ_PS_EMATH,
	} kind;

	union {
		struct function_call *fncall;
		struct assignment *asgn;
		struct eql_expression *math;
	};
};

struct function_call {
	struct variable *func;
	unsigned arglen;
	struct expression *args;
};

struct assignment {
	struct variable *var;
	struct expression *expr;
};

struct variable {
	char *name;
	struct variable *field;
};

struct eql_expression {
	enum { SQ_PS_ECMP, SQ_PS_EEQL, SQ_PS_ENEQ } kind;
	struct cmp_expression *lhs;
	struct eql_expression *rhs;
};

struct cmp_expression {
	enum { SQ_PS_CADD, SQ_PS_CLTH, SQ_PS_CGTH } kind;
	struct add_expression *lhs;
	struct cmp_expression *rhs;
};

struct add_expression {
	enum { SQ_PS_AMUL, SQ_PS_AADD, SQ_PS_ASUB } kind;
	struct mul_expression *lhs;
	struct add_expression *rhs;
};

struct mul_expression {
	enum { SQ_PS_MUNARY, SQ_PS_MMUL, SQ_PS_MDIV, SQ_PS_MMOD } kind;
	struct unary_expression *lhs;
	struct mul_expression *rhs;
};

struct unary_expression {
	enum { SQ_PS_UPRIMARY, SQ_PS_UNEG, SQ_PS_UNOT } kind;
	struct primary *rhs;
};

struct primary {
	enum {
		SQ_PS_PPAREN,
		SQ_PS_PNUMBER,
		SQ_PS_PSTRING,
		SQ_PS_PBOOLEAN,
		SQ_PS_PNULL,
		SQ_PS_PVARIABLE,
	} kind;
	union {
		struct expression *expr;
		sq_number number;
		struct sq_string *string;
		bool boolean;
		struct variable *variable;
	};
};
#endif

struct expression *parse_expression(void);

struct variable *parse_variable(void) {
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

struct primary *parse_primary() {
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
	case SQ_TK_IDENT:
		untake();
		primary.kind = SQ_PS_PVARIABLE;
		primary.variable = parse_variable();
		// todo: function call here?
		break;
	default:
		untake();
		return NULL;
	}

	return memdup(&primary, sizeof(struct primary));
}

struct unary_expression *parse_unary_expression() {
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
};

struct mul_expression *parse_mul_expression() {
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
};

struct add_expression *parse_add_expression() {
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
};

struct cmp_expression *parse_cmp_expression() {
	struct cmp_expression cmp;

	if (!(cmp.lhs = parse_add_expression()))
		return NULL;

	switch (take().kind) {
	case SQ_TK_LTH:
		cmp.kind = SQ_PS_CLTH;
		break;
	case SQ_TK_GTH:
		cmp.kind = SQ_PS_CGTH;
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
};

struct eql_expression *parse_eql_expression() {
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
};

struct function_call *parse_func_call(struct variable *func) {
	GUARD(SQ_TK_LPAREN);

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
	fncall->args = memdup(&args, sizeof(struct expression *[arg_count]));
	fncall->arglen = arg_count;

	return fncall;
}

struct assignment *parse_assignment(struct variable *var) {
	GUARD(SQ_TK_ASSIGN);

	struct assignment *asgn = xmalloc(sizeof(struct assignment));
	asgn->var = var;
	if (!(asgn->expr = parse_expression()))
		die("missing rhs for assignment");
	return asgn;
}

struct expression *parse_expression() {
	struct expression expr;
	expr.kind = SQ_PS_EMATH;

	if (!(expr.math = parse_eql_expression()))
		return NULL;

	if (expr.math->kind != SQ_PS_ECMP
		|| expr.math->lhs->kind != SQ_PS_CADD
		|| expr.math->lhs->lhs->kind != SQ_PS_AMUL
		|| expr.math->lhs->lhs->lhs->kind != SQ_PS_MUNARY
		|| expr.math->lhs->lhs->lhs->lhs->kind != SQ_PS_UPRIMARY
		|| expr.math->lhs->lhs->lhs->lhs->rhs->kind != SQ_PS_PVARIABLE
	) goto done;

	struct variable *var = expr.math->lhs->lhs->lhs->lhs->rhs->variable;
	take();
	untake();

	if (last.kind == SQ_TK_ASSIGN) {
		expr.kind = SQ_PS_EASSIGN;
		expr.asgn = parse_assignment(var);
	} else if (last.kind == SQ_TK_LPAREN) {
		expr.kind = SQ_PS_EFNCALL;
		expr.fncall = parse_func_call(var);
	}

done:

	return memdup(&expr, sizeof(struct expression));
}

struct struct_declaration *parse_struct_declaration() {
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

struct statements *parse_statements(void);

struct statements *parse_brace_statements(char *what) {
	struct statements *stmts;
	EXPECT(SQ_TK_LBRACE, "missing '{' for '%s' body", what);
	if (!(stmts = parse_statements()))
		die("missing body for '%s'", what);
	EXPECT(SQ_TK_RBRACE, "missing '}' for '%s' body", what);
	return stmts;
}

struct func_declaration *parse_func_declaration() {
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

struct if_statement *parse_if_statement() {
	GUARD(SQ_TK_IF);
	struct if_statement *if_stmt = xmalloc(sizeof(struct if_statement));
	if (!(if_stmt->cond = parse_expression()))
		die("missing condition for 'if'");

	if_stmt->iftrue = parse_brace_statements("if");

	if (take().kind == SQ_TK_ELSE) {
		if_stmt->iffalse = parse_brace_statements("else");
	} else {
		if_stmt->iffalse = NULL;
	}

	return if_stmt;
}

struct while_statement *parse_while_statement() {
	GUARD(SQ_TK_WHILE);
	struct while_statement *while_stmt = xmalloc(sizeof(struct while_statement));
	if (!(while_stmt->cond = parse_expression()))
		die("missing condition for 'while'");

	while_stmt->body = parse_brace_statements("while");
	return while_stmt;
}

struct return_statement *parse_return_statement() {
	GUARD(SQ_TK_RETURN);
	struct return_statement *ret_stmt = xmalloc(sizeof(struct return_statement));

	ret_stmt->value = parse_expression();

	return ret_stmt;
}

struct statement *parse_statement() {
	struct statement stmt;
	if ((stmt.sdecl = parse_struct_declaration())) stmt.kind = SQ_PS_SSTRUCT;
	else if ((stmt.fdecl = parse_func_declaration())) stmt.kind = SQ_PS_SFUNC;
	else if ((stmt.ifstmt = parse_if_statement())) stmt.kind = SQ_PS_SIF;
	else if ((stmt.wstmt = parse_while_statement())) stmt.kind = SQ_PS_SWHILE;
	else if ((stmt.rstmt = parse_return_statement())) stmt.kind = SQ_PS_SRETURN;
	else if ((stmt.expr = parse_expression())) stmt.kind = SQ_PS_SEXPR;
	else return NULL;

	return memdup(&stmt, sizeof(struct statement));
}

struct statements *parse_statements() {
	unsigned cap = 256, len=0;
	struct statement **list = xmalloc(sizeof(struct statement *[cap]));

	bool endl = true;
	while ((list[len] = parse_statement())) {
		if (!endl) die("missing `;` between statements");
		if (++len == cap)
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

/*


struct statement {
	enum {
		SQ_PS_SSTRUCT,
		SQ_PS_SFUNC,
		SQ_PS_SIF,
		SQ_PS_SWHILE,
		SQ_PS_SRETURN,
		SQ_PS_SEXPR,
	} kind;
	union {
		struct struct_declaration *sdecl;
		struct func_declaration *fdecl;
		struct if_statement *ifstmt;
		struct while_statement *wstmt;
		struct return_statement *rstmt;
		struct expression *expr;
	};
};*/
// program := <stmts>
// stmts := { <item> <endline> } [<item> [<endline>]]
// stmt
//  := <struct-decl>
//   | <func-decl>
//   | <if-stmt>
//   | <while-stmt>
//   | <return-stmt>
//   | <expr>
// endline := ';' | LITERAL_NEWLINE (* note `\` followed by a literal newline is ok. *)

// struct-decl := 'struct' [IDENT] '{' <fields> '}'
// func-decl := 'func' [IDENT] '(' <fields> ')' <body>
// fields := {IDENT ','} [IDENT [',']]
// if-stmt := 'if' <expr> <body> {'else' 'if' <expr> <body>} ['else' <body>]
// while-stmt := 'while' <expr> <body>
// return-stmt := 'return' [<stmt>]
// body := '{' <stmts> '}'


// /*

// enum sq_token_kind {
// 	SQ_TK_UNDEFINED = 0,
// 	SQ_TK_STRUCT,
// 	SQ_TK_FUNC,
// 	SQ_TK_IF,
// 	SQ_TK_ELSE,
// 	SQ_TK_RETURN,
// 	SQ_TK_TRUE,
// 	SQ_TK_FALSE,
// 	SQ_TK_NULL,

// 	SQ_TK_IDENT,
// 	SQ_TK_NUMBER,
// 	SQ_TK_STRING,

// 	SQ_TK_LBRACE,
// 	SQ_TK_RBRACE,
// 	SQ_TK_LPAREN,
// 	SQ_TK_RPAREN,
// 	SQ_TK_LBRACKET,
// 	SQ_TK_RBRACKET,
// 	SQ_TK_ENDLINE,
// 	SQ_TK_SOFT_ENDLINE,
// 	SQ_TK_COMMA,
// 	SQ_TK_DOT,

// 	SQ_TK_EQL,
// 	SQ_TK_NEQ,
// 	SQ_TK_LTH,
// 	SQ_TK_GTH,
// 	SQ_TK_ADD,
// 	SQ_TK_SUB,
// 	SQ_TK_MUL,
// 	SQ_TK_DIV,
// 	SQ_TK_MOD,
// 	SQ_TK_NOT,
// 	SQ_TK_AND,
// 	SQ_TK_OR,
// 	SQ_TK_ASSIGN,
// };*/

