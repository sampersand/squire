use crate::value::{Value, ValueKind};
use crate::value::numeral::NumeralParseError;
use std::fmt::{self, Display, Formatter};

#[derive(Debug)]
pub enum Error {
	OperationNotSupported { kind: ValueKind, func: &'static str },
	InvalidOperand { kind: ValueKind, func: &'static str },
	ValueError(String),
	DivisionByZero,
	OutOfBounds,
	ArgumentError { given: usize, expected: usize },
	Throw(Value),
	UnknownAttribute(String),
	Other(Box<dyn std::error::Error>),
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<NumeralParseError> for Error {
	#[inline]
	fn from(error: NumeralParseError) -> Self {
		Self::Other(Box::new(error))
	}
}

impl Display for Error {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Self::OperationNotSupported { kind, func } => write!(f, "cannot call '{:?}' with a {:?}", func, kind),
			Self::InvalidOperand { kind, func } => write!(f, "value {:?} was invalid for {:?}", kind, func),
			Self::ValueError(string) => write!(f, "{}", string),
			Self::DivisionByZero => write!(f, "divided by zero"),
			Self::OutOfBounds => write!(f, "value was out of bounds"),
			Self::ArgumentError { given, expected } => write!(f, "argc mismatch: given {}, expected {}", given, expected),
			Self::Throw(value) => write!(f, "uncaught throw: {:?}", value),
			Self::UnknownAttribute(attr) => write!(f, "unknown attribute accessed: {:?}", attr),
			Self::Other(err) => Display::fmt(&err, f),
		}

	}
}