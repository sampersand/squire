use crate::ast::expression::Primary;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Symbol};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub enum UnaryOperator {
	Pos,
	Neg,
	Not
}

#[derive(Debug)]
pub struct UnaryOp {
	op: UnaryOperator,
	expr: Box<Primary>
}

impl Parsable for UnaryOp {
	const TYPE_NAME: &'static str = "<unary op>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		const UNARY_TOKEN_KINDS: [TokenKind; 3] = [
			TokenKind::Symbol(Symbol::Plus),
			TokenKind::Symbol(Symbol::Hyphen),
			TokenKind::Symbol(Symbol::Exclamation)
		];

		let op = 
			match parser.guard(UNARY_TOKEN_KINDS)? {
				Some(Token::Symbol(Symbol::Plus)) => UnaryOperator::Pos,
				Some(Token::Symbol(Symbol::Hyphen)) => UnaryOperator::Neg,
				Some(Token::Symbol(Symbol::Exclamation)) => UnaryOperator::Not,
				Some(_) => unreachable!(),
				None => return Ok(None)
			};

		let expr = Primary::expect_parse(parser)?;

		Ok(Some(Self { op, expr: Box::new(expr) }))
	}
}

impl Compilable for UnaryOp {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use squire_runtime::vm::Opcode;

		self.expr.compile(compiler, target)?;

		if let Some(target) = target {
			match self.op {
				UnaryOperator::Pos => compiler.opcode(Opcode::Pos),
				UnaryOperator::Neg => compiler.opcode(Opcode::Negate),
				UnaryOperator::Not => compiler.opcode(Opcode::Not),
			}

			compiler.target(target); // argument
			compiler.target(target); // destination
		}

		Ok(())
	}
}
