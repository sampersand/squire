use crate::parse::Error as ParseError;
use squire_runtime::value::Value;
use squire_runtime::value::form::AlreadyDefinedError;
use std::fmt::{self, Display, Formatter};
use std::error::Error as ErrorTrait;

#[derive(Debug)]
#[non_exhaustive]
pub enum Error {
	Parse(ParseError),
	UndeclaredIdentifier(String),
	InvalidLhsForAssignment,
	RenownedAlreadyDefined(String),
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

impl Display for Error {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		match self {
			Self::Parse(err) => Display::fmt(err, f),
			Self::FormBuildError(err) => Display::fmt(err, f),
			Self::UndeclaredIdentifier(ident) => write!(f, "undeclared identifier '{}' encountered", ident),
			Self::InvalidLhsForAssignment => write!(f, "invalid LHS for assignment encountered"),
			Self::RenownedAlreadyDefined(name) => write!(f, "renowned variable '{}' already defined", name),
			Self::LabelAlreadyDefined(label) => write!(f, "label '{}' was already defined", label),
			Self::ParentNotAForm(parent) => write!(f, "parent '{:?}' is not a form", parent),
			Self::ParentNotDeclared(parent) => write!(f, "parent '{}' hasn't been declared yet", parent),
		}
	}
}

impl ErrorTrait for Error {
	fn cause(&self) -> Option<&(dyn ErrorTrait + 'static)> {
		match self {
			Self::Parse(err) => Some(err),
			Self::FormBuildError(err) => Some(err),
			_ => None
		}
	}
}