use crate::ast::{Expression, Statements};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};

#[derive(Debug)]
pub struct Whilst {
	condition: Expression,
	body: Statements,
}

impl Parsable for Whilst {
	const TYPE_NAME: &'static str = Keyword::While.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::While))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		let body = Statements::expect_parse(parser)?;

		Ok(Some(Self { condition, body }))
	}
}