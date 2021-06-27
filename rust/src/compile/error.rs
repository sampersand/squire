use crate::parse::Error as ParseError;

#[derive(Debug)]
pub enum Error {
	Parse(ParseError),
	UnknownIdentifier(String)
}

pub type Result<T> = std::result::Result<T, Error>;


impl From<ParseError> for Error {
	#[inline]
	fn from(error: ParseError) -> Self {
		Self::Parse(error)
	}
}