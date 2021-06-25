use crate::ast::expression::{Expression, Primary};
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{TokenKind, ParenKind};

#[derive(Debug)]
pub struct Index {
	into: Box<Primary>,
	key: Box<Expression>
}

impl Index {
	pub fn parse_with<I>(into: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		if parser.guard(TokenKind::LeftParen(ParenKind::Square))?.is_none() {
			return Ok(Err(into));
		}

		let key = Expression::expect_parse(parser)?;

		parser.expect(TokenKind::RightParen(ParenKind::Square))?;

		Ok(Ok(Self { into: Box::new(into), key: Box::new(key) }))
	}
}