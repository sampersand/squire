use crate::ast::Expression;
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{TokenKind, ParenKind, Symbol};


#[derive(Debug)]
enum CodexPageKind {
	Normal(Expression, Expression),
	SplatSplat(Expression)
}

#[derive(Debug)]
pub struct Codex(Vec<CodexPageKind>);

impl Parsable for Codex {
	const TYPE_NAME: &'static str = "<codex>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Curly))?.is_none() {
			return Ok(None);
		}

		let codex = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Curly), |parser| {
				if parser.guard(TokenKind::Symbol(Symbol::AsteriskAsterisk))?.is_some() {
					return Ok(CodexPageKind::SplatSplat(Expression::expect_parse(parser)?));
				}

				let key = Expression::expect_parse(parser)?;
				parser.expect(TokenKind::Symbol(Symbol::Colon))?;
				let value = Expression::expect_parse(parser)?;

				Ok(CodexPageKind::Normal(key, value))
			})?;

		Ok(Some(Self(codex)))
	}
}

