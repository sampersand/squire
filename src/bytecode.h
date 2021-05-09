#pragma once

enum sq_interrupts {
	SQ_INT_PRINT,
	SQ_INT_TONUMBER,
	SQ_INT_TOSTRING,
	SQ_INT_DUMP
};

enum sq_opcode {
	SQ_OC_UNDEFINED    = 0x00,
	SQ_OC_SWAP         = 0x01,
	SQ_OC_NOOP         = 0x02,
	SQ_OC_MOV          = 0x03,
	SQ_OC_INT          = 0x04,

	SQ_OC_JMP          = 0x10,
	SQ_OC_JMP_FALSE    = 0x11,
	SQ_OC_CALL         = 0x12,
	SQ_OC_RETURN       = 0x13,

	SQ_OC_EQL          = 0x20,
	SQ_OC_NEQ          = 0x21,
	SQ_OC_LTH          = 0x22,
	SQ_OC_GTH          = 0x23,
	SQ_OC_ADD          = 0x24,
	SQ_OC_SUB          = 0x25,
	SQ_OC_MUL          = 0x26,
	SQ_OC_DIV          = 0x27,
	SQ_OC_MOD          = 0x28,
	SQ_OC_NOT          = 0x29,
	SQ_OC_NEG          = 0x2a,

	SQ_OC_CLOAD        = 0x30, // load a constant

	SQ_OC_GLOAD        = 0x40, // load a global
	SQ_OC_GSTORE       = 0x41, // store a global

	SQ_OC_ILOAD        = 0x50, // load an instance field
	SQ_OC_ISTORE       = 0x51, // store an instance field
	SQ_OC_INEW         = 0x52, // create a struct instance
};

typedef unsigned sq_index;

union sq_bytecode {
	enum sq_opcode opcode;
	sq_index index;
};