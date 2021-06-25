use crate::ast::Expression;
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{TokenKind, ParenKind, Symbol};


#[derive(Debug)]
enum ArrayEleKind {
	Normal(Expression),
	Splat(Expression)
}

#[derive(Debug)]
pub struct Array(Vec<ArrayEleKind>);

impl Parsable for Array {
	const TYPE_NAME: &'static str = "<array>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Square))?.is_none() {
			return Ok(None);
		}

		let array = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Square), |parser| {
				if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
					Ok(ArrayEleKind::Splat(Expression::expect_parse(parser)?))
				} else {
					Ok(ArrayEleKind::Normal(Expression::expect_parse(parser)?))
				}
			})?;

		Ok(Some(Self(array)))
	}
}

