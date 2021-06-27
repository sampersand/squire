use crate::ast::{Statements, Statement};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::ParenKind;
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Grouping(Statements);

impl Parsable for Grouping {
	const TYPE_NAME: &'static str = "<grouping>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		Ok(parser.take_paren_group(ParenKind::Round, Statement::expect_parse)?.map(Self))
	}
}

impl Compilable for Grouping {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		for statement in self.0 {
			statement.compile(compiler, target)?;
		}

		Ok(())
	}
}
