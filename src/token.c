#include "token.h"
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "shared.h"
#include "roman.h"

const char *sq_stream;

static void strip_whitespace() {
	char c;

	// strip whitespace
	while ((c = *sq_stream)) {
		if (c == '#') {
			do {
				c = *++sq_stream;
			} while (c && c != '\n');
			if (c == '\n') ++sq_stream;
			continue;
		}

		if (!isspace(c) || c == '\n')
			break;

		while (isspace(c) && c != '\n') {
			c = *++sq_stream;
		}
	}
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
	if ('a' <= c && c <= 'f') return c - 'a';
	if ('A' <= c && c <= 'F') return c - 'A';
	die("char '%1$c' (\\x%1$02x) isn't a hex digit", c);
}

static void parse_arabic_numeral(struct sq_token *token) {
	token->kind = SQ_TK_NUMBER;
	token->number = 0;

	do {
		token->number = token->number * 10 + (*sq_stream - '0');
	} while (isdigit(*++sq_stream));

	if (isalpha(*sq_stream) || *sq_stream == '_')
		die("invalid trailing characters on arabic numeral literal: %llu%c\n",
			(long long) token->number, *sq_stream);
}

struct sq_token sq_next_token() {
	strip_whitespace();
	struct sq_token token;

	if (!*sq_stream) {
		token.kind = SQ_TK_UNDEFINED;
		return token;
	}
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDL);

	if (isdigit(*sq_stream))
		return parse_arabic_numeral(&token), token;

	if (sq_roman_is_numeral(*sq_stream)) {
		token.number = sq_roman_to_number(sq_stream, &sq_stream);
		if (token.number >= 0)
			return (token.kind = SQ_TK_NUMBER), token;
	}

	if (*sq_stream == '\\') {
		++sq_stream;

		if (sq_stream[1] && *++sq_stream != '\n') 
			die("unexpected '\\' on its own.");
	}

	if (*sq_stream == '\'' || *sq_stream == '\"') {
		token.kind = SQ_TK_STRING;
		token.string = malloc(sizeof(struct sq_string));
		char quote = *sq_stream;
		const char *start = ++sq_stream;

		while (*sq_stream != quote) {
			if (*sq_stream == '\\') ++sq_stream;
			if (!*sq_stream) die("unterminated quote found");
			++sq_stream;
		}
		++sq_stream;

		token.string->length = sq_stream - start - 1;
		char *dst = xmalloc(token.string->length + 1);
		unsigned ins = 0;
		char c;
		const char *src = start;
		for (unsigned i = 0; ins <= token.string->length; dst[ins++] = c, ++i) {
		top:
			c = src[i];

			if (c != '\\') continue;

			token.string->length--;
			if (i == token.string->length) die("unterminated escape sequence");
			switch (c = src[++i]) {
			case '\\': break;
			case '\'': break;
			case '\"': break;
			case '\n': ++i; goto top;
			case 'n': c = '\n'; break;
			case 't': c = '\t'; break;
			case 'f': c = '\f'; break;
			case 'v': c = '\v'; break;
			case 'r': c = '\r'; break;
			case 'x':
				if (src[i+1] == quote || src[i+1] == '\0' ||src[i+2] == quote)
					die("unterminated escape sequence");
				c = tohex(src[i+1]) * 16 + tohex(src[i+2]);
				i+=2;
				token.string->length-=2;
				break;
			default:
				die("unknown escape character '%1$c' (\\x%1$02x)", c);
			}
		}
		token.string->ptr = xrealloc(dst, token.string->length+1);
		token.string->ptr[token.string->length] = '\0';
		return token;
	}

	if (!strncmp(sq_stream, "__END__", 7)) {
		token.kind = SQ_TK_UNDEFINED;
		return token;
	}

