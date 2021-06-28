#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Interrupt {
	NewArray,
	NewCodex,
}

	// // String stuff
// }
// enum sq_interrupts {
// 	SQ_INT_TONUMBER           = 0x01,
// 	SQ_INT_TOSTRING           = 0x02,
// 	SQ_INT_TOBOOLEAN          = 0x03,
// 	SQ_INT_KINDOF             = 0x04,

// 	SQ_INT_PRINT              = 0x10,
// 	SQ_INT_DUMP               = 0x11,
// 	SQ_INT_PROMPT             = 0x12,
// 	SQ_INT_SYSTEM             = 0x13,
// 	SQ_INT_EXIT               = 0x14,
// 	SQ_INT_RANDOM             = 0x15,

// 	SQ_INT_SUBSTR             = 0x20,
// 	SQ_INT_LENGTH             = 0x21,

// 	SQ_INT_CODEX_NEW          = 0x30,
// 	SQ_INT_ARRAY_NEW          = 0x31,
// 	SQ_INT_ARRAY_INSERT       = 0x32,
// 	SQ_INT_ARRAY_DELETE       = 0x33,

// 	SQ_INT_ARABIC             = 0x40,
// 	SQ_INT_ROMAN              = 0x41,
// };

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Opcode {
	// MISC
	NoOp,
	Move,
	Interrupt,

	// Control flow
	Jump,
	JumpIfFalse,
	JumpIfTrue,
	
	CallFast,
	CallComplex,
	Return,

	ComeFrom,

	Throw,
	Attempt,
	PopHandler,

	// Logical Operations
	Not,
	Equals,
	NotEquals,
	LessThan,
	LessThanOrEqual,
	GreaterThan,
	GreaterThanOrEqual,
	Compare,

	// Math
	Pos,
	Negate,
	Add,
	Subtract,
	Multiply,
	Divide,
	Modulo,
	Power,

	// Misc Operators
	Index,
	IndexAssign,

	// VM-specific
	LoadConstant,
	LoadGlobal,
	StoreGlobal,
	GetAttribute,
	SetAttribute,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Bytecode {
	Opcode(Opcode),
	Interrupt(Interrupt),
	Local(usize),
	Global(usize),
	Constant(usize),
	Offset(isize),
	Count(usize),
	Illegal
}

// pub enum BuiltinFns {
	// // Misc
	// Random,

	// // Types
	// ToNumber,
	// ToString,
	// ToBoolean,
	// KindOf,

	// // I/O
	// Print,
	// Dump,
	// Prompt,
	// System,
	// Exit,

	// // String stuff
// }
// enum sq_interrupts {
// 	SQ_INT_TONUMBER           = 0x01,
// 	SQ_INT_TOSTRING           = 0x02,
// 	SQ_INT_TOBOOLEAN          = 0x03,
// 	SQ_INT_KINDOF             = 0x04,

// 	SQ_INT_PRINT              = 0x10,
// 	SQ_INT_DUMP               = 0x11,
// 	SQ_INT_PROMPT             = 0x12,
// 	SQ_INT_SYSTEM             = 0x13,
// 	SQ_INT_EXIT               = 0x14,
// 	SQ_INT_RANDOM             = 0x15,

// 	SQ_INT_SUBSTR             = 0x20,
// 	SQ_INT_LENGTH             = 0x21,

// 	SQ_INT_CODEX_NEW          = 0x30,
// 	SQ_INT_ARRAY_NEW          = 0x31,
// 	SQ_INT_ARRAY_INSERT       = 0x32,
// 	SQ_INT_ARRAY_DELETE       = 0x33,

// 	SQ_INT_ARABIC             = 0x40,
// 	SQ_INT_ROMAN              = 0x41,
// };