use crate::ast::expression::{Expression, Primary};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, ParenKind, Symbol, Token};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
enum Argument {
	Positional(Expression),
	Splat(Expression),
	Keyword(String, Expression),
	SplatSplat(Expression)
}

#[derive(Debug)]
pub struct FunctionCall {
	func: Box<Primary>,
	args: Vec<Argument>
}

impl FunctionCall {
	pub fn parse_with<I>(func: Primary, parser: &mut Parser<'_, I>) -> Result<Result<Self, Primary>, ParseError>
	where
		I: Iterator<Item=char>
	{
		if parser.guard(TokenKind::LeftParen(ParenKind::Round))?.is_none() {
			return Ok(Err(func));
		}

		let args = 
			parser.take_separated(TokenKind::Symbol(Symbol::Comma), TokenKind::RightParen(ParenKind::Round), |parser| {
				match parser.guard([
					TokenKind::Symbol(Symbol::AsteriskAsterisk),
					TokenKind::Symbol(Symbol::Asterisk),
					TokenKind::Identifier,
				])? {
					Some(Token::Symbol(Symbol::Asterisk)) =>
						return Ok(Argument::Splat(Expression::expect_parse(parser)?)),

					Some(Token::Symbol(Symbol::AsteriskAsterisk)) =>
						return Ok(Argument::SplatSplat(Expression::expect_parse(parser)?)),

					Some(Token::Identifier(_)) if !parser._hack_is_next_token_colon() => {
						parser.undo_next_token();
						Expression::expect_parse(parser).map(Argument::Positional)
					},

					Some(Token::Identifier(ident)) => {
						parser.expect(TokenKind::Symbol(Symbol::Colon))?;
						let value = Expression::expect_parse(parser)?;
						return Ok(Argument::Keyword(ident, value));
					},
					Some(_) => unreachable!(),
					None => Expression::expect_parse(parser).map(Argument::Positional)
				}
			})?;

		Ok(Ok(Self { func: Box::new(func), args }))
	}
}

impl Compilable for FunctionCall {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use squire_runtime::vm::Opcode;

		let func_target = target.unwrap_or_else(|| compiler.next_target());
		self.func.compile(compiler, Some(func_target))?;

		let mut args = Vec::with_capacity(self.args.len());
		for arg in self.args {
			let arg_target = compiler.next_target();
			args.push(arg_target);

			match arg {
				Argument::Positional(arg) => arg.compile(compiler, Some(arg_target))?,
				_ => todo!()
			}
		}

		compiler.opcode(Opcode::Call);
		compiler.target(func_target);
		compiler.count(args.len());

		for arg in args {
			compiler.target(arg);
		}

			compiler.target(func_target);

		Ok(())
	}
}
