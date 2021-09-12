use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::ast::statement::journey::Journey;
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Lambda(Box<Journey>);

impl Parsable for Lambda {
	const TYPE_NAME: &'static str = "<lambda>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Journey))?.is_none() {
			return Ok(None);
		}

		Ok(Some(Self(Journey::parse_without_keyword(parser, "<lambda>".to_string(), false)?.into())))
	}
}

impl Compilable for Lambda {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		// self.0.compile(compiler, target);
		let _ = (target, compiler); unimplemented!();
		// 	builder.add_recall(func.build_journey(globals.clone(), false)?)?;
	}
}
