use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
enum ArrayEleKind {
	Normal(Expression),
	Splat(Expression)
}

#[derive(Debug)]
pub struct Array(Vec<ArrayEleKind>);

impl Parsable for Array {
	const TYPE_NAME: &'static str = "<array>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Square))?.is_none() {
			return Ok(None);
		}

		let array = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Square), |parser| {
				if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
					Ok(ArrayEleKind::Splat(Expression::expect_parse(parser)?))
				} else {
					Ok(ArrayEleKind::Normal(Expression::expect_parse(parser)?))
				}
			})?;

		Ok(Some(Self(array)))
	}
}

impl Compilable for Array {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::{Opcode, Interrupt};

		let target =
			if let Some(target) = target {
				target
			} else {
				for element in self.0 {
					match element {
						ArrayEleKind::Normal(ele) => ele.compile(compiler, None)?,
						ArrayEleKind::Splat(splat) => splat.compile(compiler, None)?
					}
				}

				return Ok(());
			};

		let mut elements = Vec::new();

		for element in self.0 {
			match element {
				ArrayEleKind::Normal(ele) => {
					let element_target = compiler.next_target();
					elements.push(element_target);
					ele.compile(compiler, Some(element_target))?;
				},

				ArrayEleKind::Splat(splat) => {
					let _ = splat; todo!();
				}
			}
		}

		compiler.opcode(Opcode::Interrupt);
		compiler.interrupt(Interrupt::NewArray);
		compiler.count(elements.len());

		for element in elements {
			compiler.target(element);
		}

		compiler.target(target);

		Ok(())
	}
}
