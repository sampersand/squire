use crate::parse::Error as ParseError;
use squire_runtime::value::Value;
use squire_runtime::value::form::AlreadyDefinedError;

#[derive(Debug)]
#[non_exhaustive]
pub enum Error {
	Parse(ParseError),
	UnknownIdentifier(String),
	InvalidLhsForAssignment,
	GlobalAlreadyDefined(String),
	LabelAlreadyDefined(String),
	FormBuildError(AlreadyDefinedError),
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

impl From<AlreadyDefinedError> for Error {
	#[inline]
	fn from(err: AlreadyDefinedError) -> Self {
		Self::FormBuildError(err)
	}
}