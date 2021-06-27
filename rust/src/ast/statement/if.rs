use crate::ast::{Expression, Statements};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};


#[derive(Debug)]
pub struct If {
	condition: Expression,
	if_true: Statements,
	if_false: Option<Statements>
}

impl Parsable for If {
	const TYPE_NAME: &'static str = Keyword::If.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::If))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		let if_true = Statements::expect_parse(parser)?;

		let if_false =
			parser
				.guard(TokenKind::Keyword(Keyword::Alas))?
				.map(|_| Statements::expect_parse(parser))
				.transpose()?;

		Ok(Some(Self { condition, if_true, if_false }))
	}
}