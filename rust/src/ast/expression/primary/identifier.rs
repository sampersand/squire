use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Identifier(String);

impl Parsable for Identifier {
	const TYPE_NAME: &'static str = "<variable>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		match parser.guard(TokenKind::Identifier)? {
			Some(Token::Identifier(identifier)) => Ok(Some(Self(identifier))),
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}

impl Identifier {
	pub fn compile_assignment(
		self,
		op: Option<crate::ast::expression::binary_operator::Math>,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		use crate::runtime::Opcode;
		if op.is_some() { todo!(); }

		if let Some((global, _)) = compiler.get_global(&self.0) {
			let rhs_target = target.unwrap_or_else(|| compiler.next_target());
			rhs.compile(compiler, Some(rhs_target))?;

			compiler.opcode(Opcode::StoreGlobal);
			compiler.global(global);
			compiler.target(rhs_target);
		} else {
			let variable = compiler.define_local(self.0);
			rhs.compile(compiler, Some(variable))?;

			if let Some(target) = target {
				if variable != target {
					compiler.opcode(Opcode::Move);
					compiler.target(variable);
					compiler.target(target);
				}
			}
		}

		Ok(())
	}
}

impl Compilable for Identifier {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;

		if let Some(local) = compiler.get_local(&self.0) {
			match target {
				Some(target) if target != local => {
					compiler.opcode(Opcode::Move);
					compiler.target(local);
					compiler.target(target);
				},
				_ => { /* do nothing, as target's either zero or is the local */ },
			}

			return Ok(());
		}

		if let Some((global, _)) = compiler.get_global(&self.0) {
			let target =
				if let Some(target) = target {
					target
				} else {
					return Ok(());
				};

			compiler.opcode(Opcode::LoadGlobal);
			compiler.global(global);
			compiler.target(target);

			return Ok(())
		}

		Err(CompileError::UnknownIdentifier(self.0))
	}
}
