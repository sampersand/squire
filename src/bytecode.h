#pragma once

enum sq_opcode {
	SQ_OC_UNDEFINED,
	SQ_OC_SWAP,

	SQ_OC_JMP,
	SQ_OC_JMP_FALSE,
	SQ_OC_CALL,
	SQ_OC_RETURN,

	SQ_OC_EQL,
	SQ_OC_LTH,
	SQ_OC_GTH,
	SQ_OC_ADD,
	SQ_OC_SUB,
	SQ_OC_MUL,
	SQ_OC_DIV,
	SQ_OC_MOD,
	SQ_OC_NOT,

	SQ_OC_INEW, // create a struct instance

	SQ_OC_CLOAD,  // load a constant
	SQ_OC_GLOAD,  // load a global
	SQ_OC_ILOAD,  // load an instance field
	SQ_OC_ALOAD,  // load an array index

	SQ_OC_GSTORE, // store a global
	SQ_OC_ISTORE, // store an instance field
	SQ_OC_ASTORE, // store an array index
};

typedef unsigned sq_index;

union sq_bytecode {
	enum sq_opcode opcode;
	sq_index index;
};