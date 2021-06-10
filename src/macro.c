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

#define foo
#ifndef SQ_MACRO_INCLUDE
struct _ignored__;
#else
#include <string.h>


static struct {
	struct macro_variable {
		char *name, **args;
		struct sq_token *tokens;
		unsigned tokenlen, arglen;
		bool is_function;
	} *vars;
	unsigned len, cap;
} variables;

#define MAX_EXPANSIONS 256
#define MAX_ARGLEN 256

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

static void parse_henceforth_literal(struct macro_variable *var) {
	unsigned cap, len;
	struct sq_token *tokens;

	len = 0;
	tokens = xmalloc(sizeof(struct sq_token[cap = 8]));

	is_in_macro_declaration = true;
	bool is_verbatim = false;

	while (true) {
		switch ((tokens[len++] = sq_next_token()).kind) {
		case SQ_TK_LTH:
			if (*sq_stream != '<') break;
			++sq_stream;
			--len;
			is_verbatim = true;
			continue;

		case SQ_TK_GTH:
			if (*sq_stream != '>') break;
			++sq_stream;
			--len;
			is_verbatim = false;
			continue;

		case SQ_TK_ENDL:
			if (is_verbatim) break;
			--len;
			goto done;

		case SQ_TK_UNDEFINED:
			die("unterminated macro");

		default:
			break;
		}

		if (len == cap)
			tokens = xrealloc(tokens, sizeof(struct sq_token[cap *= 2]));
	}

done:
	is_in_macro_declaration = false;

	var->tokens = xrealloc(tokens, sizeof(struct sq_token[len]));
	var->tokenlen = len;
	var->args = NULL;
	var->arglen = 0;
	var->is_function = false;
}

static void parse_henceforth_function(struct macro_variable *var) {
	char **args = xmalloc(sizeof(char *[MAX_ARGLEN]));
	unsigned arglen = 0;

	char c;
	while (true) {
		strip_whitespace(true);
		if ((c = *sq_stream++) == ')') break;
		if (c != '$') die("expected '$' or ')'");
		if (arglen == MAX_ARGLEN) die("too many arguments");

		args[arglen++] = parse_identifier().identifier;
		strip_whitespace(true);
		if (*sq_stream == ',') ++sq_stream;
	}

	if (sq_next_token().kind != SQ_TK_ASSIGN)
		die("expected '=' after macro function declaration");

	parse_henceforth_literal(var);
	var->args = xrealloc(args, sizeof(char *[arglen]));
	var->arglen = arglen;
	var->is_function = true;
}

static void parse_henceforth(void) {
	struct macro_variable *var;
	strip_whitespace(true);

	if (*sq_stream++ != '$')
		die("expected a macro identifier");

	char *name = parse_identifier().identifier;
	for (unsigned i = 0; i < variables.len; ++i)
		if (!strcmp((var = &variables.vars[i])->name, name)) {
			free(name);
			free(var->tokens);
			goto found_token;
		}

	if (variables.len == variables.cap) {
		variables.vars = variables.len
			? xrealloc(variables.vars, sizeof(struct macro_variable[variables.cap *= 2]))
			: xmalloc(sizeof(struct macro_variable[variables.cap = 8]));
	}

	(var = &variables.vars[variables.len++])->name = name;

found_token:;

	struct sq_token token = next_normal_token();

	if (token.kind == SQ_TK_ASSIGN) {
		parse_henceforth_literal(var);
	} else if (token.kind == SQ_TK_LPAREN) {
		parse_henceforth_function(var);
	} else {
		die("unknown token after @henceforth");
	}
}


static void parse_macro_statement(char *name) {
	if (!strcmp(name, "henceforth")) {
		free(name);
		parse_henceforth();
	} else {
		die("unknown macro statement kind '%s'", name);
	}
}

