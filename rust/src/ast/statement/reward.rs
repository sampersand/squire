use crate::ast::Expression;
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{TokenKind, Keyword};


#[derive(Debug)]
pub struct Reward {
	what: Expression,
}

impl Parsable for Reward {
	const TYPE_NAME: &'static str = Keyword::Return.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Return))?.is_none() {
			return Ok(None);
		}

		let what = Expression::expect_parse(parser)?;

		Ok(Some(Self { what }))
	}
}