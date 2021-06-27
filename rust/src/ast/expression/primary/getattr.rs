use crate::ast::expression::Primary;
use crate::parse::{Error as ParseError, Parser};
use crate::parse::token::{TokenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

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
}

impl Compilable for GetAttr {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;

		self.from.compile(compiler, target)?;

		if let Some(target) = target {
			let attribute_index = compiler.get_constant(self.attribute.into());

			compiler.opcode(Opcode::GetAttribute);
			compiler.target(target);
			compiler.constant(attribute_index);
		}

		Ok(())
	}
}