static void	parse_macro_identifier_invocation(struct expansion *exp, struct macro_variable *var) {
	if (sq_next_token().kind != SQ_TK_LPAREN)
		die("expected '(' after macro function '%s'", var->name);

	struct {
		struct sq_token *args;
		unsigned len;
	} args[var->arglen];
	struct sq_token *arg, token;
	unsigned cap, len, paren_depth;

	// parse arguments
	for (unsigned i = 0; i < var->arglen; ++i) {
		len = 0;
		cap = 8;
		paren_depth = 0;
		arg = xmalloc(sizeof(struct sq_token [cap]));
		bool is_verbatim = false;

		while (true) {
			switch ((token = sq_next_token()).kind) {
			case SQ_TK_LTH:
				if (*sq_stream != '<') break;
				++sq_stream;
				is_verbatim = true;
				continue;

			case SQ_TK_GTH:
				if (*sq_stream != '>') break;
				++sq_stream;
				is_verbatim = false;
				continue;

			case SQ_TK_LPAREN:
				if (!is_verbatim) paren_depth++;
				break;

			case SQ_TK_RPAREN:
				if (!is_verbatim && !paren_depth--) {
					if (i != var->arglen - 1)
						die("unexpected `)`; too few arguments");
					goto next_arg;
				}
				break;

			case SQ_TK_COMMA:
				if (is_verbatim) break;
				if (!paren_depth) goto next_arg;

			default:
				;
			}

			arg[len++] = token;
			if (len == cap)
				arg = xrealloc(arg, sizeof(struct sq_token [cap *= 2]));
		}

	next_arg:

		args[i].args = xrealloc(arg, sizeof(struct sq_token [cap]));
		args[i].len = len;
	}

	if (!var->arglen && sq_next_token().kind != SQ_TK_RPAREN)
		die("missing closing ')'");

	// expand them out and make the resulting array
	len = 0;
	cap = var->tokenlen;
	struct sq_token *tokens = xmalloc(sizeof(struct sq_token [cap]));

	for (unsigned i = 0; i < var->tokenlen; ++i) {
		if (cap == len)
			tokens = xrealloc(tokens, sizeof(struct sq_token [cap *= 2]));

		if (var->tokens[i].kind != SQ_TK_MACRO_VAR) {
			tokens[len++] = var->tokens[i]; // todo: dup it?
			continue;
		}

		for (unsigned j = 0; j < var->arglen; ++j) {
			if (strcmp(var->args[j], var->tokens[i].identifier))
				continue;

			for (unsigned k = 0; k < args[j].len; ++k) {
				if (cap == len) tokens = xrealloc(tokens, sizeof(struct sq_token [cap *= 2]));
				tokens[len++] = args[j].args[k];
			}

			goto next_argument;
		}

		tokens[len++] = var->tokens[i];
	next_argument:
		;
		// assert(!is_in_macro_declaration);

		// expansions[++expansion_pos].len = var->tokenlen;
		// expansions[expansion_pos].pos = i;
		// expansions[expansion_pos].tokens = var->tokens;

		// unsigned pos = expansion_pos;
		// do {
		// 	if (cap == len) tokens = xrealloc(tokens, sizeof(struct sq_token [cap *= 2]));
		// 	tokens[len++] = sq_next_token();
		// } while (pos < expansion_pos);

		// // ie we're done with the entire macro parsing
		// if (expansion_pos < pos) break;

		// assert(pos == expansion_pos);
		// i += expansions[expansion_pos--].pos;
		// i--; // will wraparound but that's ok, as we add one.
	}

	exp->tokens = xrealloc(tokens, sizeof(struct sq_token [len]));
	exp->len = len;
}

static bool parse_macro_identifier(char *name) {
	if (is_in_macro_declaration)
		return true;

	struct macro_variable *var;

	for (unsigned i = 0; i < variables.len; ++i)
		if (!strcmp(name, (var = &variables.vars[i])->name))
			goto found;

	die("unknown macro identifier '$%s'", name);

found:
	// free(name);
	if (MAX_EXPANSIONS < expansion_pos)
		die("too many expansions!");

	struct expansion exp;

	// if it's a normal identifier, just return its expansion
	if (!var->is_function) {
		exp.tokens = var->tokens;
		exp.len = var->tokenlen;
	} else {
		parse_macro_identifier_invocation(&exp, var);
	}

 	expansions[++expansion_pos].tokens = exp.tokens;
 	expansions[expansion_pos].len = exp.len;
 	expansions[expansion_pos].pos = 0;

	return false;
}

#endif /* SQ_MACRO_INCLUDE */
