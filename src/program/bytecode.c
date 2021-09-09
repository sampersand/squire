#include <squire/bytecode.h>

const char *sq_interrupt_repr(enum sq_interrupt interrupt) {
	switch (interrupt) {
	case SQ_INT_TONUMERAL: return "SQ_INT_TONUMERAL";
	case SQ_INT_TOTEXT: return "SQ_INT_TOTEXT";
	case SQ_INT_TOVERACITY: return "SQ_INT_TOVERACITY";
	case SQ_INT_TOBOOK: return "SQ_INT_TOBOOK";
	case SQ_INT_TOCODEX: return "SQ_INT_TOCODEX";
	case SQ_INT_KINDOF: return "SQ_INT_KINDOF";

	case SQ_INT_PRINT: return "SQ_INT_PRINT";
	case SQ_INT_PRINTLN: return "SQ_INT_PRINTLN";
	case SQ_INT_DUMP: return "SQ_INT_DUMP";
	case SQ_INT_PROMPT: return "SQ_INT_PROMPT";
	case SQ_INT_SYSTEM: return "SQ_INT_SYSTEM";
	case SQ_INT_EXIT: return "SQ_INT_EXIT";
	case SQ_INT_RANDOM: return "SQ_INT_RANDOM";

	case SQ_INT_SUBSTR: return "SQ_INT_SUBSTR";
	case SQ_INT_LENGTH: return "SQ_INT_LENGTH";

	case SQ_INT_CODEX_NEW: return "SQ_INT_CODEX_NEW";
	case SQ_INT_BOOK_NEW: return "SQ_INT_BOOK_NEW";
	case SQ_INT_ARRAY_INSERT: return "SQ_INT_ARRAY_INSERT";
	case SQ_INT_ARRAY_DELETE: return "SQ_INT_ARRAY_DELETE";

	case SQ_INT_ARABIC: return "SQ_INT_ARABIC";
	case SQ_INT_ROMAN: return "SQ_INT_ROMAN";

	case SQ_INT_FOPEN: return "SQ_INT_FOPEN";
	case SQ_INT_FCLOSE: return "SQ_INT_FCLOSE";
	case SQ_INT_FREAD: return "SQ_INT_FREAD";
	case SQ_INT_FREADALL: return "SQ_INT_FREADALL";
	case SQ_INT_FWRITE: return "SQ_INT_FWRITE";
	case SQ_INT_FTELL: return "SQ_INT_FTELL";
	case SQ_INT_FSEEK: return "SQ_INT_FSEEK";
	case SQ_INT_ASCII: return "SQ_INT_ASCII";
	}
}

const char *sq_opcode_repr(enum sq_opcode opcode) {
	switch (opcode) {
	case SQ_OC_UNDEFINED: return "SQ_OC_UNDEFINED";
	case SQ_OC_NOOP: return "SQ_OC_NOOP";
	case SQ_OC_MOV: return "SQ_OC_MOV";
	case SQ_OC_INT: return "SQ_OC_INT";
	
	case SQ_OC_JMP: return "SQ_OC_JMP";
	case SQ_OC_JMP_FALSE: return "SQ_OC_JMP_FALSE";
	case SQ_OC_JMP_TRUE: return "SQ_OC_JMP_TRUE";
	case SQ_OC_CALL: return "SQ_OC_CALL";
	case SQ_OC_RETURN: return "SQ_OC_RETURN";
	case SQ_OC_COMEFROM: return "SQ_OC_COMEFROM";
	case SQ_OC_TRYCATCH: return "SQ_OC_TRYCATCH";
	case SQ_OC_THROW: return "SQ_OC_THROW";
	case SQ_OC_POPTRYCATCH: return "SQ_OC_POPTRYCATCH";
	
	case SQ_OC_NOT: return "SQ_OC_NOT";
	case SQ_OC_NEG: return "SQ_OC_NEG";
	case SQ_OC_EQL: return "SQ_OC_EQL";
	case SQ_OC_NEQ: return "SQ_OC_NEQ";
	case SQ_OC_LTH: return "SQ_OC_LTH";
	case SQ_OC_GTH: return "SQ_OC_GTH";
	case SQ_OC_LEQ: return "SQ_OC_LEQ";
	case SQ_OC_GEQ: return "SQ_OC_GEQ";
	case SQ_OC_CMP: return "SQ_OC_CMP";
	case SQ_OC_ADD: return "SQ_OC_ADD";
	case SQ_OC_SUB: return "SQ_OC_SUB";
	case SQ_OC_MUL: return "SQ_OC_MUL";
	case SQ_OC_DIV: return "SQ_OC_DIV";
	case SQ_OC_MOD: return "SQ_OC_MOD";
	case SQ_OC_POW: return "SQ_OC_POW";
	case SQ_OC_INDEX: return "SQ_OC_INDEX";
	case SQ_OC_INDEX_ASSIGN: return "SQ_OC_INDEX_ASSIGN";
	case SQ_OC_MATCHES: return "SQ_OC_MATCHES";
	
	case SQ_OC_CLOAD: return "SQ_OC_CLOAD";
	case SQ_OC_GLOAD: return "SQ_OC_GLOAD";
	case SQ_OC_GSTORE: return "SQ_OC_GSTORE";
	case SQ_OC_ILOAD: return "SQ_OC_ILOAD";
	case SQ_OC_ISTORE: return "SQ_OC_ISTORE";
	case SQ_OC_FEGENUS_STORE: return "SQ_OC_FEGENUS_STORE";
	case SQ_OC_FMGENUS_STORE: return "SQ_OC_FMGENUS_STORE";
	}
}
