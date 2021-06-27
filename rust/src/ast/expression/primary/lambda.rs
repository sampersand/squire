use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::ast::Statements;
use crate::ast::statement::function::Arguments;
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Lambda {
	args: Box<Arguments>,
	body: Statements
}

impl Parsable for Lambda {
	const TYPE_NAME: &'static str = "<lambda>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Function))?.is_none() {
			return Ok(None);
		}

		let args = Arguments::expect_parse(parser)?;
		let body = Statements::expect_parse(parser)?;

		Ok(Some(Self { args: Box::new(args), body }))
	}
}

impl Compilable for Lambda {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (target, compiler); todo!();
	}
}
