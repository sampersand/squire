#ifndef SQ_PARSE_H
#define SQ_PARSE_H

#include <squire/journey.h>

struct statements *sq_parse_statements(const char *stream);
/*
struct sq_ps_statements {
	unsigned len;
	struct statement **stmts;
};

struct sq_ps_statement {
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
		struct form_declaration *cdecl;
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
	unsigned nsubjects;
	struct {
		enum {
			SQ_PS_K_FORM,
			SQ_PS_K_STATEMENT,
			SQ_PS_K_JOURNEY,
			SQ_PS_K_RENOWNED,
			SQ_PS_K_KINGDOM,
		} kind;

		union {
			struct form_declaration *form_decl;	
			struct statement *statement;
			struct journey *journey;
			struct scope_declaration *renowned;
			struct kingdom_declaration *kingdom;
		};
	} *subjects;
};

struct scope_declaration {
	char *name;
	struct expression *value; // can be null
};

struct form_declaration {
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
		struct function_call_old *fncall;
		struct assignment *asgn;
		struct index_assign *ary_asgn;
		struct bool_expression *math;
		struct index *index;
	};
};

struct function_call_old {
	struct identifier *func;
	unsigned arglen;
	struct expression **args;
};

struct assignment {
	struct identifier *var;
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

struct identifier {
	char *name;
	struct identifier *field;
	bool is_namespace_access; // currently assigned to, but ignored for namespaces.
};
*/

struct sq_ps_assign {
	enum {
		SQ_PS_ANORMAL,
		SQ_PS_AATTR,
		SQ_PS_AINDEX,
	} kind;

	struct sq_ps_expression *base; // undefined when `SQ_PS_ANORMAL`.
	char *variable;
};

struct sq_ps_if {
	unsigned count; // there's always at least one---that's the base condition.

	struct sq_ps_if_branch {
		struct sq_ps_expression *condition;
		struct sq_ps_statements *body;
	} *branches;

	struct sq_ps_statements *alas; // is `NULL` if there's no default.
};

struct sq_ps_whilst {
	struct sq_ps_expression *condition;
	struct sq_ps_statements *body;
	struct sq_ps_expression *alas; // it's run if the condition initially turns out to be false.
};

struct sq_ps_fork {
	struct sq_ps_expression *condition;
	unsigned count;

	struct sq_ps_fork_path {
		unsigned cond_counts; // how many conditions there are present
		struct sq_ps_expression **conditions;
		struct sq_ps_statement *body;
	} *paths;

	struct sq_ps_statments *alas; // is `NULL` if there's no default.
};

struct sq_ps_variable_decl {
	char *name;
	struct sq_ps_pattern *pattern;
};

struct sq_ps_besiege {
	struct sq_ps_statements *body;
	unsigned nbranches;
	struct sq_ps_besiege_branch {
		char *error_name;
		struct sq_ps_statements *alas; // `catch`. can be NULL (ie only indubitably is used)
	} *branches;
	struct sq_ps_statements *indubitably; // `finally`. can be NULL.
};

struct sq_ps_expression {
	enum {
		SQ_PS_ECOMPOUND,
		SQ_PS_EASSIGN,
		SQ_PS_EIF,
		SQ_PS_EWHILST,
		SQ_PS_EFORK,
		SQ_PS_EWHENCE,
		SQ_PS_EREWARD,
		SQ_PS_ECATAPULT,
		SQ_PS_EBESIEGE,
	} kind;

	union {
		struct sq_ps_compound *compound;
		struct sq_ps_assign *assign;
		struct sq_ps_if *if_;
		struct sq_ps_whilst *whilst;
		struct sq_ps_fork *fork;
		char *whence;
		struct sq_ps_expression *reward; // is `NULL` if there's no return expr (ie default).
		struct sq_ps_expression *catapult; // is `NULL` if the current expr should be thrown.
		struct sq_ps_besiege *besiege;
	};
};

struct sq_ps_fn_call {
	struct sq_ps_expression *function;
	unsigned arglen;
	// todo: keyword arguments.
	struct sq_ps_expression **args;
};

struct sq_ps_compound {
	enum {
		/* not compound, just a primary. */
		SQ_PS_CPRIMARY,

		/* unaries */
		SQ_PS_CNEG = 0x10,
		SQ_PS_CPOS,
		SQ_PS_CNOT,

		/* binary */
		// math
		SQ_PS_CADD,
		SQ_PS_CSUB,
		SQ_PS_CMUL,
		SQ_PS_CDIV,
		SQ_PS_CMOD,
		SQ_PS_CPOW,

		// logic
		SQ_PS_CEQL,
		SQ_PS_CNEQ,
		SQ_PS_CLTH,
		SQ_PS_CLEQ,
		SQ_PS_CGTH,
		SQ_PS_CGEQ,
		SQ_PS_CCMP,

		// short circuit
		SQ_PS_CAND,
		SQ_PS_COR,
		SQ_PS_CTERNARY,

		// misc
		SQ_PS_CMATCHES,
		SQ_PS_CINDEX,
		SQ_PS_CFN_CALL
	} kind;

	union {
		struct sq_ps_compound *args[3];
		struct sq_ps_fn_call *fncall;
	};
};

struct sq_ps_book {
	unsigned len;
	struct sq_ps_expression **pages;
};

struct sq_ps_codex {
	unsigned len;
	struct sq_ps_expression **keys, **vals;
};

struct sq_ps_get_attr {
	struct sq_ps_expression *expr;
	char *identifier;
};

struct sq_ps_primary {
	enum {
		SQ_PS_PNUMERAL,
		SQ_PS_PVERACITY,
		SQ_PS_PTEXT,
		SQ_PS_PIDENTIFIER,
		SQ_PS_PNI,

		SQ_PS_PBOOK,
		SQ_PS_PCODEX,
		SQ_PS_PEXPRESSION,
		SQ_PS_PLAMBDA,
		SQ_PS_PGET_ATTR,
	} kind;

	union {
		// literals
		sq_numeral numeral;
		sq_veracity veracity;
		struct sq_text *text;
		char *identifier;
		// `ni` is only done thru `kind`

		// compound
		struct sq_ps_book *book;
		struct sq_ps_codex *codex;

		// misc
		struct sq_ps_expression *expr;
		struct sq_ps_journey *lambda;
		struct sq_ps_get_attr *get_attr;
	};
};

#endif /* !SQ_PARSE_H */
