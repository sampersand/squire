#ifndef SQ_BYTECODE_H
#define SQ_BYTECODE_H

enum sq_interrupt {
	SQ_INT_TONUMERAL    = 0x01, // [A,DST] DST <- A.to_numeral()
	SQ_INT_TOTEXT       = 0x02, // [A,DST] DST <- A.to_text()
	SQ_INT_TOVERACITY   = 0x03, // [A,DST] DST <- A.to_veracity()
	SQ_INT_KINDOF       = 0x04, // [A,DST] DST <- A.genus

	SQ_INT_PRINT        = 0x10, // [A,DST] Print `A`, DST <- ni
	SQ_INT_PRINTLN      = 0x11, // [A,DST] Print `A` with a newline, DST <- ni
	SQ_INT_DUMP         = 0x12, // [A,DST] Dumps out `A`, DST <- A
	SQ_INT_PROMPT       = 0x13, // [DST] DST <- next line from stdin
	SQ_INT_SYSTEM       = 0x14, // [CMD,DST] DST <- stdout of running `cmd`.
	SQ_INT_EXIT         = 0x15, // [CODE] Exits with the given code.
	SQ_INT_RANDOM       = 0x16, // [DST] DST <- random numeral

	SQ_INT_SUBSTR       = 0x20, // [A,B,C,DST] DST <- A[B..B+C]
	SQ_INT_LENGTH       = 0x21, // [A,DST] DST <- length A: book/codex/text

	SQ_INT_CODEX_NEW    = 0x30, // [N,...,DST] DST <- N key-value pairs.
	SQ_INT_BOOK_NEW     = 0x31, // [N,...,DST] DST <- N-length array.
	SQ_INT_ARRAY_INSERT = 0x32, // [A,B,C,DST] A.insert(len=B,pos=C); (Stores in DST, though this is not intended)
	SQ_INT_ARRAY_DELETE = 0x33, // [A,B,DST] DST <- A.delete(B)

	SQ_INT_ARABIC       = 0x40, // [A,DST] DST <- A.to_numeral().arabic()
	SQ_INT_ROMAN        = 0x41, // [A,DST] DST <- A.to_numeral().roman()
};


enum sq_opcode {
	SQ_OC_UNDEFINED     = 0x00, // should never occur in code
	SQ_OC_NOOP          = 0x01, // [] do nothing
	SQ_OC_MOV           = 0x02, // [SRC,DST] Dst <- SRC
	SQ_OC_INT           = 0x03, // [INT,...] Does the interrupt.

	SQ_OC_JMP           = 0x10, // [POS] IP <- POS
	SQ_OC_JMP_FALSE     = 0x11, // [CND,POS] IP <- POS if CND is false
	SQ_OC_JMP_TRUE      = 0x12, // [CND,POS] IP <- POS if CND if true
	SQ_OC_CALL          = 0x13, // [FN,NUM,...] Calls FN; NUM args are read
	SQ_OC_RETURN        = 0x14, // [IDX] Returns the given value
	SQ_OC_COMEFROM      = 0x15, // [AMNT,...] Performs COMEFROM for AMNT times
	SQ_OC_TRYCATCH      = 0x16, // [POS,ERR] Go when `catapult`s occur, set `ERR`
	SQ_OC_THROW         = 0x17, // [IDX] Throws an exception
	SQ_OC_POPTRYCATCH   = 0x19, // [] Removes a `catch` block from the stack.

	SQ_OC_NOT           = 0x20, // [A,DST] DST <- !A
	SQ_OC_NEG           = 0x21, // [A,DST] DST <- -A` (ie unary minus)
	SQ_OC_EQL           = 0x22, // [A,B,DST] DST <- A == B
	SQ_OC_NEQ           = 0x23, // [A,B,DST] DST <- A != B
	SQ_OC_LTH           = 0x24, // [A,B,DST] DST <- A < B
	SQ_OC_GTH           = 0x25, // [A,B,DST] DST <- A > B
	SQ_OC_LEQ           = 0x26, // [A,B,DST] DST <- A <= B
	SQ_OC_GEQ           = 0x27, // [A,B,DST] DST <- A >= B
	SQ_OC_ADD           = 0x28, // [A,B,DST] DST <- A + B
	SQ_OC_SUB           = 0x29, // [A,B,DST] DST <- A - B
	SQ_OC_MUL           = 0x2a, // [A,B,DST] DST <- A * B
	SQ_OC_DIV           = 0x2b, // [A,B,DST] DST <- A / B
	SQ_OC_MOD           = 0x2c, // [A,B,DST] DST <- A % B
	SQ_OC_INDEX         = 0x2d, // [A,B,DST] DST <- A[B]
	SQ_OC_INDEX_ASSIGN  = 0x2e, // [A,B,C] Performs `A[B]=C`; no destination.
	SQ_OC_MATCHES       = 0x2F, // [A,B,DST] DST <- A.matches(B)

	SQ_OC_CLOAD         = 0x30, // [CNST,DST] DST <- constant `CNST`
	SQ_OC_GLOAD         = 0x31, // [GLBL,DST] DST <- global `GLBL`
	SQ_OC_GSTORE        = 0x32, // [SRC,GLBL] global GLBL <- SRC
	SQ_OC_ILOAD         = 0x33, // [A,B,DST] DST <- A.B
	SQ_OC_ISTORE        = 0x34, // [A,B,C,DST] Performs `A.B=C`; (Stores in DST, though this is not intended)
	SQ_OC_FEGENUS_STORE = 0x35, // [A,B,C] Sets `A.B`'s kind to constant `C` (essence)
	SQ_OC_FMGENUS_STORE = 0x36, // [A,B,C] Sets `A.B`'s kind to constant `C` (matter)
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
