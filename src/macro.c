#ifndef SQ_MACRO_INCLUDE
struct _ignored__;
#else
#include "macros.h"
#include <string.h>

struct macro_variable {
	char *name;
	struct sq_token *tokens;
	unsigned tokenlen;
};

static struct {
	struct macro_variable *vars;
	unsigned len, cap;
} variables;

#define MAX_EXPANSIONS 256
static unsigned expansion_pos;
static struct expansion {
	struct sq_token *tokens;
	unsigned len, pos;
} expansions[MAX_EXPANSIONS];

const struct sq_token UNDEFINED = { .kind = SQ_TK_UNDEFINED };

struct sq_token next_macro_token(void) {
	struct sq_token token;
	token.kind = SQ_TK_UNDEFINED;


	if (!expansion_pos)
		return token;

	struct expansion *exp = &expansions[expansion_pos];

	if (exp->len == exp->pos) {
		if (--expansion_pos) return next_macro_token();
		return token;
	}

	return exp->tokens[exp->pos++];
}

static void parse_henceforth_literal(unsigned i) {
	unsigned cap, len;
	struct sq_token *tokens;

	len = 0;
	tokens = xmalloc(sizeof(struct sq_token[cap = 8]));

	while (true) {
		switch ((tokens[len++] = sq_next_token()).kind) {
		case SQ_TK_ENDL:
		case SQ_TK_SOFT_ENDL:
			--len;
		case SQ_TK_UNDEFINED:
			goto done;
		default: break;
		}

		if (len == cap)
			tokens = xrealloc(tokens, sizeof(struct sq_token[cap *= 2]));
	}

done:

	variables.vars[i].tokens = xrealloc(tokens, sizeof(struct sq_token[len]));
	variables.vars[i].tokenlen = len;
}

static void parse_henceforth_function(unsigned i) {
	(void) i;
}


static void parse_henceforth(void) {
	unsigned i;
	strip_whitespace(true);

	if (*sq_stream++ != '$')
		die("expected a macro identifier");

	char *name = parse_identifier().identifier;
	for (i = 0; i < variables.len; ++i)
		if (!strcmp(variables.vars[i].name, name)) {
			free(name);
			free(variables.vars[i].tokens);
			goto found_token;
		}

	if (variables.len == variables.cap) {
		variables.vars = variables.len
			? xrealloc(variables.vars, sizeof(struct macro_variable[variables.cap *= 2]))
			: xmalloc(sizeof(struct macro_variable[variables.cap = 8]));
	}

	variables.vars[variables.len++].name = name;

found_token:;

	struct sq_token token = sq_next_token_nointerpolate();
	if (token.kind == SQ_TK_ASSIGN)
		parse_henceforth_literal(i);
	else
		parse_henceforth_function(i);
}


void parse_macro_statement(char *name) {
	if (!strcmp(name, "henceforth")) free(name), parse_henceforth();
	else die("unknown macro statement start '%s'", name);
}

void parse_macro_identifier(char *name) {
	for (unsigned i = 0; i < variables.len; ++i) {
		if (!strcmp(name, variables.vars[i].name)) {
			free(name);
			expansions[++expansion_pos].tokens = variables.vars[i].tokens;
			expansions[expansion_pos].len = variables.vars[i].tokenlen;
			expansions[expansion_pos].pos = 0;
			if (MAX_EXPANSIONS < expansion_pos)
				die("too many expansions!");
			return;
		}
	}

	die("unknown macro identifier '$%s'", name);
}

#endif /* SQ_MACRO_INCLUDE */
