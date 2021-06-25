use crate::ast::Expression;
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{TokenKind, Keyword, Symbol};

#[derive(Debug)]
pub struct Renowned {
	name: String,
	init: Option<Expression>
}

#[derive(Debug)]
pub struct Nigh {
	name: String,
	init: Option<Expression>
}

impl Parsable for Renowned {
	const TYPE_NAME: &'static str = Keyword::Global.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Global))?.is_none() {
			return Ok(None);
		}

		let name = parser.expect_identifier()?;
		let init =
			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				Some(Expression::expect_parse(parser)?)
			} else {
				None
			};

		Ok(Some(Self { name, init }))
	}
}

impl Parsable for Nigh {
	const TYPE_NAME: &'static str = Keyword::Local.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Local))?.is_none() {
			return Ok(None);
		}

		let name = parser.expect_identifier()?;
		let init =
			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				Some(Expression::expect_parse(parser)?)
			} else {
				None
			};

		Ok(Some(Self { name, init }))
	}
}
