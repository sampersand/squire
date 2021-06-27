use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Symbol, ParenKind};
use crate::ast::Expression;
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

mod class;
pub mod function;
mod attempt;
mod catapult;
mod reward;
mod scope;
mod r#if;
mod whilst;
mod fork;
mod labels;

pub use class::Class;
pub use function::Function;
pub use attempt::Attempt;
pub use catapult::Catapult;
pub use reward::Reward;
pub use scope::{Renowned, Nigh};
pub use r#if::If;
pub use whilst::Whilst;
pub use fork::Fork;
pub use labels::{Label, ComeFrom};

pub type Statements = Vec<Statement>;

#[derive(Debug)]
#[non_exhaustive]
pub enum Statement {
	Class(Class),
	Function(Function),
	Attempt(Attempt),
	Catapult(Catapult),
	Reward(Reward),

	Renowned(Renowned),
	Nigh(Nigh),

	If(If),
	Whilst(Whilst),
	Fork(Fork),
	Label(Label),
	ComeFrom(ComeFrom),

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

			match parser.expect([TokenKind::Symbol(Symbol::Endline), TokenKind::RightParen(ParenKind::Curly)])? {
				Token::Symbol(Symbol::Endline) => continue,
				Token::RightParen(ParenKind::Curly) => break,
				_ => unreachable!()
			}
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
			Class, Function,  Attempt, Catapult, Reward,
			Renowned, Nigh,
			If, Whilst, Fork, /* Label, ComeFrom,*/
			Expression
		);

		while parser.guard(TokenKind::Symbol(Symbol::Endline))?.is_some() {
			// strip trailing `;`s
		}

		stmt
	}
}

impl Compilable for Statement {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); todo!()
	}
}