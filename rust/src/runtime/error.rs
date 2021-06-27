use crate::value::ValueKind;
use crate::value::numeral::NumeralParseError;

#[derive(Debug)]
pub enum Error {
	OperationNotSupported { kind: ValueKind, func: &'static str },
	InvalidOperand { kind: ValueKind, func: &'static str },
	ValueError(String),
	DivisionByZero,
	OutOfBounds,
	Other(Box<dyn std::error::Error>)
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<NumeralParseError> for Error {
	#[inline]
	fn from(error: NumeralParseError) -> Self {
		Self::Other(Box::new(error))
	}
}
