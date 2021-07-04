use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use crate::runtime::Opcode;
use crate::value::Value;

#[derive(Debug)]
pub struct Reward {
	what: Option<Expression>,
}

impl Parsable for Reward {
	const TYPE_NAME: &'static str = Keyword::Reward.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Reward))?.is_none() {
			return Ok(None);
		}

		let what = Expression::parse(parser)?;

		Ok(Some(Self { what }))
	}
}

impl Compilable for Reward {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let target = target.unwrap_or(Compiler::SCRATCH_TARGET);

		if let Some(what) = self.what {
			what.compile(compiler, Some(target))?;
		} else {
			let null = compiler.get_constant(Value::Ni);
			compiler.opcode(Opcode::LoadConstant);
			compiler.constant(null);
			compiler.target(target);
		}

		compiler.opcode(Opcode::Return);
		compiler.target(target);

		Ok(())
	}
}
