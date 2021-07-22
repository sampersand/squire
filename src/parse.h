#ifndef SQ_PARSE_H
#define SQ_PARSE_H

#include "journey.h"

struct statements *sq_parse_statements(const char *stream);

struct statements {
	unsigned len;
	struct statement **stmts;
};

struct statement {
	enum {
		SQ_PS_SKINGDOM,
		SQ_PS_SCLASS,
		SQ_PS_SJOURNEY,
		SQ_PS_STRYCATCH,
		SQ_PS_STHROW,
		SQ_PS_SRETURN,

		SQ_PS_SGLOBAL,
		SQ_PS_SLOCAL,

		SQ_PS_SIF,
		SQ_PS_SWHILE,
		SQ_PS_SLABEL,
		SQ_PS_SCOMEFROM,
		SQ_PS_SSWITCH,

		SQ_PS_SEXPR,
	} kind;

	union {
		struct kingdom_declaration *kdecl;
		struct scope_declaration *gdecl, *ldecl;
		struct class_declaration *cdecl;
		struct journey_declaration *jdecl;
		struct if_statement *ifstmt;
		struct while_statement *wstmt;
		struct return_statement *rstmt;
		struct trycatch_statement *tcstmt;
		struct switch_statement *sw_stmt;
		struct expression *throwstmt;
		char *label, *comefrom;
		struct expression *expr;
	};
};

struct kingdom_declaration {
	char *name;
	struct statement *statements;
};

struct scope_declaration {
	char *name;
	struct expression *value; // can be null
};

struct class_declaration {
	char *name;
	unsigned nmatter, nfuncs, nmeths, nparents, nessences;
	char **parents;

	struct journey_declaration **funcs, **meths, *constructor;

	struct matter_declaration {
		char *name;
		struct primary *genus; // may be NULL
	} *matter;

	struct essence_declaration {
		char *name;
		struct expression *value;
		struct primary *genus; // may be NULL
	} *essences;
};

struct journey_argument {
	char *name;
	struct primary *genus; // may be NULL
	struct expression *default_; // may be NULL.
};

#define SQ_JOURNEY_MAX_PATTERNS 255
struct journey_declaration {
	char *name;
	unsigned npatterns;
	struct journey_pattern {
		unsigned pargc, kwargc;
		struct journey_argument pargv[SQ_JOURNEY_MAX_ARGC], kwargv[SQ_JOURNEY_MAX_ARGC];

		char *splat, *splatsplat; // both maybe NULL
		struct primary *return_genus; // may be NULL
		struct statements *body;
		struct expression *condition; // may be NULL
	} patterns[SQ_JOURNEY_MAX_PATTERNS];
};

struct if_statement {
	struct expression *cond;
	struct statements *iftrue;
	struct statements *iffalse; // may be NULL
};

struct switch_statement {
	struct expression *cond;
	struct statements *alas; // may be NULL

	struct case_statement {
		struct expression *expr;
		struct statements *body;
	} *cases;
	unsigned ncases;
};

struct while_statement {
	struct expression *cond;
	struct statements *body;
};

struct return_statement {
	struct expression *value;
};

struct trycatch_statement {
	struct statements *try, *catch;
	char *exception;
};

struct expression {
	enum {
		SQ_PS_EFNCALL,
		SQ_PS_EASSIGN,
		SQ_PS_EARRAY_ASSIGN,
		SQ_PS_EMATH,
		SQ_PS_EINDEX,
	} kind;

	union {
		struct function_call *fncall;
		struct assignment *asgn;
		struct index_assign *ary_asgn;
		struct bool_expression *math;
		struct index *index;
	};
};

struct function_call {
	struct variable *func;
	unsigned arglen;
	struct expression **args;
};

struct assignment {
	struct variable *var;
	struct expression *expr;
};

struct index_assign {
	struct primary *into;
	struct expression *index;
	struct expression *value;
};

struct index {
	struct primary *into;
	struct expression *index;
};

struct variable {
	char *name;
	struct variable *field;
	bool is_namespace_access; // currently assigned to, but ignored for namespaces.
};

struct bool_expression {
	enum { SQ_PS_BEQL, SQ_PS_BAND, SQ_PS_BOR } kind;
	struct eql_expression *lhs;
	struct bool_expression *rhs; // may be NULL.
};

struct eql_expression {
	enum { SQ_PS_ECMP, SQ_PS_EEQL, SQ_PS_ENEQ, SQ_PS_EMATCHES } kind;
	struct cmp_expression *lhs;
	struct eql_expression *rhs; // may be NULL.
};

struct cmp_expression {
	enum { SQ_PS_CADD, SQ_PS_CLTH, SQ_PS_CLEQ, SQ_PS_CGTH, SQ_PS_CGEQ, SQ_PS_CCMP } kind;
	struct add_expression *lhs;
	struct cmp_expression *rhs; // may be NULL.
};

struct add_expression {
	enum { SQ_PS_AMUL, SQ_PS_AADD, SQ_PS_ASUB } kind;
	struct mul_expression *lhs;
	struct add_expression *rhs; // may be NULL.
};

struct mul_expression {
	enum { SQ_PS_MPOW, SQ_PS_MMUL, SQ_PS_MDIV, SQ_PS_MMOD } kind;
	struct pow_expression *lhs;
	struct mul_expression *rhs; // may be NULL.
};

struct pow_expression {
	enum { SQ_PS_PUNARY, SQ_PS_PPOW } kind;
	struct unary_expression *lhs;
	struct pow_expression *rhs; // may be NULL.
};

struct unary_expression {
	enum { SQ_PS_UPRIMARY, SQ_PS_UNEG, SQ_PS_UNOT } kind;
	struct primary *rhs;
};

struct book {
	unsigned npages;
	struct expression **pages;
};

struct dict {
	unsigned neles;
	struct expression **keys, **vals;
};

struct primary {
	enum {
		SQ_PS_PPAREN,
		SQ_PS_PLAMBDA,
		SQ_PS_PNUMERAL,
		SQ_PS_PTEXT,
		SQ_PS_PVERACITY,
		SQ_PS_PNI,
		SQ_PS_PVARIABLE,
		SQ_PS_PBOOK,
		SQ_PS_PCODEX,
		SQ_PS_PINDEX,
	} kind;
	union {
		struct expression *expr;
		struct journey_declaration *lambda;
		sq_numeral numeral;
		struct sq_text *text;
		sq_veracity veracity;
		struct variable *variable;
		struct book *book;
		struct dict *dict;
		struct index *index;
	};
};

#endif /* !SQ_PARSE_H */
