#ifndef SQ_BYTECODE_H
#define SQ_BYTECODE_H

enum sq_interrupts {
	SQ_INT_TONUMBER           = 0x01,
	SQ_INT_TOSTRING           = 0x02,
	SQ_INT_TOBOOLEAN          = 0x03,
	SQ_INT_KINDOF             = 0x04,

	SQ_INT_PRINT              = 0x10,
	SQ_INT_DUMP               = 0x11,
	SQ_INT_PROMPT             = 0x12,
	SQ_INT_SYSTEM             = 0x13,
	SQ_INT_EXIT               = 0x14,
	SQ_INT_RANDOM             = 0x15,

	SQ_INT_SUBSTR             = 0x20,
	SQ_INT_LENGTH             = 0x21,

	SQ_INT_DICT_NEW           = 0x30,
	SQ_INT_ARRAY_NEW          = 0x31,
	SQ_INT_ARRAY_INSERT       = 0x32,
	SQ_INT_ARRAY_DELETE       = 0x33,

	SQ_INT_ARABIC             = 0x40,
	SQ_INT_ROMAN              = 0x41,
};

enum sq_opcode {
	SQ_OC_UNDEFINED    = 0x00,
	SQ_OC_SWAP         = 0x01,
	SQ_OC_NOOP         = 0x02,
	SQ_OC_MOV          = 0x03,
	SQ_OC_INT          = 0x04,

	SQ_OC_JMP          = 0x10,
	SQ_OC_JMP_FALSE    = 0x11,
	SQ_OC_JMP_TRUE     = 0x12,
	SQ_OC_CALL         = 0x13,
	SQ_OC_RETURN       = 0x14,
	SQ_OC_COMEFROM     = 0x15,
	SQ_OC_TRYCATCH     = 0x16,
	SQ_OC_THROW        = 0x17,
	SQ_OC_UNDO         = 0x18,
	SQ_OC_POPTRYCATCH  = 0x19,

	SQ_OC_EQL          = 0x20,
	SQ_OC_NEQ          = 0x21,
	SQ_OC_LTH          = 0x22,
	SQ_OC_GTH          = 0x23,
	SQ_OC_LEQ          = 0x24,
	SQ_OC_GEQ          = 0x25,
	SQ_OC_ADD          = 0x26,
	SQ_OC_SUB          = 0x27,
	SQ_OC_MUL          = 0x28,
	SQ_OC_DIV          = 0x29,
	SQ_OC_MOD          = 0x2A,
	SQ_OC_NOT          = 0x2B,
	SQ_OC_NEG          = 0x2C,
	SQ_OC_INDEX        = 0x2D,
	SQ_OC_INDEX_ASSIGN = 0x2E,

	SQ_OC_CLOAD        = 0x30, // load a constant

	SQ_OC_GLOAD        = 0x40, // load a global
	SQ_OC_GSTORE       = 0x41, // store a global

	SQ_OC_ILOAD        = 0x50, // load an instance field
	SQ_OC_ISTORE       = 0x51, // store an instance field
};

typedef unsigned sq_index;

union sq_bytecode {
	enum sq_opcode opcode;
	sq_index index;
};

#endif /* !SQ_BYTECODE_H */
