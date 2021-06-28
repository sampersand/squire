use crate::ast::expression::{Expression, Primary};
use crate::parse::{Error as ParseError, Parser};
use crate::parse::token::{TokenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use crate::runtime::Opcode;

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

		let attribute = parser.expect_identifier()?;

		Ok(Ok(Self { from: Box::new(from), attribute }))
	}

	pub fn compile_assignment(
		self,
		op: Option<crate::ast::expression::binary_operator::Math>,
		rhs: Box<Expression>,
		compiler: &mut Compiler,
		target: Option<Target>
	) -> Result<(), CompileError> {
		if op.is_some() { todo!(); }

		let lhs_target = target.unwrap_or_else(|| compiler.next_target());
		self.from.compile(compiler, Some(lhs_target))?;
		let rhs_target = compiler.next_target();
		rhs.compile(compiler, Some(rhs_target))?;
		let constant = compiler.get_constant(self.attribute.into());

		compiler.opcode(Opcode::SetAttribute);
		compiler.target(lhs_target);
		compiler.constant(constant);
		compiler.target(rhs_target);
		compiler.target(target.unwrap_or(lhs_target));

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
