use crate::value::{Value, ValueKind};
use crate::value::numeral::NumeralParseError;
use std::fmt::{self, Display, Formatter};
use std::borrow::Cow;
use std::io;

#[derive(Debug)]
pub enum Error {
	OperationNotSupported { kind: ValueKind, func: &'static str },
	CannotConvert { from: ValueKind, to: ValueKind },
	InvalidOperand { kind: ValueKind, func: &'static str },
	ValueError(String),
	DivisionByZero,
	OutOfBounds,
	ArgumentCountError { given: usize, expected: usize },
	ArgumentError(Cow<'static, str>),
	Throw(Value),
	UnknownAttribute(String),
	InvalidReturnType { expected: ValueKind, given: ValueKind, func: &'static str },
	Io(io::Error),
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
			Self::ArgumentCountError { given, expected } => write!(f, "argc mismatch: given {}, expected {}", given, expected),
			Self::ArgumentError(message) => write!(f, "argument error: {}", message),
			Self::Throw(value) => write!(f, "uncaught throw: {:?}", value),
			Self::UnknownAttribute(attr) => write!(f, "unknown attribute accessed: {:?}", attr),
			Self::InvalidReturnType { given, expected, func }
				=> write!(f, "bad return type for {}: given {:?}, expected {:?}", func, given, expected),
			Self::CannotConvert { from, to } => write!(f, "cannot convert a {:?} to a {:?}", from, to),
			Self::Io(err) => Display::fmt(&err, f),
			Self::Other(err) => Display::fmt(&err, f),
		}

	}
}

impl From<io::Error> for Error {
	#[inline]
	fn from(error: io::Error) -> Self {
		Self::Io(error)
	}
}
