#include "token.h"
#include <ctype.h>
#include <string.h>
#include "shared.h"

#define PEEK(stream) (**(stream))
#define ADVANCE(stream) (++*(stream))
#define PEEK_ADVANCE(stream) (*(*(stream))++)
#define NEXT(stream) (*(*(stream))++)

static void strip_whitespace(const char **stream) {
	char c;

	// strip whitespace
	while ((c = **stream)) {
		if (c == '#') {
			do {
				c = *++*stream;
			} while (c && c != '\n');
			if (c == '\n') ++*stream;
			continue;
		}

		if (!isspace(c) || c == '\n')
			break;

		while (isspace(c) && c != '\n') {
			c = *++*stream;
		}
	}
}

#define CHECK_FOR_START(str, tkn) \
	if (!strncmp(str, *stream, strlen(str))) {\
		*stream += strlen(str); token.kind = tkn; return token; \
	}

#define CHECK_FOR_START_KW(str, tkn) \
	if (!strncmp(str, *stream, strlen(str)) \
		&& !isalnum(*(*stream + strlen(str))) && *(*stream + strlen(str)) != '_') {\
		*stream += strlen(str); token.kind = tkn; return token; }

sq_token sq_next_token(const char **stream) {
	strip_whitespace(stream);
	sq_token token;

	if (!**stream) {
		token.kind = SQ_TK_UNDEFINED;
		return token;
	}
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDLINE);

	if (isdigit(**stream)) {
		token.kind = SQ_TK_NUMBER;
		token.number = 0;

		do {
			token.number = token.number * 10 + (**stream - '0');
		} while (isdigit(*++*stream));

		if (isalpha(**stream) || **stream == '_')
			die("invalid trailing characters on number literal: %llu%c\n",
				(long long) token.number, **stream);
		return token;
	}

	if (**stream == '\'' || **stream == '\"') {
		token.kind = SQ_TK_STRING;
		token.string = malloc(sizeof(struct sq_string));
		char quote = **stream;
		const char *start = ++*stream;

		while (**stream != quote) {
			if (!**stream) die("unterminated quote found");
			++*stream;
		}
		++*stream;

		token.string->length = *stream - start;
		token.string->ptr = strndup(start, token.string->length);
		return token;
	}

	CHECK_FOR_START_KW("struct", SQ_TK_STRUCT);
	CHECK_FOR_START_KW("func", SQ_TK_FUNC);
	CHECK_FOR_START_KW("if", SQ_TK_IF);
	CHECK_FOR_START_KW("else", SQ_TK_ELSE);
	CHECK_FOR_START_KW("return", SQ_TK_RETURN);
	CHECK_FOR_START_KW("true", SQ_TK_TRUE);
	CHECK_FOR_START_KW("false", SQ_TK_FALSE);
	CHECK_FOR_START_KW("null", SQ_TK_NULL);

	if (isalpha(**stream) || **stream == '_') {
		token.kind = SQ_TK_IDENT;
		const char *start = *stream;

		while (isalnum(**stream) || **stream == '_')
			++*stream;

		token.identifier = strndup(start, *stream - start);
		return token;
	}

	CHECK_FOR_START("{", SQ_TK_LRBACE);
	CHECK_FOR_START("}", SQ_TK_RBRACE);
	CHECK_FOR_START("(", SQ_TK_LPAREN);
	CHECK_FOR_START(")", SQ_TK_RPAREN);
	CHECK_FOR_START("[", SQ_TK_LBRACKET);
	CHECK_FOR_START("]", SQ_TK_RBRACKET);
	CHECK_FOR_START(";", SQ_TK_ENDLINE);
	CHECK_FOR_START("\n", SQ_TK_SOFT_ENDLINE);
	CHECK_FOR_START(",", SQ_TK_COMMA);
	CHECK_FOR_START(".", SQ_TK_DOT);

	CHECK_FOR_START("==", SQ_TK_EQL);
	CHECK_FOR_START("!=", SQ_TK_NEQ);
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

	die("unknown token start '%c'", **stream);
}


void sq_token_dump(const sq_token *token) {
	switch (token->kind) {
	case SQ_TK_UNDEFINED: printf("Keyword(undefined)"); break;
	case SQ_TK_STRUCT: printf("Keyword(struct)"); break;
	case SQ_TK_FUNC: printf("Keyword(func)"); break;
	case SQ_TK_IF: printf("Keyword(if)"); break;
	case SQ_TK_ELSE: printf("Keyword(else)"); break;
	case SQ_TK_RETURN: printf("Keyword(return)"); break;
	case SQ_TK_TRUE: printf("Keyword(true)"); break;
	case SQ_TK_FALSE: printf("Keyword(false)"); break;

	case SQ_TK_IDENT: printf("Ident(%s)", token->identifier); break;
	case SQ_TK_NUMBER: printf("Number(%lld)", (long long) token->number); break;
	case SQ_TK_STRING: printf("String(%s)", token->string->ptr); break;

	case SQ_TK_LRBACE: printf("Punct({)"); break;
	case SQ_TK_RBRACE: printf("Punct(})"); break;
	case SQ_TK_LPAREN: printf("Punct(()"); break;
	case SQ_TK_RPAREN: printf("Punct())"); break;
	case SQ_TK_LBRACKET: printf("Punct([)"); break;
	case SQ_TK_RBRACKET: printf("Punct(])"); break;
	case SQ_TK_ENDLINE: printf("Punct(;)"); break;
	case SQ_TK_SOFT_ENDLINE: printf("Punct(\\n)"); break;
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