use crate::parse::Error as ParseError;

#[derive(Debug)]
pub enum Error {
	Parse(ParseError),
	UnknownIdentifier(String),
	InvalidLhsForAssignment,
	GlobalAlreadyDefined(String),
	FormValueAlreadyDefined { name: String, kind: &'static str }
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<ParseError> for Error {
	#[inline]
	fn from(error: ParseError) -> Self {
		Self::Parse(error)
	}
}