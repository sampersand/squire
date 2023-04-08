/*
# Macros in Squire. the `$` sigil is used to denote macro parameters, and
# the `@` sigil is used to denote the start of a preprocessor directive.

# Simple declarations and functional macros. They simply take the next line, like c.
@henceforth $ab = "ab" # note `$` is required
@henceforth $add(a, b) $a + $bs
proclaim($add(2, 3)); # => same as `proclaim(2 + 3)`.

ab = "ab"
# Conditional compilation
@if $ab == "ab"
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
#include <string.h>
#include <errno.h>

#ifndef NPOSIX
# include <limits.h>
# include <stdio.h>
# include <stdlib.h>
#endif

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
	tokens = sq_malloc(sq_sizeof_array(struct sq_token, cap = 8));

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
			sq_throw("unterminated macro");

		default:
			break;
		}

		if (len == cap)
			tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token, cap *= 2));
	}

done:
	is_in_macro_declaration = false;

	var->tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token, len));
	var->tokenlen = len;
	var->args = NULL;
	var->arglen = 0;
	var->is_function = false;
}

static void parse_henceforth_function(struct macro_variable *var) {
	char **args = sq_malloc(sq_sizeof_array(char *, MAX_ARGLEN));
	unsigned arglen = 0;

	char c;
	while (true) {
		strip_whitespace();
		if ((c = *sq_stream++) == ')') break;
		if (c != '$') sq_throw("expected '$' or ')'");
		if (arglen == MAX_ARGLEN) sq_throw("too many arguments");

		args[arglen++] = parse_identifier().identifier;
		strip_whitespace();
		if (*sq_stream == ',') ++sq_stream;
	}

	if (sq_next_token().kind != SQ_TK_ASSIGN)
		sq_throw("expected '=' after macro function declaration");

	parse_henceforth_literal(var);
	var->args = sq_realloc(args, sq_sizeof_array(char *, arglen));
	var->arglen = arglen;
	var->is_function = true;
}

static char *parse_macro_identifier_name(void) {
	strip_whitespace();

	if (*sq_stream++ != '$')
		sq_throw("expected a macro identifier");

	return parse_identifier().identifier;
}

static void parse_henceforth(void) {
	struct macro_variable *var;
	char *name = parse_macro_identifier_name();

	for (unsigned i = 0; i < variables.len; ++i)
		if (!strcmp((var = &variables.vars[i])->name, name)) {
			free(name);
			free(var->tokens);
			goto found_token;
		} // todo: make use of `<impossible>`s

	if (variables.len == variables.cap) {
		variables.vars = variables.len
			? sq_realloc(variables.vars, sq_sizeof_array(struct macro_variable, variables.cap *= 2))
			: sq_malloc(sq_sizeof_array(struct macro_variable, variables.cap = 8));
	}

	(var = &variables.vars[variables.len++])->name = name;

found_token:;

	struct sq_token token = next_normal_token();

	if (token.kind == SQ_TK_ASSIGN) {
		parse_henceforth_literal(var);
	} else if (token.kind == SQ_TK_LPAREN) {
		parse_henceforth_function(var);
	} else {
		sq_throw("unknown token after @henceforth");
	}
}

static void parse_nevermore(void) {
	char *name = parse_macro_identifier_name();
	for (unsigned i = 0; i < variables.len; ++i) {
		if (strcmp(variables.vars[i].name, name)) continue;
		free(name);
		variables.vars[i].name = "<impossible>"; // impossible name
		return;
	}
}


static void parse_whereupon(void) {
	char *name = parse_macro_identifier_name();

	// if we find something with the name, jsut return. a `@nowhere` won't do
	// anything.
	bool is_defined = false;

	for (unsigned i = 0; i < variables.len; ++i)
		if (!strcmp(variables.vars[i].name, name)) { is_defined = true; break; }

	unsigned len = 0, cap = 128;
	struct sq_token token, *tokens = sq_malloc(sq_sizeof_array(struct sq_token, cap));

	while (true) {
		strip_whitespace_maybe_ignore_slash(true);

		if (*sq_stream == '@') {
			//exit(0);
			if (!strncmp(sq_stream, "@nowhere", 8) && !isalpha(sq_stream[8])) {
				sq_stream += 8;
				break;
			} else if (!strncmp(sq_stream, "@alas", 5) && !isalpha(sq_stream[5])) {
				sq_stream += 5;
				is_defined = !is_defined; // i mean technically it works...
				continue;
			}
		}

		// lol we'll take `@nowhere` within strings and comments....
		if (!is_defined) {
			// strip_whitespace();
			++sq_stream;
			continue;
		}

		if ((token = sq_next_token()).kind == SQ_TK_UNDEFINED)
			sq_throw("`@nowhere` found nowhere.");

		if (!is_defined) continue;

		if (cap == len)
			tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token, cap *= 2));

		tokens[len++] = token;
	}

	expansions[++expansion_pos].len = len;
	expansions[expansion_pos].pos = 0;
	expansions[expansion_pos].tokens = tokens;
}


#ifdef NPOSIX
static bool should_compile(char *filename) {
	return true; // lol, always compile on windows or somethin. idk.
}
#else
static bool should_compile(char *filename) {
	static char **paths;
	static unsigned npaths, pathcap;

	char *path = realpath(filename, NULL);

	for (unsigned i = 0; i < npaths; ++i)
		if (!strcmp(paths[i], path))
			return free(path), false;

	if (npaths == pathcap) {
		if (!paths) paths = sq_malloc(sq_sizeof_array(char *, pathcap = 16));
		else paths = sq_realloc(paths, sq_sizeof_array(char *, pathcap *= 2));
	}

	paths[npaths++] = path;
	return true;
}
#endif

static void parse_transcribe(void) {
	strip_whitespace();
	if (*sq_stream != '\'' && *sq_stream != '\"') sq_throw("can only compile strings");
	char *filename = parse_text().text->ptr; // lol memfree?

	if (!should_compile(filename)) return;

	errno = 0;
	FILE *file = fopen(filename, "rb");
	if (errno) fprintf(stderr, "cannot open file '%s': %s", filename, strerror(errno)), exit(1);

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);
	if (errno) perror("cannot get file size"), exit(1);

	size_t stream_len = strlen(sq_stream);
	char *new_stream = sq_malloc(stream_len + file_size + 1);
	fread(new_stream, 1, file_size, file);

	if (errno) perror("cannot get read file contents"), exit(1);
	fclose(file);
	if (errno) perror("cannot get close file"), exit(1);

	// this _will_ leak memory, but eh we're compiling who cares
	memcpy(new_stream + file_size, sq_stream, stream_len + 1);
	sq_stream = new_stream;
}

static void parse_macro_statement(char *name) {
	if (!strcmp(name, "henceforth")) parse_henceforth();
	else if (!strcmp(name, "nevermore")) parse_nevermore();
	else if (!strcmp(name, "transcribe")) parse_transcribe();
	else if (!strcmp(name, "whereupon")) parse_whereupon();
	else if (!strcmp(name, "nowhere")) sq_throw("unexpected '@nowhere'");
	else if (!strcmp(name, "alas")) sq_throw("unexpected '@alas'");
	else if (!strcmp(name, "expand")) { /* parse_expand(); */ }
	else sq_throw("unknown macro statement kind '%s'", name);;

	free(name);
}

