use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::ast::statement::journey::Journey;
use crate::parse::token::{TokenKind, Token, Keyword, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::value::Value;
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct Lambda(Box<Journey>);

impl Parsable for Lambda {
	const TYPE_NAME: &'static str = "<lambda>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let has_patterns =
			match parser.guard([TokenKind::Keyword(Keyword::Journey), TokenKind::Symbol(Symbol::BackSlash)])? {
				Some(Token::Keyword(Keyword::Journey)) => true,
				Some(Token::Symbol(Symbol::BackSlash)) => false,
				Some(_) => unreachable!(),
				None => return Ok(None)
			};

		Ok(Some(Self(Journey::parse_without_keyword(parser, None, false, has_patterns)?.into())))
	}
}

impl Compilable for Lambda {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let journey = Value::Journey(self.0.build_journey(compiler.globals().clone())?.into());
		
		if let Some(target) = target {
			let constant_index = compiler.get_constant(journey);
			compiler.opcode(Opcode::LoadConstant);
			compiler.constant(constant_index);
			compiler.target(target)
		} else {
			warn!("no target for lambda");
		}

		Ok(())
	}
}
