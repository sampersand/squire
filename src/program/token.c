#include <squire/token.h>
#include <squire/shared.h>

#include <ctype.h>
#include <string.h>
#include <stdbool.h>

const char *sq_stream;
static char put_back_quote;

static struct sq_token next_macro_token(void);
static void parse_macro_statement(char *);
static bool parse_macro_identifier(char *);

static size_t fraktur_length(const char *stream, size_t *index) {
	static const char *const FRAKTUR[26 * 2] = {
		// A	B	C	D	E	F	G	H	I
		  "𝔄", "𝔅", "ℭ", "𝔇", "𝔈", "𝔉", "𝔊", "ℌ", "ℑ",
		// J	K	L	M	N	O	P	Q	R
		  "𝔍", "𝔎", "𝔏", "𝔐", "𝔑", "𝔒", "𝔓", "𝔔", "ℜ",
		// S	T	U	V	W	X	Y	Z
		  "𝔖", "𝔗", "𝔘", "𝔙", "𝔚", "𝔛", "𝔜", "ℨ",

		// a	b	c	d	e	f	g	h	i
		  "𝔞", "𝔟", "𝔠", "𝔡", "𝔢", "𝔣", "𝔤", "𝔥", "𝔦",
		// j	k	l	m	n	o	p	q	r
		  "𝔧", "𝔨", "𝔩", "𝔪", "𝔫", "𝔬", "𝔭", "𝔮", "𝔯",
		// s	t	u	v	w	x	y	z
		  "𝔰", "𝔱", "𝔲", "𝔳", "𝔴", "𝔵", "𝔶", "𝔷"
	};

	for (size_t i = 0; i < 26 * 2; ++i) {
		size_t len = strlen(FRAKTUR[i]);
		if (!strncmp(stream, FRAKTUR[i], len)) {
			*index = i;
			return len;
		}
	}
	return 0;
}

static struct sq_text *parse_fraktur_bareword(void) {
	size_t fraktur_len, fraktur_pos;

	if (!(fraktur_len = fraktur_length(sq_stream, &fraktur_pos)))
		return NULL;

	char *fraktur = xmalloc(16);
	unsigned cap = 16, len = 0;

	do {
		if (cap == len)
			fraktur = xrealloc(fraktur, cap *= 2);

		if ((fraktur_len = fraktur_length(sq_stream, &fraktur_pos))) {
			sq_stream += fraktur_len;
			fraktur[len++] = (fraktur_pos < 26) ? ('A' + fraktur_pos) : ('a' + (fraktur_pos - 26));
			continue;
		}

		// note the rhs of the `++` will always be true.
		if (isspace(*sq_stream) || (*sq_stream == '\\' && ++sq_stream)) {
			fraktur[len++] = *(sq_stream++);
			continue;
		}

		break;
	} while (*sq_stream != '\0');

	while (isspace(fraktur[len - 1]))
		--len;

	fraktur[len] = '\0';
	return sq_text_new2(fraktur, len);
}

static bool strip_whitespace_maybe_ignore_slash(bool ignore_slash) <%
	(void) ignore_slash; // todo: i forgot why i used this
	char c;
	bool had_a_newline = false;

	// strip whitespace
	while ((c = *sq_stream)) {
		if (c == '#' || !strncmp(sq_stream, "N.B. ", 5)) { // single line
			do {
				c = *++sq_stream;
			} while (c && c != '\n');
		} else if (c == '/' && sq_stream[1] == '*') { // multiline
			sq_stream += 2;

			while (true) {
				if (sq_stream[0] == '*' && sq_stream[1] == '/') {
					sq_stream += 2;
					break;
				}

				if (!*sq_stream++) die("unterminated block comment");
			}
		}
#if 0
		else if (*sq_stream == '\\') { // i forget why this is here
			++sq_stream;

			if (*sq_stream && *sq_stream++ != '\n' && !ignore_slash) {
				sq_stream -= 2;
				continue;
				// die("unexpected '\\' on its own.");
			}
			continue;
		}
#endif
		else if (!isspace(c)) break; // nonspace = exit out
		else for (; isspace(c); c = *++sq_stream) { // space
			// note that this implementation won't throw an exception on spaces on the first line.
			// but oh well whatever.
			if (c == '\n') had_a_newline = true;
			else if (c == ' ' && had_a_newline)
				die("THOU SHALT NOT INDENT WITH SPACES!");
		}
	}

	return had_a_newline;
