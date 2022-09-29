#ifndef SQ_BYTECODE_H
#define SQ_BYTECODE_H

enum sq_interrupt {
	SQ_INT_UNDEFINED    = 0x00,
	SQ_INT_TONUMERAL    = 0x01, // [A,DST] DST <- A.to_numeral()
	SQ_INT_TOTEXT       = 0x02, // [A,DST] DST <- A.to_text()
	SQ_INT_TOVERACITY   = 0x03, // [A,DST] DST <- A.to_veracity()
	SQ_INT_TOBOOK       = 0x04, // [A,DST] DST <- A.to_book()
	SQ_INT_TOCODEX      = 0x05, // [A,DST] DST <- A.to_codex()
	SQ_INT_KINDOF       = 0x06, // [A,DST] DST <- A.genus

	SQ_INT_PRINT        = 0x10, // [A,DST] Print `A`, DST <- ni
	SQ_INT_PRINTLN      = 0x11, // [A,DST] Print `A` with a newline, DST <- ni
	SQ_INT_DUMP         = 0x12, // [A,DST] Dumps out `A`, DST <- A
	SQ_INT_PROMPT       = 0x13, // [DST] DST <- next line from stdin
	SQ_INT_SYSTEM       = 0x14, // [CMD,DST] DST <- stdout of running `cmd`.
	SQ_INT_EXIT         = 0x15, // [CODE] Exits with the given code.
	SQ_INT_RANDOM       = 0x16, // [DST] DST <- random numeral
	SQ_INT_PTR_GET      = 0x17, // [A,DST] DST <- *A
	SQ_INT_PTR_SET      = 0x18, // [A,B,DST] DST <- *A = B

	SQ_INT_SUBSTR       = 0x20, // [A,B,C,DST] DST <- A[B..B+C]
	SQ_INT_LENGTH       = 0x21, // [A,DST] DST <- length A: book/codex/text

	SQ_INT_CODEX_NEW    = 0x30, // [N,...,DST] DST <- N key-value pairs.
	SQ_INT_BOOK_NEW     = 0x31, // [N,...,DST] DST <- N-length array.
	SQ_INT_ARRAY_INSERT = 0x32, // [A,B,C,DST] A.insert(len=B,pos=C); (Stores in DST, though this is not intended)
	SQ_INT_ARRAY_DELETE = 0x33, // [A,B,DST] DST <- A.delete(B)

	SQ_INT_ARABIC       = 0x40, // [A,DST] DST <- A.to_numeral().arabic()
	SQ_INT_ROMAN        = 0x41, // [A,DST] DST <- A.to_numeral().roman()

	// temporary hacks until we get kingdoms working.
	SQ_INT_FOPEN,

	// ASCII
	SQ_INT_ASCII,
};


enum sq_opcode {
	SQ_OC_UNDEFINED     = 0x00, // should never occur in code
	SQ_OC_NOOP          = 0x01, // [] do nothing
	SQ_OC_MOV           = 0x02, // [SRC,DST] Dst <- SRC
	SQ_OC_INT           = 0x03, // [INT,...] Does the interrupt.

	SQ_OC_JMP           = 0x20, // [POS] IP <- POS
	SQ_OC_JMP_FALSE     = 0x21, // [CND,POS] IP <- POS if CND is false
	SQ_OC_JMP_TRUE      = 0x22, // [CND,POS] IP <- POS if CND if true
	SQ_OC_CALL          = 0x23, // [FN,NUM,...] Calls FN; NUM args are read
	SQ_OC_RETURN        = 0x24, // [IDX] Returns the given value
	SQ_OC_COMEFROM      = 0x25, // [AMNT,...] Performs COMEFROM for AMNT times
	SQ_OC_TRYCATCH      = 0x26, // [POS,ERR] Go when `catapult`s occur, set `ERR`
	SQ_OC_THROW         = 0x27, // [IDX] Throws an exception
	SQ_OC_POPTRYCATCH   = 0x29, // [] Removes a `catch` block from the stack.
#ifndef SQ_NMOON_JOKE
	SQ_OC_WERE_JMP      = 0x2A, // same as JMP_FALSE, but 1% chance not to on full moon
#endif /* !SQ_MOON_JOKE */

	SQ_OC_CITE          = 0x2B, // [A,DST] DST <- &A
	SQ_OC_NOT           = 0x40, // [A,DST] DST <- !A
	SQ_OC_NEG           = 0x41, // [A,DST] DST <- -A` (ie unary minus)
	SQ_OC_EQL           = 0x42, // [A,B,DST] DST <- A == B
	SQ_OC_NEQ           = 0x43, // [A,B,DST] DST <- A != B
	SQ_OC_LTH           = 0x44, // [A,B,DST] DST <- A < B
	SQ_OC_GTH           = 0x45, // [A,B,DST] DST <- A > B
	SQ_OC_LEQ           = 0x46, // [A,B,DST] DST <- A <= B
	SQ_OC_GEQ           = 0x47, // [A,B,DST] DST <- A >= B
	SQ_OC_CMP           = 0x48, // [A,B,DST] DST <- A <=> B
	SQ_OC_ADD           = 0x49, // [A,B,DST] DST <- A + B
	SQ_OC_SUB           = 0x4A, // [A,B,DST] DST <- A - B
	SQ_OC_MUL           = 0x4B, // [A,B,DST] DST <- A * B
	SQ_OC_DIV           = 0x4C, // [A,B,DST] DST <- A / B
	SQ_OC_MOD           = 0x4D, // [A,B,DST] DST <- A % B
	SQ_OC_POW           = 0x4E, // [A,B,DST] DST <- A ^ B
	SQ_OC_INDEX         = 0x4F, // [A,B,DST] DST <- A[B]
	SQ_OC_INDEX_ASSIGN  = 0x50, // [A,B,C] Performs `A[B]=C`; no destination.
	SQ_OC_MATCHES       = 0x51, // [A,B,DST] DST <- A.matches(B)

	SQ_OC_CLOAD         = 0x60, // [CNST,DST] DST <- constant `CNST`
	SQ_OC_GLOAD         = 0x61, // [GLBL,DST] DST <- global `GLBL`
	SQ_OC_GSTORE        = 0x62, // [SRC,GLBL] global GLBL <- SRC
	SQ_OC_ILOAD         = 0x63, // [A,B,DST] DST <- A.B
	SQ_OC_ISTORE        = 0x64, // [A,B,C,DST] Performs `A.B=C`; (Stores in DST, though this is not intended)
	SQ_OC_FEGENUS_STORE = 0x65, // [A,B,C] Sets `A.B`'s kind to constant `C` (essence)
	SQ_OC_FMGENUS_STORE = 0x66, // [A,B,C] Sets `A.B`'s kind to constant `C` (matter)
};

union sq_bytecode {
	enum sq_opcode opcode;
	unsigned index;
	enum sq_interrupt interrupt;
	unsigned count;
};

const char *sq_interrupt_repr(enum sq_interrupt interrupt);
const char *sq_opcode_repr(enum sq_opcode opcode);

#endif /* !SQ_BYTECODE_H */
