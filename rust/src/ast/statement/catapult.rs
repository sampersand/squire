use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};


#[derive(Debug)]
pub struct Catapult {
	what: Expression,
}

impl Parsable for Catapult {
	const TYPE_NAME: &'static str = Keyword::Throw.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Throw))?.is_none() {
			return Ok(None);
		}

		let what = Expression::expect_parse(parser)?;

		Ok(Some(Self { what }))
	}
}