%>

static bool strip_whitespace(void) {
	return strip_whitespace_maybe_ignore_slash(false);
}

#define CHECK_FOR_START(str, tkn) \
	if (!strncmp(str, sq_stream, strlen(str))) {\
		sq_stream += strlen(str); token.kind = tkn; return token; \
	}

#define CHECK_FOR_START_KW(str, tkn) \
	if (!strncmp(str, sq_stream, strlen(str)) \
		&& !isalnum(*(sq_stream + strlen(str))) && *(sq_stream + strlen(str)) != '_') {\
		sq_stream += strlen(str); token.kind = tkn; return token; }


static unsigned tohex(char c) {
	if (isdigit(c)) return c - '0';
	if ('a' <= c && c <= 'f') return c - 'a' + 10;
	if ('A' <= c && c <= 'F') return c - 'A' + 10;
	die("char '%1$c' (\\x%1$02x) isn't a hex digit", c);
}

static struct sq_token parse_arabic_numeral(void) {
	struct sq_token token;
	token.kind = SQ_TK_NUMERAL;
	token.numeral = 0;

	do {
		token.numeral = token.numeral * 10 + (*sq_stream - '0');
	} while (isdigit(*++sq_stream));

	if (isalpha(*sq_stream) || *sq_stream == '_')
		die("invalid trailing characters on arabic numeral literal: %llu%c\n",
			(long long) token.numeral, *sq_stream);

	return token;
}

static struct sq_token next_normal_token(void);

#define MAX_INTERPOLATIONS 256
static struct { int stage; unsigned depth; char quote; } interpolations[MAX_INTERPOLATIONS];
static unsigned interpolation_length;
static bool _interpolate_is_curly_brace;

static struct sq_token next_interpolation_token(void) {
	struct sq_token token;

	switch (interpolations[interpolation_length].stage++){
	case -1: // `-1` is what's set to after we do the final `)`.
		put_back_quote = interpolations[interpolation_length--].quote;

	case 0:
		token.kind = SQ_TK_ADD;
		return token;

	case 1:
		token.kind = SQ_TK_IDENT;
		token.identifier = strdup("text");
		return token;

	case 2:
		token.kind = SQ_TK_LPAREN;
		return token;
	}

	token = next_normal_token();

	// yeah, i think ahead.
	if ((token.kind == SQ_TK_LPAREN && !_interpolate_is_curly_brace)
			|| (_interpolate_is_curly_brace && token.kind == SQ_TK_LBRACE)
	) {
		token.kind = SQ_TK_LPAREN;
		++interpolations[interpolation_length].depth;
	} else if ((token.kind == SQ_TK_RPAREN && !_interpolate_is_curly_brace)
		|| (_interpolate_is_curly_brace && token.kind == SQ_TK_RBRACE)
	) {
		if (!--interpolations[interpolation_length].depth)
			interpolations[interpolation_length].stage = -1;
		token.kind = SQ_TK_RPAREN;
	}

	return token;
}
// whelp, `"foo \(bar)!" * 3` will not expand properly, so we need to have
// every text return a leading `(` first...; but that's too hard rn.

static struct sq_token parse_text(void) {
	unsigned length = 0;
	char *dst = xmalloc(strlen(sq_stream));
	char quote, c;

	if (put_back_quote)
		quote = put_back_quote, put_back_quote = '\0';
	else
		quote = *sq_stream++;

	while ((c = *sq_stream++) != quote) {
	top:
		if (!c)
			die("unterminated quote encountered");

		if (c == '{') {
			_interpolate_is_curly_brace = true;
			goto interpolate;
		}

		if (c != '\\') {
			dst[length++] = c;
			continue;
		}

		if (quote == '\'') {
			switch (c = *sq_stream++) {
				case '\\':
				case '\'':
				case '\"':
				case '{':
					break;
				default:
					dst[length++] = '\\';
			}

			dst[length++] = c;
			continue;
		}

		switch (c = *sq_stream++) {
		case '\\':
		case '\'': 
		case '\"':
			break;

		case '\n':
			goto top;

		case '(':
		interpolate:
			if (MAX_INTERPOLATIONS < interpolation_length)
				die("too many interpolations");

			interpolations[interpolation_length + 1].depth = 1;
			interpolations[interpolation_length + 1].quote = quote;
			interpolations[++interpolation_length].stage = 0;

			goto done;

		case 'n': c = '\n'; break;
		case 't': c = '\t'; break;
		case 'f': c = '\f'; break;
		case 'v': c = '\v'; break;
		case 'r': c = '\r'; break;

		case 'x':
			if (sq_stream[0] == quote || sq_stream[0] == '\0' || sq_stream[1] == quote)
				die("unterminated escape sequence");

			c = tohex(sq_stream[0]) * 16 + tohex(sq_stream[1]);
			sq_stream += 2;
			break;
		}

		dst[length++] = c;
	}

done:

