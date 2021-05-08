#pragma once

enum sq_opcode {
	SQ_OC_UNDEFINED    =  0,
	SQ_OC_SWAP         =  1,

	SQ_OC_JMP          = 10,
	SQ_OC_JMP_FALSE    = 11,
	SQ_OC_CALL         = 12,
	SQ_OC_RETURN       = 13,

	SQ_OC_EQL          = 20,
	SQ_OC_LTH          = 21,
	SQ_OC_GTH          = 22,
	SQ_OC_ADD          = 23,
	SQ_OC_SUB          = 24,
	SQ_OC_MUL          = 25,
	SQ_OC_DIV          = 26,
	SQ_OC_MOD          = 27,
	SQ_OC_NOT          = 28,

	SQ_OC_CLOAD        = 30, // load a constant

	SQ_OC_GLOAD        = 40, // load a global
	SQ_OC_GSTORE       = 41, // store a global

	SQ_OC_ILOAD        = 50, // load an instance field
	SQ_OC_ISTORE       = 51, // store an instance field
	SQ_OC_INEW         = 52, // create a struct instance
};

typedef unsigned sq_index;

union sq_bytecode {
	enum sq_opcode opcode;
	sq_index index;
};