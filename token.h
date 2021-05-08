#pragma once

#include "value.h"
#include "string.h"

typedef enum {
	SQ_TK_UNDEFINED,
	SQ_TK_STRUCT,
	SQ_TK_FUNC,
	SQ_TK_IF,
	SQ_TK_ELSE,
	SQ_TK_RETURN,
	SQ_TK_TRUE,
	SQ_TK_FALSE,
	SQ_TK_NULL,

	SQ_TK_IDENT,
	SQ_TK_NUMBER,
	SQ_TK_STRING,

	SQ_TK_LRBACE,
	SQ_TK_RBRACE,
	SQ_TK_LPAREN,
	SQ_TK_RPAREN,
	SQ_TK_LBRACKET,
	SQ_TK_RBRACKET,
	SQ_TK_ENDLINE,
	SQ_TK_SOFT_ENDLINE,
	SQ_TK_COMMA,
	SQ_TK_DOT,

	SQ_TK_EQL,
	SQ_TK_NEQ,
	SQ_TK_LTH,
	SQ_TK_GTH,
	SQ_TK_ADD,
	SQ_TK_SUB,
	SQ_TK_MUL,
	SQ_TK_DIV,
	SQ_TK_MOD,
	SQ_TK_NOT,
	SQ_TK_AND,
	SQ_TK_OR,
	SQ_TK_ASSIGN,
} sq_tk_kind;

// struct sq_tk_

typedef struct {
	sq_tk_kind kind;
	union {
		sq_number number;
		struct sq_string *string;
		char *identifier;
	};
} sq_token;

sq_token sq_next_token(const char **stream);
void sq_token_dump(const sq_token *);
