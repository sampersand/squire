use crate::ast::{Expression, Statements};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct If {
	condition: Expression,
	if_true: Statements,
	if_false: Option<Statements>
}

impl Parsable for If {
	const TYPE_NAME: &'static str = Keyword::If.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::If))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		let if_true = Statements::expect_parse(parser)?;

		let if_false =
			parser
				.guard(TokenKind::Keyword(Keyword::Alas))?
				.map(|_| Statements::expect_parse(parser))
				.transpose()?;

		Ok(Some(Self { condition, if_true, if_false }))
	}
}

impl Compilable for If {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		self.condition.compile(compiler, Some(Compiler::SCRATCH_TARGET))?;
		compiler.opcode(Opcode::JumpIfFalse);
		compiler.target(Compiler::SCRATCH_TARGET);
		let if_false_dst = compiler.defer_jump();

		self.if_true.compile(compiler, target)?;

		if let Some(if_false) = self.if_false {
			compiler.opcode(Opcode::Jump);
			let end = compiler.defer_jump();

			if_false_dst.set_jump_to_current(compiler);
			if_false.compile(compiler, target)?;
			end.set_jump_to_current(compiler);
		} else {
			if_false_dst.set_jump_to_current(compiler);
		}

		Ok(())
	}
}