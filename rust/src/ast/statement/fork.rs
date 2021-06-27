use crate::ast::{Expression, Statements, Statement};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Fork {
	condition: Expression,
	paths: Vec<Path>,
	alas: Option<Statements>
}

#[derive(Debug)]
struct Path {
	exprs: Vec<Expression>, // should have at least one
	body: Statements

}

impl Fork {
	fn parse_paths<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Path, ParseError> {
		let mut exprs = vec![Expression::expect_parse(parser)?];
		parser.expect(TokenKind::Symbol(Symbol::Colon))?;

		while parser.guard(TokenKind::Keyword(Keyword::Case))?.is_some() {
			exprs.push(Expression::expect_parse(parser)?);
			parser.expect(TokenKind::Symbol(Symbol::Colon))?;
		}

		Ok(Path { exprs, body: parser.collect(Statement::parse)? })
	}

}

impl Parsable for Fork {
	const TYPE_NAME: &'static str = Keyword::Switch.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Switch))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		parser.expect(TokenKind::LeftParen(ParenKind::Curly))?;

		let mut paths = Vec::new();
		let mut alas = None;

		while parser.guard(TokenKind::RightParen(ParenKind::Curly))?.is_none() {
			match parser.expect([TokenKind::Keyword(Keyword::Case), TokenKind::Keyword(Keyword::Alas)])? {
				Token::Keyword(Keyword::Alas) if alas.is_some() => return Err(parser.error("an `alas` was already given")),
				Token::Keyword(Keyword::Alas) => {
					parser.expect(TokenKind::Symbol(Symbol::Colon))?;
					alas = Some(parser.collect(Statement::parse)?)
				},
				Token::Keyword(Keyword::Case) => paths.push(Self::parse_paths(parser)?),
				_ => unreachable!()
			}
		}

		Ok(Some(Self { condition, paths, alas }))
	}
}

impl Compilable for Fork {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); todo!();
	}
}
