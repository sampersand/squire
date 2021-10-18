#ifndef SQ_TOKEN_H
#define SQ_TOKEN_H

#include <squire/value.h>
#include <squire/text.h>

enum sq_token_kind {
	SQ_TK_UNDEFINED = 0,

	SQ_TK_CLASS = 0x10,
	SQ_TK_METHOD,
	SQ_TK_FIELD,
	SQ_TK_ESSENCE,
	SQ_TK_CLASSFN,
	SQ_TK_CONSTRUCTOR,
	SQ_TK_FUNC,
	SQ_TK_LAMBDA,

	SQ_TK_GLOBAL = 0x20,
	SQ_TK_LOCAL,

	SQ_TK_IF = 0x30,
	SQ_TK_ELSE,
	SQ_TK_COMEFROM,
	SQ_TK_GOTO,
	SQ_TK_WHILE,
	SQ_TK_RETURN,
	SQ_TK_TRY,
	SQ_TK_THROW,
	SQ_TK_SWITCH,
	SQ_TK_CASE,
	SQ_TK_REJOIN,
	SQ_TK_KINGDOM,
	// TODO: `assert` as `challenge`?

	SQ_TK_MACRO_VAR = 0x40,

	SQ_TK_YAY = 0x50,
	SQ_TK_NAY,
	SQ_TK_NI,

	SQ_TK_IDENT = 0x60,
	SQ_TK_NUMERAL,
	SQ_TK_TEXT,
	SQ_TK_LABEL,

	SQ_TK_LBRACE = 0x70,
	SQ_TK_RBRACE,
	SQ_TK_LPAREN,
	SQ_TK_RPAREN,
	SQ_TK_LBRACKET,
	SQ_TK_RBRACKET,
	SQ_TK_ENDL,
	SQ_TK_SOFT_ENDL,
	SQ_TK_COMMA,
	SQ_TK_COLON,
	SQ_TK_COLONCOLON,
	SQ_TK_DOT,
	SQ_TK_ARROW,

	SQ_TK_EQL = 0x80,
	SQ_TK_NEQ,
	SQ_TK_LTH,
	SQ_TK_LEQ,
	SQ_TK_GTH,
	SQ_TK_GEQ,
	SQ_TK_CMP,

	SQ_TK_ADD,
	SQ_TK_SUB,
	SQ_TK_MUL,
	SQ_TK_DIV,
	SQ_TK_MOD,
	SQ_TK_POW,
	SQ_TK_ADD_ASSIGN,
	SQ_TK_SUB_ASSIGN,
	SQ_TK_MUL_ASSIGN,
	SQ_TK_DIV_ASSIGN,
	SQ_TK_MOD_ASSIGN,
	SQ_TK_POW_ASSIGN,

	SQ_TK_NOT,
	SQ_TK_NEG,
	SQ_TK_AND,
	SQ_TK_OR,
	SQ_TK_MATCHES,
	SQ_TK_ASSIGN,
	SQ_TK_INDEX,
	SQ_TK_INDEX_ASSIGN,
};

struct sq_token {
	enum sq_token_kind kind;
	union {
		sq_numeral numeral;
		struct sq_text *text;
		char *identifier;
	};
};

extern const char *sq_stream;
struct sq_token sq_next_token(void);
void sq_token_dump(const struct sq_token *token);

#endif /* !SQ_TOKEN_H */
