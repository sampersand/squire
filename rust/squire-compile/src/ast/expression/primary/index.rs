use crate::ast::expression::{Expression, Primary};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;
use crate::ast::expression::binary_operator;

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

	fn compile_compound_assignment(
		self,
		op: binary_operator::Math,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		let into_target = compiler.next_target();
		let key_target = compiler.next_target();
		let dst_target = compiler.next_target();
		self._compile(compiler, into_target, key_target, dst_target)?;

		// compile the RHS.
		let rhs_target = target.unwrap_or_else(|| compiler.next_target());
		rhs.compile(compiler, Some(rhs_target))?;

		compiler.opcode(op.opcode());
		compiler.target(dst_target);
		compiler.target(rhs_target);
		compiler.target(rhs_target);

		compiler.opcode(Opcode::IndexAssign);
		compiler.target(into_target);
		compiler.target(key_target);
		compiler.target(rhs_target);

		Ok(())
	}

	pub fn compile_assignment(
		self,
		op: Option<binary_operator::Math>,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		if let Some(op) = op {
			return self.compile_compound_assignment(op, rhs, compiler, target);
		}

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

	fn _compile(
		self,
		compiler: &mut Compiler,
		into_target: Target,
		key_target: Target,
		target: Target
	) -> Result<(), CompileError> {
		self.into.compile(compiler, Some(into_target))?;
		self.key.compile(compiler, Some(key_target))?;

		compiler.opcode(Opcode::Index);
		compiler.target(into_target);
		compiler.target(key_target);
		compiler.target(target);

		Ok(())
	}
}

impl Compilable for Index {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let key_target = compiler.next_target();
		let target = target.unwrap_or_else(|| compiler.next_target());

		self._compile(compiler, target, key_target, target)
	}
}
