use crate::ast::expression::Primary;
use crate::parse::{Error as ParseError, Parser};
use crate::parse::token::{TokenKind, Symbol};

#[derive(Debug)]
pub struct GetAttr {
	from: Box<Primary>,
	value: String
}


impl GetAttr {
	pub fn parse_with<I>(from: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		if parser.guard(TokenKind::Symbol(Symbol::Dot))?.is_none() {
			return Ok(Err(from));
		}

		let value = parser.expect_identifier()?;

		Ok(Ok(Self { from: Box::new(from), value }))
	}
}