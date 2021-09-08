use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct Catapult {
	what: Expression,
}

impl Parsable for Catapult {
	const TYPE_NAME: &'static str = Keyword::Catapult.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Catapult))?.is_none() {
			return Ok(None);
		}

		let what = Expression::expect_parse(parser)?;

		Ok(Some(Self { what }))
	}
}

impl Compilable for Catapult {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let target = target.unwrap_or(Compiler::SCRATCH_TARGET);

		self.what.compile(compiler, Some(target))?;
		compiler.opcode(Opcode::Throw);
		compiler.target(target);

		Ok(())
	}
}
