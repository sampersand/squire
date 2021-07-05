use std::fmt::{self, Debug, Formatter};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Interrupt {
	NewArray,
	NewCodex,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum Opcode {
	// MISC
	NoOp,
	Move,
	Interrupt,
	Interpolate,
	CheckMatches,

	// Control flow
	Jump,
	JumpIfFalse,
	JumpIfTrue,
	
	// CallFast,
	// CallComplex,
	Call,
	Return,
	ComeFrom,

	Throw,
	Attempt,
	PopHandler,

	// Logical Operations
	Not,
	Matches,
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

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
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

impl Debug for Bytecode {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Self::Opcode(opcode) => write!(f, "Opcode({:?})", opcode),
			Self::Interrupt(interrupt) => write!(f, "Interrupt({:?})", interrupt),
			Self::Local(local) => write!(f, "Local({:?})", local),
			Self::Global(global) => write!(f, "Global({:?})", global),
			Self::Constant(constant) => write!(f, "Constant({:?})", constant),
			Self::Offset(offset) => write!(f, "Offset({:?})", offset),
			Self::Count(count) => write!(f, "Count({:?})", count),
			Self::Illegal => write!(f, "Illegal"),
		}	
	}
}