use squire_runtime::value::numeral::NumeralParseError;
use super::{Token, TokenKind};
use std::fmt::{self, Display, Formatter};
use std::error::Error as ErrorTrait;

#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum ErrorKind {
	BadNumeral(NumeralParseError),
	BadFrakturSuffix(char),
	UnterminatedEscapeSequence,
	InvalidHexDigit(char),
	InvalidHexEscape(u32),
	UnknownEscapeCharacter(char),
	UnknownMacroInvocation(String),
	UnknownTokenStart(char),
	MissingRequiredAst(&'static str),
	BadToken { given: Option<Token>, expected: Vec<TokenKind> },
	Message(&'static str)
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
	pub lineno: usize,
	pub file: Option<String>,
	pub kind: ErrorKind
}

pub type Result<T> = std::result::Result<T, Error>;

impl From<NumeralParseError> for ErrorKind {
	#[inline]
	fn from(error: NumeralParseError) -> Self {
		Self::BadNumeral(error)
	}
}

impl From<&'static str> for ErrorKind {
	#[inline]
	fn from(msg: &'static str) -> Self {
		Self::Message(msg)
	}
}

impl ErrorTrait for Error {
	fn cause(&self) -> Option<&(dyn ErrorTrait + 'static)> {
		match &self.kind {
			ErrorKind::BadNumeral(err) => Some(err),
			_ => None
		}
	}
}
impl Display for Error {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		write!(f, "{}:{}: {}", self.file.as_deref().unwrap_or("<eval>"), self.lineno, self.kind)
	}
}

impl Display for ErrorKind {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		match self {
			Self::BadNumeral(err) => Display::fmt(err, f),
			Self::BadFrakturSuffix(suffix) => write!(f, "bad suffix encountered on fraktur literal: {:?}", suffix),
			Self::UnterminatedEscapeSequence => write!(f, "unterminated escape sequence encountered"),
			Self::InvalidHexDigit(digit) => write!(f, "invalid hex digit {:?}", digit),
			Self::InvalidHexEscape(escape) => write!(f, "invalid hex escape '\\u{:x}'", escape),
			Self::UnknownEscapeCharacter(chr) => write!(f, "unknown escape character {:?}", chr),
			Self::UnknownMacroInvocation(kind) => write!(f, "unknown macro kind '{}'", kind),
			Self::UnknownTokenStart(chr) => write!(f, "unknown character {:?} encountered", chr),
			Self::MissingRequiredAst(kind) => write!(f, "an {} was expected, but not found", kind),
			Self::BadToken { given, expected } => {
				write!(f, "expected one of {:?}, but was given ", expected)?;
				if let Some(given) = given {
					write!(f, "{:?}", given)
				} else {
					f.write_str("<EOF>")
				}
			},
			Self::Message(msg) => f.write_str(msg),
		}
	}
}