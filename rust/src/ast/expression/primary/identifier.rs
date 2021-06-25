use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{Token, TokenKind};

#[derive(Debug)]
pub struct Identifier(String);

impl Parsable for Identifier {
	const TYPE_NAME: &'static str = "<variable>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		match parser.guard(TokenKind::Identifier)? {
			Some(Token::Identifier(identifier)) => Ok(Some(Self(identifier))),
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}