static void	parse_macro_identifier_invocation(struct expansion *exp, struct macro_variable *var) {
	if (sq_next_token().kind != SQ_TK_LPAREN)
		sq_throw("expected '(' after macro function '%s'", var->name);

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
		arg = sq_malloc(sq_sizeof_array(struct sq_token , cap));
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
						sq_throw("unexpected `)`; too few arguments");
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
				arg = sq_realloc(arg, sq_sizeof_array(struct sq_token , cap *= 2));
		}

	next_arg:

		args[i].args = sq_realloc(arg, sq_sizeof_array(struct sq_token , cap));
		args[i].len = len;
	}

	if (!var->arglen && sq_next_token().kind != SQ_TK_RPAREN)
		sq_throw("missing closing ')'");

	// expand them out and make the resulting array
	len = 0;
	cap = var->tokenlen;
	struct sq_token *tokens = sq_malloc(sq_sizeof_array(struct sq_token , cap));

	for (unsigned i = 0; i < var->tokenlen; ++i) {
		if (cap == len)
			tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token , cap *= 2));

		if (var->tokens[i].kind != SQ_TK_MACRO_VAR) {
			tokens[len++] = var->tokens[i]; // todo: dup it?
			continue;
		}

		for (unsigned j = 0; j < var->arglen; ++j) {
			if (strcmp(var->args[j], var->tokens[i].identifier))
				continue;

			for (unsigned k = 0; k < args[j].len; ++k) {
				if (cap == len) tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token , cap *= 2));
				tokens[len++] = args[j].args[k];
			}

			goto next_argument;
		}

		tokens[len++] = var->tokens[i];
	next_argument:
		;
	}

	exp->tokens = sq_realloc(tokens, sq_sizeof_array(struct sq_token , len));
	exp->len = len;
}

static bool parse_macro_identifier(char *name) {
	static unsigned long long unique_value;

	if (is_in_macro_declaration)
		return true;

	if (!strcmp(name, "__COUNTER__")) {
		expansions[++expansion_pos].tokens = sq_malloc(sizeof(struct sq_token));
		expansions[expansion_pos].tokens[0].kind = SQ_TK_NUMERAL;
		expansions[expansion_pos].tokens[0].numeral = unique_value++;
		expansions[expansion_pos].len = 1;
		expansions[expansion_pos].pos = 0;
		return false;
	}

	struct macro_variable *var;

	for (unsigned i = 0; i < variables.len; ++i)
		if (!strcmp(name, (var = &variables.vars[i])->name))
			goto found;

	sq_throw("unknown macro identifier '$%s'", name);

found:
	// free(name);
	if (MAX_EXPANSIONS < expansion_pos)
		sq_throw("too many expansions!");

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
