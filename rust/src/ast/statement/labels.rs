use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct ComeFrom(String);

#[derive(Debug)]
pub struct Label(String);

impl Parsable for Label {
	const TYPE_NAME: &'static str = "<label>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		match parser.guard(TokenKind::Identifier)? {
			Some(Token::Identifier(label)) if parser._hack_is_next_token_colon() => Ok(Some(Self(label))) ,
			Some(Token::Identifier(_)) => {
				parser.undo_next_token();
				Ok(None)
			},
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}

impl Parsable for ComeFrom {
	const TYPE_NAME: &'static str = Keyword::Whence.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Whence))?.is_none() {
			return Ok(None);
		}

		Ok(Some(Self(parser.expect_identifier()?)))
	}
}

impl Compilable for Label {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); unimplemented!();
	}
}

impl Compilable for ComeFrom {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); unimplemented!();
	}
}