/*
archetype

class Child {
	field name, age;
	constructor(quux) { ... }
	function(...) { ... }
	static function(...) { ... }
}

form RobinHood isa Myth {
	matter bow, name

	substantiate(name) {
		thine.bow = "bow"
		thine.name = name
	}

	describe please_help() {
		proclaim("Oh robin hood, please save us!\n")
	}

	action steal_from(whom) {
		reward whom.take( )
	}

	# member function
	journey steal_from(whom) {
		proclaim("Hello, " + whom.name + "\n")
		proclaim("Give me your money or i'll shoot you with my " + thine.weapon + "\n")
		reward whom.take()
	}


}
# class definition
form Child isa Parent {
	# fields
	matter name, age;
	
	# constructor
	substantiate(...) { ...}
	# class function
	describe(...) { ... }

	# instance function
	???(...) { ... }
}

myth Foo {
	field bar, baz;

	realize(quux) { ... }
	recite(...) { ... }
	journey(blargh) { ... }
}
*/
	CHECK_FOR_START_KW("form",         SQ_TK_CLASS);
	CHECK_FOR_START_KW("matter",       SQ_TK_FIELD);
	CHECK_FOR_START_KW("action",       SQ_TK_METHOD);
	CHECK_FOR_START_KW("describe",     SQ_TK_CLASSFN);
	CHECK_FOR_START_KW("substantiate", SQ_TK_CONSTRUCTOR);

	CHECK_FOR_START_KW("journey",      SQ_TK_FUNC);
	CHECK_FOR_START_KW("renowned",     SQ_TK_GLOBAL);
	CHECK_FOR_START_KW("nigh",         SQ_TK_LOCAL);
	CHECK_FOR_START_KW("import",       SQ_TK_IMPORT); // `befriend`? `beseech`?

	CHECK_FOR_START_KW("if",           SQ_TK_IF); // _should_ we have a better one?
	CHECK_FOR_START_KW("alas",         SQ_TK_ELSE);
	CHECK_FOR_START_KW("whence",       SQ_TK_COMEFROM);
	CHECK_FOR_START_KW("whilst",       SQ_TK_WHILE);
	CHECK_FOR_START_KW("reward",       SQ_TK_RETURN);
	CHECK_FOR_START_KW("attempt",      SQ_TK_TRY);
	CHECK_FOR_START_KW("retreat",      SQ_TK_CATCH);
	CHECK_FOR_START_KW("hark",         SQ_TK_THROW);
	CHECK_FOR_START_KW("feint",        SQ_TK_UNDO);

	CHECK_FOR_START_KW("yay",          SQ_TK_TRUE);
	CHECK_FOR_START_KW("nay",          SQ_TK_FALSE);
	CHECK_FOR_START_KW("null",         SQ_TK_NULL); // `naught`?

	if (isalpha(*sq_stream) || *sq_stream == '_') {
		token.kind = SQ_TK_IDENT;
		const char *start = sq_stream;

		while (isalnum(*sq_stream) || *sq_stream == '_')
			++sq_stream;

		token.identifier = strndup(start, sq_stream - start);

		// check to see if we're a label
		while (isspace(*sq_stream) || *sq_stream == '#')
			if (*sq_stream == '#')
				while (*sq_stream && *sq_stream++ != '\n');
			else
				++sq_stream;

		if (*sq_stream == ':')
			++sq_stream, token.kind = SQ_TK_LABEL;

		return token;
	}

	CHECK_FOR_START("{", SQ_TK_LBRACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("(", SQ_TK_LPAREN);
	CHECK_FOR_START(")", SQ_TK_RPAREN);
	CHECK_FOR_START("[", SQ_TK_LBRACKET);
	CHECK_FOR_START("]", SQ_TK_RBRACKET);
	CHECK_FOR_START(";", SQ_TK_ENDL);
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDL);
	CHECK_FOR_START(",", SQ_TK_COMMA);
	CHECK_FOR_START(".", SQ_TK_DOT);

	CHECK_FOR_START("==", SQ_TK_EQL);
	CHECK_FOR_START("!=", SQ_TK_NEQ);
	CHECK_FOR_START("<=", SQ_TK_LEQ);
	CHECK_FOR_START(">=", SQ_TK_GEQ);
	CHECK_FOR_START("<", SQ_TK_LTH);
	CHECK_FOR_START(">", SQ_TK_GTH);
	CHECK_FOR_START("+", SQ_TK_ADD);
	CHECK_FOR_START("-", SQ_TK_SUB);
	CHECK_FOR_START("*", SQ_TK_MUL);
	CHECK_FOR_START("/", SQ_TK_DIV);
	CHECK_FOR_START("%", SQ_TK_MOD);
	CHECK_FOR_START("!", SQ_TK_NOT);
	CHECK_FOR_START("&&", SQ_TK_AND);
	CHECK_FOR_START("||", SQ_TK_OR);
	CHECK_FOR_START("=", SQ_TK_ASSIGN);

	die("unknown token start '%c'", *sq_stream);
}


void sq_token_dump(const struct sq_token *token) {
	switch (token->kind) {
	case SQ_TK_UNDEFINED: printf("Keyword(undefined)"); break;
	case SQ_TK_CLASS: printf("Keyword(class)"); break;
	case SQ_TK_FUNC: printf("Keyword(func)"); break;
	case SQ_TK_GLOBAL: printf("Keyword(global)"); break;
	case SQ_TK_IF: printf("Keyword(if)"); break;
	case SQ_TK_ELSE: printf("Keyword(else)"); break;
	case SQ_TK_RETURN: printf("Keyword(return)"); break;
	case SQ_TK_TRUE: printf("Keyword(true)"); break;
	case SQ_TK_FALSE: printf("Keyword(false)"); break;

	case SQ_TK_IDENT: printf("Ident(%s)", token->identifier); break;
	case SQ_TK_NUMBER: printf("Number(%lld)", (long long) token->number); break;
	case SQ_TK_STRING: printf("String(%s)", token->string->ptr); break;

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
	case SQ_TK_AND: printf("Operator(&&)"); break;
	case SQ_TK_OR: printf("Operator(||)"); break;
	case SQ_TK_ASSIGN: printf("Operator(=)"); break;

	default: printf("Unknown(%d)", token->kind); break;
	}
}