	dst[length] = '\0';

	struct sq_token token;
	token.kind = SQ_TK_TEXT;
	token.text = sq_text_new2(dst, length);

	return token;
}

bool _sq_do_not_allow_space_if_in_identifiers;
static struct sq_token parse_identifier(void) {
	struct sq_token token;
	token.kind = SQ_TK_IDENT;
	unsigned len = 0, cap = 16;

	token.identifier = xmalloc(cap);

	while (true) {
		if (len == cap)
			token.identifier = xrealloc(token.identifier, cap *= 2);

		if (isupper(*sq_stream) && len && !isupper(token.identifier[0])) {
			token.identifier[len++] = '_';
			token.identifier[len++] = *sq_stream++ - 'A' + 'a';
		} else if (isalnum(*sq_stream) || *sq_stream == '_') {
			token.identifier[len++] = *sq_stream++;
		} else if (*sq_stream == '-' && (isalpha(sq_stream[1]) || sq_stream[1] == '_')) {
			++sq_stream;
			token.identifier[len++] = '_';
		} else if (*sq_stream == ' ' && strncmp(sq_stream, " N.B. ", 6) && (
			_sq_do_not_allow_space_if_in_identifiers && strncmp(sq_stream, " if", 3))) {
			while (*++sq_stream == ' ' || sq_stream[-1] == '\t');
			if (isalnum(*sq_stream) || *sq_stream == '_') token.identifier[len++] = '_';
			else break;
		} else break;
	}

	token.identifier = xrealloc(token.identifier, sizeof_array(char , len + 1));
	token.identifier[len] = '\0';

	// check to see if we're a label
	while (isspace(*sq_stream) || *sq_stream == '#')
		if (*sq_stream == '#')
			while (*sq_stream && *sq_stream++ != '\n');
		else
			++sq_stream;

	if (*sq_stream == ':' && sq_stream[1] != ':')
		++sq_stream, token.kind = SQ_TK_LABEL;

	return token;
}


struct sq_token next_non_macro_token() {
	return interpolation_length ? next_interpolation_token() : next_normal_token();
}

// scope creep go brr
struct sq_token sq_next_token() {
	struct sq_token token = next_macro_token();

	if (token.kind != SQ_TK_UNDEFINED)
		return token;
	return next_non_macro_token();
}

static struct sq_token next_normal_token(void) {
	struct sq_token token;

	if (put_back_quote) return parse_text();

	if (strip_whitespace()) return token.kind = SQ_TK_SOFT_ENDL, token;

	//printf("<<%s>>\n", sq_stream);
	if (!*sq_stream || !strncmp(sq_stream, "@__END__", 8)) {
		//printf("here?\n");
		return token.kind = SQ_TK_UNDEFINED, token;
	}

	if (isdigit(*sq_stream))
		return parse_arabic_numeral();

	if (sq_numeral_starts(sq_stream)) {
		token.numeral = sq_roman_to_numeral(sq_stream, &sq_stream);
		if (token.numeral >= 0)
			return (token.kind = SQ_TK_NUMERAL), token;
	}

	struct sq_text *fraktur;
	if ((fraktur = parse_fraktur_bareword()) != NULL) {
		token.kind = SQ_TK_TEXT;
		token.text = fraktur;
		return token;
	}

	if (*sq_stream == '\'' || *sq_stream == '\"')
		return parse_text();

	if (*sq_stream == '@') {
		++sq_stream;
		parse_macro_statement(parse_identifier().identifier);
		return sq_next_token();
	} else if (*sq_stream == '$') {
		++sq_stream;
		if (parse_macro_identifier(token.identifier = parse_identifier().identifier))
			return token.kind = SQ_TK_MACRO_VAR, token;
		return sq_next_token();
	}

