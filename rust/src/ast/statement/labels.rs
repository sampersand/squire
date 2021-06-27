use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{/*Token, TokenKind,*/ Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct ComeFrom {

}

#[derive(Debug)]
pub struct Label {

}

impl Parsable for Label {
	const TYPE_NAME: &'static str = "<label>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		// match parser.guard(TokenKind::Identifier)? {
		// 	Token::Identifier(label)
		// }
		let _ = parser; todo!();
	}
}

impl Parsable for ComeFrom {
	const TYPE_NAME: &'static str = Keyword::ComeFrom.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let _ = parser; todo!();
	}
}

impl Compilable for Label {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); todo!();
	}
}

impl Compilable for ComeFrom {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); todo!();
	}
}
