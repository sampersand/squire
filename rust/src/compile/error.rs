use crate::parse::Error as ParseError;
use crate::value::Value;

#[derive(Debug)]
#[non_exhaustive]
pub enum Error {
	Parse(ParseError),
	UnknownIdentifier(String),
	InvalidLhsForAssignment,
	GlobalAlreadyDefined(String),
	LabelAlreadyDefined(String),
	FormValueAlreadyDefined { name: String, kind: &'static str },
	ParentNotAForm(Value),
	ParentNotDeclared(String),
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<ParseError> for Error {
	#[inline]
	fn from(error: ParseError) -> Self {
		Self::Parse(error)
	}
}