	CHECK_FOR_START_KW("form",         SQ_TK_CLASS);
	CHECK_FOR_START_KW("matter",       SQ_TK_FIELD);
	CHECK_FOR_START_KW("change",       SQ_TK_METHOD);
	CHECK_FOR_START_KW("recollect",    SQ_TK_CLASSFN); // deprecated
	CHECK_FOR_START_KW("recall",       SQ_TK_CLASSFN);
	CHECK_FOR_START_KW("imitate",      SQ_TK_CONSTRUCTOR);
	CHECK_FOR_START_KW("essence",      SQ_TK_ESSENCE);
	CHECK_FOR_START_KW("cite",         SQ_TK_CITE);
	// substance?
	// essence is static  variable

	if (*sq_stream == '\\')
		return ++sq_stream, token.kind = SQ_TK_LAMBDA, token;
	CHECK_FOR_START_KW("journey",      SQ_TK_FUNC);
	CHECK_FOR_START_KW("renowned",     SQ_TK_GLOBAL);
	CHECK_FOR_START_KW("nigh",         SQ_TK_LOCAL);

	CHECK_FOR_START_KW("if",           SQ_TK_IF); // _should_ we have a better one?
	CHECK_FOR_START_KW("alas",         SQ_TK_ELSE);
	CHECK_FOR_START_KW("whence",       SQ_TK_COMEFROM);
	CHECK_FOR_START_KW("thence",       SQ_TK_THENCE);
	CHECK_FOR_START_KW("whilst",       SQ_TK_WHILE);
	CHECK_FOR_START_KW("reward",       SQ_TK_RETURN);
	CHECK_FOR_START_KW("attempt",      SQ_TK_TRY);
	CHECK_FOR_START_KW("catapult",     SQ_TK_THROW);
	CHECK_FOR_START_KW("fork",         SQ_TK_SWITCH);
	CHECK_FOR_START_KW("rejoin",       SQ_TK_REJOIN);
	CHECK_FOR_START_KW("path",         SQ_TK_CASE);
	CHECK_FOR_START_KW("kingdom",      SQ_TK_KINGDOM);

	CHECK_FOR_START_KW("yea",          SQ_TK_YAY);
	CHECK_FOR_START_KW("nay",          SQ_TK_NAY);
	CHECK_FOR_START_KW("ni",           SQ_TK_NI);

	if (isalpha(*sq_stream) || *sq_stream == '_')
		return parse_identifier();

	CHECK_FOR_START("[]", SQ_TK_INDEX);
	CHECK_FOR_START("[]=", SQ_TK_INDEX_ASSIGN);
	CHECK_FOR_START("{", SQ_TK_LBRACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("(", SQ_TK_LPAREN);
	CHECK_FOR_START(")", SQ_TK_RPAREN);
	CHECK_FOR_START("[", SQ_TK_LBRACKET);
	CHECK_FOR_START("]", SQ_TK_RBRACKET);
	CHECK_FOR_START(";", SQ_TK_ENDL);
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDL);
	CHECK_FOR_START(",", SQ_TK_COMMA);
	CHECK_FOR_START(".", SQ_TK_DOT);
	CHECK_FOR_START("::", SQ_TK_COLONCOLON);
	CHECK_FOR_START(":", SQ_TK_COLON);

	CHECK_FOR_START("^=", SQ_TK_POW_ASSIGN);
	CHECK_FOR_START("*=", SQ_TK_MUL_ASSIGN);
	CHECK_FOR_START("+=", SQ_TK_ADD_ASSIGN);
	CHECK_FOR_START("-=", SQ_TK_SUB_ASSIGN);
	CHECK_FOR_START("/=", SQ_TK_DIV_ASSIGN);
	CHECK_FOR_START("%=", SQ_TK_MOD_ASSIGN);

