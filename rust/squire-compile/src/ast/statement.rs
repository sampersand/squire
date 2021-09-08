use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Symbol, ParenKind};
use crate::ast::Expression;
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

mod form;
pub mod journey;
mod attempt;
mod catapult;
mod reward;
mod scope;
mod r#if;
mod whilst;
mod fork;
mod labels;
mod genus_declaration;

use genus_declaration::GenusDeclaration;
pub use form::Form;
pub use journey::Journey;
pub use attempt::Attempt;
pub use catapult::Catapult;
pub use reward::Reward;
pub use scope::{Renowned, Nigh};
pub use r#if::If;
pub use whilst::Whilst;
pub use fork::Fork;
pub use labels::{Label, Whence, Thence};

pub type Statements = Vec<Statement>;

#[derive(Debug)]
#[non_exhaustive]
pub enum Statement {
	Form(Form),
	Journey(Journey),
	Attempt(Attempt),
	Catapult(Catapult),
	Reward(Reward),

	Renowned(Renowned),
	Nigh(Nigh),

	If(If),
	Whilst(Whilst),
	Fork(Fork),
	Label(Label),
	Whence(Whence),
	Thence(Thence),

	Expression(Expression),
}

impl Parsable for Statements {
	const TYPE_NAME: &'static str = "Statements";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Curly))?.is_none() {
			return Ok(None);
		}

		let mut statements = Vec::new();

		while parser.guard(TokenKind::RightParen(ParenKind::Curly))?.is_none() {
			// ignore empty statements; this is a special case for statgements, and why we cant use `parse_grouped`
			if parser.guard(TokenKind::Symbol(Symbol::Endline))?.is_some() {
				continue;
			}
	
			statements.push(Statement::expect_parse(parser)?);

			// match parser.expect([TokenKind::Symbol(Symbol::Endline), TokenKind::RightParen(ParenKind::Curly)])? {
			// 	Token::Symbol(Symbol::Endline) => continue,
			// 	Token::RightParen(ParenKind::Curly) => break,
			// 	_ => unreachable!()
			// }
		}

		Ok(Some(statements))
	}
}

impl Parsable for Statement {
	const TYPE_NAME: &'static str = "Statement";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		while parser.guard(TokenKind::Symbol(Symbol::Endline))?.is_some() {
			// strip leading `;`s.
		}
				
		macro_rules! try_parse {
			($($name:ident),*) => (
				$(if let Some(token) = $name::parse(parser)? {
					Ok(Some(Self::$name(token)))
				} else)* {
					Ok(None)
				}
			);
		}

		let stmt = try_parse!(
			Form, Journey, Attempt, Catapult, Reward,
			Renowned, Nigh,
			If, Whilst, Fork,
			Label, Whence, Thence,
			Expression
		);

		while parser.guard(TokenKind::Symbol(Symbol::Endline))?.is_some() {
			// strip trailing `;`s
		}

		stmt
	}
}

impl Compilable for Statements {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		for statement in self {
			statement.compile(compiler, target)?;
		}

		Ok(())
	}
}

impl Compilable for Statement {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		match self {
			Self::Form(form) => form.compile(compiler, target),
			Self::Journey(journey) => journey.compile(compiler, target),
			Self::Attempt(attempt) => attempt.compile(compiler, target),
			Self::Catapult(catapult) => catapult.compile(compiler, target),
			Self::Reward(reward) => reward.compile(compiler, target),

			Self::Renowned(renowned) => renowned.compile(compiler, target),
			Self::Nigh(nigh) => nigh.compile(compiler, target),

			Self::If(r#if) => r#if.compile(compiler, target),
			Self::Whilst(whilst) => whilst.compile(compiler, target),
			Self::Fork(fork) => fork.compile(compiler, target),
			Self::Label(label) => label.compile(compiler, target),
			Self::Whence(whence) => whence.compile(compiler, target),
			Self::Thence(thence) => thence.compile(compiler, target),

			Self::Expression(expression) => expression.compile(compiler, target),
		}
	}
}