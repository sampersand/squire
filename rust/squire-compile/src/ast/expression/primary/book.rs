use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
enum PageKind {
	Normal(Expression),
	Splat(Expression)
}

#[derive(Debug)]
pub struct Book(Vec<PageKind>);

impl Parsable for Book {
	const TYPE_NAME: &'static str = "<book>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Square))?.is_none() {
			return Ok(None);
		}

		let book = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Square), |parser| {
				if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
					Ok(PageKind::Splat(Expression::expect_parse(parser)?))
				} else {
					Ok(PageKind::Normal(Expression::expect_parse(parser)?))
				}
			})?;

		Ok(Some(Self(book)))
	}
}

impl Compilable for Book {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use squire_runtime::vm::{Opcode, Interrupt};

		let target =
			if let Some(target) = target {
				target
			} else {
				for page in self.0 {
					match page {
						PageKind::Normal(ele) => ele.compile(compiler, None)?,
						PageKind::Splat(splat) => splat.compile(compiler, None)?
					}
				}

				return Ok(());
			};

		let mut elements = Vec::new();

		for page in self.0 {
			match page {
				PageKind::Normal(ele) => {
					let element_target = compiler.next_target();
					elements.push(element_target);
					ele.compile(compiler, Some(element_target))?;
				},

				PageKind::Splat(splat) => {
					let _ = splat; todo!();
				}
			}
		}

		compiler.opcode(Opcode::Interrupt);
		compiler.interrupt(Interrupt::NewArray);
		compiler.count(elements.len());

		for page in elements {
			compiler.target(page);
		}

		compiler.target(target);

		Ok(())
	}
}
