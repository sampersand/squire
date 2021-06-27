use crate::ast::expression::{Expression, Primary};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

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
}

impl Compilable for Index {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;

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
