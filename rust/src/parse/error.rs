use crate::value::numeral::NumeralParseError;

#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum ErrorKind {
	BadNumeral(NumeralParseError)
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
	pub lineno: usize,
	pub file: Option<String>,
	pub error: ErrorKind
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<NumeralParseError> for ErrorKind {
	#[inline]
	fn from(error: NumeralParseError) -> Self {
		Self::BadNumeral(error)
	}
}