	CHECK_FOR_START("~~", SQ_TK_MATCHES);
	CHECK_FOR_START("<=>", SQ_TK_CMP);
	CHECK_FOR_START("==", SQ_TK_EQL);
	CHECK_FOR_START("!=", SQ_TK_NEQ);
	CHECK_FOR_START("<=", SQ_TK_LEQ);
	CHECK_FOR_START(">=", SQ_TK_GEQ);
	CHECK_FOR_START("=>", SQ_TK_ARROW);
	CHECK_FOR_START("^", SQ_TK_POW);
	CHECK_FOR_START("<", SQ_TK_LTH);
	CHECK_FOR_START(">", SQ_TK_GTH);
	CHECK_FOR_START("+", SQ_TK_ADD);
	CHECK_FOR_START("-@", SQ_TK_NEG);
	CHECK_FOR_START("-", SQ_TK_SUB);
	CHECK_FOR_START("*", SQ_TK_MUL);
	CHECK_FOR_START("/", SQ_TK_DIV);
	CHECK_FOR_START("%", SQ_TK_MOD);
	CHECK_FOR_START("!", SQ_TK_NOT);
	CHECK_FOR_START("~", SQ_TK_PAT_NOT);
	CHECK_FOR_START("&&", SQ_TK_AND);
	CHECK_FOR_START("||", SQ_TK_OR);
	CHECK_FOR_START("&", SQ_TK_PAT_AND);
	CHECK_FOR_START("|", SQ_TK_PAT_OR);
	CHECK_FOR_START("=", SQ_TK_ASSIGN);

	die("unknown token start '%c'", *sq_stream);
}


void sq_token_dump(const struct sq_token *token) {
	// this isn't exactly correct anymore...
	switch (token->kind) {
	case SQ_TK_UNDEFINED: printf("Keyword(undefined)"); break;
	case SQ_TK_CLASS: printf("Keyword(class)"); break;
	case SQ_TK_FUNC: printf("Keyword(func)"); break;
	case SQ_TK_GLOBAL: printf("Keyword(global)"); break;
	case SQ_TK_IF: printf("Keyword(if)"); break;
	case SQ_TK_ELSE: printf("Keyword(else)"); break;
	case SQ_TK_RETURN: printf("Keyword(return)"); break;
	case SQ_TK_YAY: printf("Keyword(true)"); break;
	case SQ_TK_NAY: printf("Keyword(false)"); break;
	case SQ_TK_NI: printf("Keyword(ni)"); break;

	case SQ_TK_IDENT: printf("Ident(%s)", token->identifier); break;
	case SQ_TK_NUMERAL: printf("Numeral(%lld)", (long long) token->numeral); break;
	case SQ_TK_TEXT: printf("Text(%s)", token->text->ptr); break;

	case SQ_TK_LBRACE: printf("Punct({)"); break;
	case SQ_TK_RBRACE: printf("Punct(})"); break;
	case SQ_TK_LPAREN: printf("Punct(()"); break;
	case SQ_TK_RPAREN: printf("Punct())"); break;
	case SQ_TK_LBRACKET: printf("Punct([)"); break;
	case SQ_TK_RBRACKET: printf("Punct(])"); break;
	case SQ_TK_ENDL: printf("Punct(;)"); break;
	case SQ_TK_SOFT_ENDL: printf("Punct(\\n)"); break;
	case SQ_TK_COMMA: printf("Punct(,)"); break;
	case SQ_TK_DOT: printf("Punct(.)"); break;

	case SQ_TK_EQL: printf("Operator(==)"); break;
	case SQ_TK_NEQ: printf("Operator(!=)"); break;
	case SQ_TK_ADD: printf("Operator(+)"); break;
	case SQ_TK_SUB: printf("Operator(-)"); break;
	case SQ_TK_MUL: printf("Operator(*)"); break;
	case SQ_TK_DIV: printf("Operator(/)"); break;
	case SQ_TK_MOD: printf("Operator(%%)"); break;
	case SQ_TK_NOT: printf("Operator(!)"); break;
	case SQ_TK_PAT_NOT: printf("Operator(~)"); break;
	case SQ_TK_PAT_AND: printf("Operator(&)"); break;
	case SQ_TK_PAT_OR: printf("Operator(|)"); break;
	case SQ_TK_AND: printf("Operator(&&)"); break;
	case SQ_TK_OR: printf("Operator(||)"); break;
	case SQ_TK_ASSIGN: printf("Operator(=)"); break;

	default: printf("Unknown(%d)", token->kind); break;
	}
}


#define SQ_MACRO_INCLUDE
#include "macro.c"
