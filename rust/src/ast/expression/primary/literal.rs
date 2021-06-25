use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{Token, TokenKind, Literal as TokenLiteral};

#[derive(Debug)]
pub struct Literal(TokenLiteral);

impl Parsable for Literal {
	const TYPE_NAME: &'static str = "<literal>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		match parser.guard(TokenKind::Literal)? {
			Some(Token::Literal(literal)) => Ok(Some(Self(literal))),
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}
