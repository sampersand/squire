/*
# Macros in Squire. the `$` sigil is used to denote macro parameters, and
# the `@` sigil is used to denote the start of a preprocessor directive.
# Note that because im lazy, they'll be parsed at the same time as normal code, and
# so you can actually reference variables in the surrounding scope.

# Simple declarations and functional macros. They simply take the next line, like c.
@henceforth $ab = "ab" # note `$` is required
@henceforth $add(a, b) $a + $bs
proclaim($add(2, 3)); # => same as `proclaim(2 + 3)`.

ab = "ab"
# Conditional compilation
@if $ab == ab # you can just use arbitrary expressions here, including referencing surroudning scope
    @henceforth $life = 42
@alasif $ab == "bc"
    @henceforth $life = 43
@alas
    @henceforth $foo = "bar"
@end

# Looping. requires the use of arrays.
@foreach i in [$ab, "cd", "ef"]
    proclaim($i + "\n");
@end

# Interpret a value as a token stream
@henceforth $x = "proclaim("
@henceforth $y = "'hello, world!\n'"
@henceforth $z = ")"
@explicate $x + $y + $z # => proclaim('hello, world!\n')]
# ^-- maybe fix?
*/

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

bool is_in_macro_declaration;

static struct sq_token next_macro_token(void) {
	struct sq_token token;
	token.kind = SQ_TK_UNDEFINED;


	if (!expansion_pos)
		return token;

	struct expansion *exp = &expansions[expansion_pos];

	if (exp->len == exp->pos) {
		if (--expansion_pos) return next_macro_token();
		return token;
	}

	if ((token = exp->tokens[exp->pos++]).kind == SQ_TK_MACRO_VAR) {
		assert(!is_in_macro_declaration);
		parse_macro_identifier(token.identifier);
		return next_macro_token();
	}

	return token;
}

static void parse_henceforth_literal(unsigned i) {
	unsigned cap, len;
	struct sq_token *tokens;

	len = 0;
	tokens = xmalloc(sizeof(struct sq_token[cap = 8]));

	is_in_macro_declaration = true;
	while (true) {
		switch ((tokens[len++] = sq_next_token()).kind) {
		case SQ_TK_ENDL:
		case SQ_TK_SOFT_ENDL:
			--len;
		case SQ_TK_UNDEFINED:
			goto done;
		default:
			break;
		}

		if (len == cap)
			tokens = xrealloc(tokens, sizeof(struct sq_token[cap *= 2]));
	}

done:
	is_in_macro_declaration = false;

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
	else if (token.kind == SQ_TK_LPAREN)
		parse_henceforth_function(i);
	else die("unknown token after @henceforth");
}


static void parse_macro_statement(char *name) {
	if (!strcmp(name, "henceforth")) free(name), parse_henceforth();
	else die("unknown macro statement kind '%s'", name);
}

static bool parse_macro_identifier(char *name) {
	if (is_in_macro_declaration)
		return true;

	for (unsigned i = 0; i < variables.len; ++i) {
		if (!strcmp(name, variables.vars[i].name)) {
			free(name);
			expansions[++expansion_pos].tokens = variables.vars[i].tokens;
			expansions[expansion_pos].len = variables.vars[i].tokenlen;
			expansions[expansion_pos].pos = 0;
			if (MAX_EXPANSIONS < expansion_pos)
				die("too many expansions!");
			return false;
		}
	}

	die("unknown macro identifier '$%s'", name);
}

#endif /* SQ_MACRO_INCLUDE */
