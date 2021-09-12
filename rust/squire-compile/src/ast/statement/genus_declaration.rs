#![allow(unused_imports)]
use crate::ast::{Expression, expression::Primary};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Token, Literal, LiteralKind, ParenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;
use squire_runtime::value::Text;


#[derive(Debug)]
pub struct GenusDeclaration(Primary);

impl Parsable for GenusDeclaration {
	const TYPE_NAME: &'static str = "<genus declaration>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Symbol(Symbol::Colon))?.is_none() {
			return Ok(None);
		}

		Ok(Some(Self(Primary::expect_parse(parser)?)))
	}
}

impl Compilable for GenusDeclaration {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		self.0.compile(compiler, target)
	}
}


impl GenusDeclaration {
	pub fn check(self, target: Target, compiler: &mut Compiler) -> Result<(), CompileError> {
		let genus_target =
			if target == Compiler::SCRATCH_TARGET {
				compiler.next_target()
			} else {
				Compiler::SCRATCH_TARGET
			};

		self.0.compile(compiler, Some(genus_target))?;
		compiler.opcode(Opcode::CheckMatches);
		compiler.target(target);
		compiler.target(genus_target);

		Ok(())
	}
}
