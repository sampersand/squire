use crate::ast::expression::{Expression, Primary};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct Index {
	into: Box<Primary>,
	key: Box<Expression>
}

impl Index {
	pub fn parse_with<I>(into: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		if parser.guard(TokenKind::LeftParen(ParenKind::Square))?.is_none() {
			return Ok(Err(into));
		}

		let key = Expression::expect_parse(parser)?;

		parser.expect(TokenKind::RightParen(ParenKind::Square))?;

		Ok(Ok(Self { into: Box::new(into), key: Box::new(key) }))
	}

	pub fn compile_assignment(
		self,
		op: Option<crate::ast::expression::binary_operator::Math>,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		if op.is_some() { todo!(); }

		let into_target = compiler.next_target();
		let key_target = compiler.next_target();
		let rhs_target = target.unwrap_or(Compiler::SCRATCH_TARGET);

		self.into.compile(compiler, Some(into_target))?;
		self.key.compile(compiler, Some(key_target))?;
		rhs.compile(compiler, Some(rhs_target))?;

		compiler.opcode(Opcode::IndexAssign);
		compiler.target(into_target);
		compiler.target(key_target);
		compiler.target(rhs_target);

		Ok(())
	}
}

impl Compilable for Index {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let target =
			if let Some(target) = target {
				target
			} else {
				self.into.compile(compiler, None)?;
				self.key.compile(compiler, None)?;
				return Ok(());
			};

		let key_index = compiler.next_target();

		self.into.compile(compiler, Some(target))?;
		self.key.compile(compiler, Some(key_index))?;

		compiler.opcode(Opcode::Index);
		compiler.target(target);
		compiler.target(key_index);
		compiler.target(target);

		Ok(())
	}
}
