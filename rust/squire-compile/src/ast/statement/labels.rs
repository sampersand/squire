use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Symbol, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Whence(String);

#[derive(Debug)]
pub struct Thence(String);

#[derive(Debug)]
pub struct Label(String);

impl Parsable for Label {
	const TYPE_NAME: &'static str = "<label>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		match parser.guard(TokenKind::Identifier)? {
			Some(Token::Identifier(label)) if parser._hack_is_next_token_colon() => {
				parser.expect(TokenKind::Symbol(Symbol::Colon)).expect("we verified this worked via `_hack_is_next_token_colon`");
				Ok(Some(Self(label)))
			},
			Some(Token::Identifier(_)) => {
				parser.undo_next_token();
				Ok(None)
			},
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}

impl Parsable for Whence {
	const TYPE_NAME: &'static str = Keyword::Whence.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Whence))?.is_none() {
			return Ok(None);
		}

		Ok(Some(Self(parser.expect_identifier()?)))
	}
}

impl Parsable for Thence {
	const TYPE_NAME: &'static str = Keyword::Thence.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Thence))?.is_none() {
			return Ok(None);
		}

		Ok(Some(Self(parser.expect_identifier()?)))
	}
}

impl Compilable for Label {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		debug_assert_eq!(target, None);
		let _ = target;

		compiler.get_label(self.0).declare(compiler)
	}
}

impl Compilable for Whence {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		debug_assert_eq!(target, None);
		let _ = target;

		compiler.get_label(self.0).whence(compiler);

		Ok(())
	}
}

impl Compilable for Thence {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {		
		debug_assert_eq!(target, None);
		let _ = target;

		compiler.get_label(self.0).thence(compiler);

		Ok(())
	}
}
