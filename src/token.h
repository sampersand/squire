#ifndef SQ_TOKEN_H
#define SQ_TOKEN_H

#include "value.h"
#include "string.h"

enum sq_token_kind {
	SQ_TK_UNDEFINED = 0,

	SQ_TK_CLASS,
	SQ_TK_THIS,
	SQ_TK_FUNC,
	SQ_TK_FIELD,
	SQ_TK_CLASSFN,
	SQ_TK_CONSTRUCTOR,

	SQ_TK_GLOBAL,
	SQ_TK_LOCAL,
	SQ_TK_IMPORT,

	SQ_TK_IF,
	SQ_TK_ELSE,
	SQ_TK_COMEFROM,
	SQ_TK_WHILE,
	SQ_TK_RETURN,
	SQ_TK_TRY,
	SQ_TK_CATCH,
	SQ_TK_THROW,
	SQ_TK_UNDO,
	// TODO: `assert` as `challenge`?
	// TODOO: `alias` is `henceforth`

	SQ_TK_TRUE,
	SQ_TK_FALSE,
	SQ_TK_NULL,

	SQ_TK_IDENT,
	SQ_TK_NUMBER,
	SQ_TK_STRING,
	SQ_TK_LABEL,

	SQ_TK_LBRACE,
	SQ_TK_RBRACE,
	SQ_TK_LPAREN,
	SQ_TK_RPAREN,
	SQ_TK_LBRACKET,
	SQ_TK_RBRACKET,
	SQ_TK_ENDL,
	SQ_TK_SOFT_ENDL,
	SQ_TK_COMMA,
	SQ_TK_DOT,

	SQ_TK_EQL,
	SQ_TK_NEQ,
	SQ_TK_LTH,
	SQ_TK_LEQ,
	SQ_TK_GTH,
	SQ_TK_GEQ,
	SQ_TK_ADD,
	SQ_TK_SUB,
	SQ_TK_MUL,
	SQ_TK_DIV,
	SQ_TK_MOD,
	SQ_TK_NOT,
	SQ_TK_AND,
	SQ_TK_OR,
	SQ_TK_ASSIGN,
};

struct sq_token {
	enum sq_token_kind kind;
	union {
		sq_number number;
		struct sq_string *string;
		char *identifier;
	};
};

extern const char *sq_stream;
struct sq_token sq_next_token(void);
void sq_token_dump(const struct sq_token *token);

#endif /* !SQ_TOKEN_H */
