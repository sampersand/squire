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
	SQ_INT_BABEL        = 0x34, // [A,...,B,DST] DST <- babel(exec=A,stdin=B,args=...)

	SQ_INT_ARABIC       = 0x40, // [A,DST] DST <- A.to_numeral().arabic()
	SQ_INT_ROMAN        = 0x41, // [A,DST] DST <- A.to_numeral().roman()

	// temporary hacks until we get kingdoms working.
	SQ_INT_FOPEN,

	// ASCII
	SQ_INT_ASCII,
};


#define SQ_OPCODE_MAX_ARITY 3 // the max amount of operands (3) is from INDEX_ASSIGN
#define SQ_OPCODE_SHIFT_AMOUNT 2
_Static_assert(SQ_OPCODE_MAX_ARITY < (1 << SQ_OPCODE_SHIFT_AMOUNT), "max arity and shift amnt mismatch");

#define SQ_OPCODE(arity, id) ( \
	((id) << SQ_OPCODE_SHIFT_AMOUNT) | (arity) \
)
	
/*
 * bottom 2 bits are size
 */
enum sq_opcode {
	SQ_OC_UNDEFINED     = SQ_OPCODE(0,  0), // should never occur in code
	SQ_OC_NOOP          = SQ_OPCODE(0,  1), // [] do nothing
	SQ_OC_MOV           = SQ_OPCODE(1,  0), // [SRC,DST] Dst <- SRC
	SQ_OC_INT           = SQ_OPCODE(0,  2), // [INT,...] Does the interrupt.

	SQ_OC_JMP           = SQ_OPCODE(0,  3), // [POS] IP <- POS
	SQ_OC_JMP_TRUE      = SQ_OPCODE(1,  1), // [CND,POS] IP <- POS if CND if true
	SQ_OC_JMP_FALSE     = SQ_OPCODE(1,  2), // [CND,POS] IP <- POS if CND is false
#ifndef SQ_NMOON_JOKE
	SQ_OC_WERE_JMP      = SQ_OPCODE(1,  3), // same as JMP_FALSE, but 1% chance not to on full moon
#endif /* !SQ_MOON_JOKE */
	SQ_OC_CALL          = SQ_OPCODE(1,  4), // [FN,NUM,...] Calls FN; NUM args are read
	SQ_OC_RETURN        = SQ_OPCODE(1,  7), // [IDX] Returns the given value
	SQ_OC_COMEFROM      = SQ_OPCODE(0,  4), // [AMNT,...] Performs COMEFROM for AMNT times
	SQ_OC_TRYCATCH      = SQ_OPCODE(0,  5), // [POS,ERR] Go when `catapult`s occur, set `ERR`
	SQ_OC_THROW         = SQ_OPCODE(1,  8), // [IDX] Throws an exception
	SQ_OC_POPTRYCATCH   = SQ_OPCODE(0,  6), // [] Removes a `catch` block from the stack.
	SQ_OC_CITE          = SQ_OPCODE(0,  7), // [A,DST] DST <- &A

	SQ_OC_NOT           = SQ_OPCODE(1, 10), // [A,DST] DST <- !A
	SQ_OC_NEG           = SQ_OPCODE(1, 11), // [A,DST] DST <- -A` (ie unary minus)
	SQ_OC_EQL           = SQ_OPCODE(2,  0), // [A,B,DST] DST <- A == B
	SQ_OC_NEQ           = SQ_OPCODE(2,  1), // [A,B,DST] DST <- A != B
	SQ_OC_LTH           = SQ_OPCODE(2,  2), // [A,B,DST] DST <- A < B
	SQ_OC_GTH           = SQ_OPCODE(2,  3), // [A,B,DST] DST <- A > B
	SQ_OC_LEQ           = SQ_OPCODE(2,  4), // [A,B,DST] DST <- A <= B
	SQ_OC_GEQ           = SQ_OPCODE(2,  5), // [A,B,DST] DST <- A >= B
	SQ_OC_CMP           = SQ_OPCODE(2,  6), // [A,B,DST] DST <- A <=> B
	SQ_OC_ADD           = SQ_OPCODE(2,  7), // [A,B,DST] DST <- A + B
	SQ_OC_SUB           = SQ_OPCODE(2,  8), // [A,B,DST] DST <- A - B
	SQ_OC_MUL           = SQ_OPCODE(2,  9), // [A,B,DST] DST <- A * B
	SQ_OC_DIV           = SQ_OPCODE(2, 10), // [A,B,DST] DST <- A / B
	SQ_OC_MOD           = SQ_OPCODE(2, 11), // [A,B,DST] DST <- A % B
	SQ_OC_POW           = SQ_OPCODE(2, 12), // [A,B,DST] DST <- A ^ B
	SQ_OC_INDEX         = SQ_OPCODE(2, 13), // [A,B,DST] DST <- A[B]
	SQ_OC_INDEX_ASSIGN  = SQ_OPCODE(3,  0), // [A,B,C] Performs `A[B]=C`; no destination.
	SQ_OC_MATCHES       = SQ_OPCODE(2, 14), // [A,B,DST] DST <- A.matches(B)
	SQ_OC_PAT_AND       = SQ_OPCODE(2, 15), // [A,B,DST] DST <- A & B
	SQ_OC_PAT_OR        = SQ_OPCODE(2, 16), // [A,B,DST] DST <- A | B
	SQ_OC_PAT_NOT       = SQ_OPCODE(1,  9), // [A,DST] DST <- ~A

	SQ_OC_CLOAD         = SQ_OPCODE(0,  9), // [CNST,DST] DST <- constant `CNST`
	SQ_OC_GLOAD         = SQ_OPCODE(0, 10), // [GLBL,DST] DST <- global `GLBL`
	SQ_OC_GSTORE        = SQ_OPCODE(1, 12), // [SRC,GLBL] global GLBL <- SRC
	SQ_OC_ILOAD         = SQ_OPCODE(1,  6), // [A,B,DST] DST <- A.B
	SQ_OC_ISTORE        = SQ_OPCODE(2, 17), // [A,B,C,DST] Performs `A.B=C`; (Stores in DST, though this is not intended)
	SQ_OC_FEGENUS_STORE = SQ_OPCODE(2, 18), // [A,B,C] Sets `A.B`'s kind to constant `C` (essence)
	SQ_OC_FMGENUS_STORE = SQ_OPCODE(2, 19), // [A,B,C] Sets `A.B`'s kind to constant `C` (matter)
};
#undef SQ_OPCODE

static inline unsigned sq_opcode_arity(enum sq_opcode opcode) {
	return opcode & ((1 << SQ_OPCODE_SHIFT_AMOUNT) - 1);
}

union sq_bytecode {
	enum sq_opcode opcode;
	unsigned index;
	enum sq_interrupt interrupt;
	unsigned count;
};

const char *sq_interrupt_repr(enum sq_interrupt interrupt);
const char *sq_opcode_repr(enum sq_opcode opcode);

#endif /* !SQ_BYTECODE_H */
