use crate::ast::{Expression, Statements};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use crate::runtime::Opcode;

#[derive(Debug)]
pub struct Whilst {
	condition: Expression,
	body: Statements,
}

impl Parsable for Whilst {
	const TYPE_NAME: &'static str = Keyword::While.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::While))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		let body = Statements::expect_parse(parser)?;

		Ok(Some(Self { condition, body }))
	}
}

impl Compilable for Whilst {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let start_of_condition = compiler.current_pos();

		let condition_target = target.unwrap_or(Compiler::SCRATCH_TARGET);
		self.condition.compile(compiler, Some(condition_target))?;

		compiler.opcode(Opcode::JumpIfFalse);
		compiler.target(condition_target);
		let end_of_while_loop = compiler.defer_jump();

		self.body.compile(compiler, None)?;
		compiler.opcode(Opcode::Jump);
		compiler.jump_to(start_of_condition);

		end_of_while_loop.set_jump_to_current(compiler);
		Ok(())
	}
}
