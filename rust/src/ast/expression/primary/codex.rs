use crate::ast::Expression;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
enum CodexPageKind {
	Normal(Expression, Expression),
	SplatSplat(Expression)
}

#[derive(Debug)]
pub struct Codex(Vec<CodexPageKind>);

impl Parsable for Codex {
	const TYPE_NAME: &'static str = "<codex>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Curly))?.is_none() {
			return Ok(None);
		}

		let codex = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Curly), |parser| {
				if parser.guard(TokenKind::Symbol(Symbol::AsteriskAsterisk))?.is_some() {
					return Ok(CodexPageKind::SplatSplat(Expression::expect_parse(parser)?));
				}

				let key = Expression::expect_parse(parser)?;
				parser.expect(TokenKind::Symbol(Symbol::Colon))?;
				let value = Expression::expect_parse(parser)?;

				Ok(CodexPageKind::Normal(key, value))
			})?;

		Ok(Some(Self(codex)))
	}
}

impl Compilable for Codex {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::{Opcode, Interrupt};

		let target =
			if let Some(target) = target {
				target
			} else {
				for page in self.0 {
					match page {
						CodexPageKind::Normal(key, value) => {
							key.compile(compiler, None)?;
							value.compile(compiler, None)?;
						},
						CodexPageKind::SplatSplat(expr) => expr.compile(compiler, None)?
					}
				}

				return Ok(());
			};

		let mut pages = Vec::new();

		for page in self.0 {
			let page_key_target = compiler.next_target();
			let page_value_target = compiler.next_target();

			match page {
				CodexPageKind::Normal(key, value) => {
					key.compile(compiler, Some(page_key_target))?;
					value.compile(compiler, Some(page_value_target))?;
					pages.push((page_key_target, page_value_target));
				},

				CodexPageKind::SplatSplat(splatsplat) => {
					let _ = splatsplat; todo!();
				}
			}
		}

		compiler.opcode(Opcode::Interrupt);
		compiler.interrupt(Interrupt::NewCodex(pages.len()));

		for (key, value) in pages {
			compiler.target(key);
			compiler.target(value);
		}

		compiler.target(target);

		Ok(())
	}
}
