use crate::ast::expression::{binary_operator, Expression, Primary};
use crate::parse::{Error as ParseError, Parser};
use crate::parse::token::{TokenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct GetAttr {
	from: Box<Primary>,
	attribute: String
}

impl GetAttr {
	pub fn parse_with<I>(from: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		if parser.guard(TokenKind::Symbol(Symbol::Dot))?.is_none() {
			return Ok(Err(from));
		}

		let attribute = parser.expect_identifier_or_operator()?;

		Ok(Ok(Self { from: Box::new(from), attribute }))
	}
}

impl GetAttr {
	fn compile_compound_assignment(
		self,
		op: binary_operator::Math,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		let from_target = compiler.next_target();
		self.from.compile(compiler, Some(from_target))?;

		// this can probably be combiend with `GetAttr::compile`
		let lhs_target = compiler.next_target();
		let attribute_index = compiler.get_constant(self.attribute.into());

		compiler.opcode(Opcode::GetAttribute);
		compiler.target(from_target);
		compiler.constant(attribute_index);
		compiler.target(lhs_target);

		// compile the RHS.
		let rhs_target = target.unwrap_or_else(|| compiler.next_target());
		rhs.compile(compiler, Some(rhs_target))?;

		compiler.opcode(op.opcode());
		compiler.target(lhs_target);
		compiler.target(rhs_target);
		compiler.target(rhs_target);

		compiler.opcode(Opcode::SetAttribute);
		compiler.target(from_target);
		compiler.constant(attribute_index);
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

		let lhs_target = target.unwrap_or_else(|| compiler.next_target());
		self.from.compile(compiler, Some(lhs_target))?;
		let rhs_target = compiler.next_target();
		rhs.compile(compiler, Some(rhs_target))?;
		let constant = compiler.get_constant(self.attribute.into());

		compiler.opcode(Opcode::SetAttribute);
		compiler.target(lhs_target);
		compiler.constant(constant);
		compiler.target(rhs_target);

		Ok(())
	}
}

impl Compilable for GetAttr {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		self.from.compile(compiler, target)?;

		if let Some(target) = target {
			let attribute_index = compiler.get_constant(self.attribute.into());

			compiler.opcode(Opcode::GetAttribute);
			compiler.target(target);
			compiler.constant(attribute_index);
			compiler.target(target);
		}

		Ok(())
	}
}
