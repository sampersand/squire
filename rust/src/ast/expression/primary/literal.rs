use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, LiteralKind, Literal as TokenLiteral};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Literal(TokenLiteral);

impl Parsable for Literal {
	const TYPE_NAME: &'static str = "<literal>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		// unimplemented: text interpolation...?

		match parser.guard(TokenKind::Literal(LiteralKind::Any))? {
			Some(Token::Literal(literal)) => Ok(Some(Self(literal))),
			Some(_) => unreachable!(),
			None => Ok(None)
		}
	}
}

impl Compilable for Literal {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;
		use crate::value::Value;

		if let TokenLiteral::TextInterpolation(groups, end) = self.0 {
			let /*mut*/ targets = Vec::with_capacity(groups.len());

			for (prefix, expr) in groups {
				let dst = compiler.next_target();
				let _ = (prefix, expr, dst);
				// expr.compile(compiler, target);
				// targets.push((compiler.get_constant(prefix), dst));
				todo!();
			}

			compiler.opcode(Opcode::Interpolate);
			compiler.count(targets.len());

			for (prefix, target) in targets {
				compiler.constant(prefix);
				compiler.target(target);
			}

			let constant = compiler.get_constant(end.into());
			compiler.constant(constant);
			let target = target.unwrap_or_else(|| compiler.next_target()); // not strictly necessary
			compiler.target(target);
			return Ok(());
		}

		if target.is_none() {
			return Ok(());
		}

		let constant_index = 
			match self.0 {
				TokenLiteral::Ni => compiler.get_constant(Value::Ni),
				TokenLiteral::Boolean(boolean) => compiler.get_constant(Value::Veracity(boolean)),
				TokenLiteral::Numeral(numeral) => compiler.get_constant(Value::Numeral(numeral)),
				TokenLiteral::Text(text) => compiler.get_constant(Value::Text(text)),
				TokenLiteral::TextInterpolation(_, _) => unreachable!()
			};

		if let Some(target) = target {
			compiler.opcode(Opcode::LoadConstant);
			compiler.constant(constant_index);
			compiler.target(target);
		}

		Ok(())
	}
}
