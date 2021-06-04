#ifndef SQ_PARSE_H
#define SQ_PARSE_H

struct statements *sq_parse_statements(const char *stream);

struct statements {
	unsigned len;
	struct statement **stmts;
};

struct statement {
	enum {
		SQ_PS_SCLASS,
		SQ_PS_SIMPORT,
		SQ_PS_SFUNC,
		SQ_PS_SGLOBAL,
		SQ_PS_SLOCAL,
		SQ_PS_SIF,
		SQ_PS_SWHILE,
		SQ_PS_SLABEL,
		SQ_PS_SCOMEFROM,
		SQ_PS_SRETURN,
		SQ_PS_SEXPR,
	} kind;

	union {
		struct scope_declaration *gdecl, *ldecl;
		char *import;
		struct class_declaration *cdecl;
		struct func_declaration *fdecl;
		struct if_statement *ifstmt;
		struct while_statement *wstmt;
		struct return_statement *rstmt;
		char *label, *comefrom;
		struct expression *expr;
	};
};

struct scope_declaration {
	char *name;
	struct expression *value; // can be null
};

struct class_declaration {
	char *name;
	unsigned nfields, nfuncs, nmeths;
	char **fields;
	struct func_declaration **funcs, **meths, *constructor;
};

struct func_declaration {
	char *name;
	unsigned nargs;
	char **args;
	struct statements *body;
};

struct if_statement {
	struct expression *cond;
	struct statements *iftrue;
	struct statements *iffalse;
};

struct while_statement {
	struct expression *cond;
	struct statements *body;
};

struct return_statement {
	struct expression *value;
};

struct expression {
	enum {
		SQ_PS_EFNCALL,
		SQ_PS_EASSIGN,
		SQ_PS_EARRAY_ASSIGN,
		SQ_PS_EMATH
	} kind;

	union {
		struct function_call *fncall;
		struct assignment *asgn;
		struct array_assignment *ary_asgn;
		struct bool_expression *math;
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

struct array_assignment {
	struct variable *var;
	struct expression *index;
	struct expression *value;
};

struct variable {
	char *name;
	struct variable *field;
};

struct bool_expression {
	enum { SQ_PS_BEQL, SQ_PS_BAND, SQ_PS_BOR } kind;
	struct eql_expression *lhs;
	struct bool_expression *rhs;
};

struct eql_expression {
	enum { SQ_PS_ECMP, SQ_PS_EEQL, SQ_PS_ENEQ } kind;
	struct cmp_expression *lhs;
	struct eql_expression *rhs;
};

struct cmp_expression {
	enum { SQ_PS_CADD, SQ_PS_CLTH, SQ_PS_CLEQ, SQ_PS_CGTH, SQ_PS_CGEQ } kind;
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

struct array {
	unsigned nargs;
	struct expression **args;
};

struct array_index {
	struct variable *array;
	struct expression *index;
};

struct primary {
	enum {
		SQ_PS_PPAREN,
		SQ_PS_PLAMBDA,
		SQ_PS_PNUMBER,
		SQ_PS_PSTRING,
		SQ_PS_PBOOLEAN,
		SQ_PS_PNULL,
		SQ_PS_PVARIABLE,
		SQ_PS_PARRAY,
		SQ_PS_PARRAY_INDEX,
	} kind;
	union {
		struct expression *expr;
		struct func_declaration *lambda;
		sq_number number;
		struct sq_string *string;
		bool boolean;
		struct variable *variable;
		struct array *array;
		struct array_index *array_index;
	};
};

#endif /* !SQ_PARSE_H */
