use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, Symbol};
use crate::ast::expression::{Expression, Primary};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use std::cmp::Ordering;
use crate::runtime::Opcode;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Math {
	Add, Sub, Mul, Div, Mod, Pow
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Logic {
	Eql, Neq, Lth, Leq, Gth, Geq, Cmp
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ShortCircuit {
	And, Or
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Operator {
	Assignment(Option<Math>),
	Math(Math),
	Logic(Logic),
	ShortCircuit(ShortCircuit),
}

impl PartialOrd for Operator {
	fn partial_cmp(&self, rhs: &Self) -> Option<Ordering> {
		Some(self.cmp(rhs))
	}
}

impl Ord for Operator {
	fn cmp(&self, rhs: &Self) -> Ordering {
		match (self, rhs) {
			(Self::Assignment(_), Self::Assignment(_)) => Ordering::Equal,
			(Self::Assignment(_), _) => Ordering::Greater,
			(_, Self::Assignment(_)) => Ordering::Less,

			(Self::ShortCircuit(_), Self::ShortCircuit(_)) => Ordering::Equal,
			(Self::ShortCircuit(_), _) => Ordering::Greater,
			(_, Self::ShortCircuit(_)) => Ordering::Less,

			(Self::Logic(_), Self::Logic(_)) => Ordering::Equal,
			(Self::Logic(_), _) => Ordering::Greater,
			(_, Self::Logic(_)) => Ordering::Less,

			(Self::Math(Math::Add | Math::Sub), Self::Math(Math::Add | Math::Sub)) => Ordering::Equal,
			(Self::Math(Math::Add | Math::Sub), _ ) => Ordering::Greater,
			(_, Self::Math(Math::Add | Math::Sub)) => Ordering::Less,

			(Self::Math(Math::Mul | Math::Div | Math::Mod), Self::Math(Math::Mul | Math::Div | Math::Mod)) => Ordering::Equal,
			(Self::Math(Math::Mul | Math::Div | Math::Mod), _) => Ordering::Greater,
			(_, Self::Math(Math::Mul | Math::Div | Math::Mod)) => Ordering::Less,

			(Self::Math(Math::Pow), Self::Math(Math::Pow)) => Ordering::Equal,
		}
	}
}

#[derive(Debug)]
pub struct BinaryOperator {
	op: Operator,
	lhs: Box<Expression>,
	rhs: Box<Expression>
}

impl BinaryOperator {
	pub fn parse_with<I>(lhs: Expression, parser: &mut Parser<'_, I>, level: Option<Operator>)
		-> Result<Expression, ParseError>
	where
		I: Iterator<Item=char>
	{
		let op =
			match parser.next_token()? {
				Some(Token::Symbol(Symbol::Equal)) => Operator::Assignment(None),

				Some(Token::Symbol(Symbol::EqualEqual)) => Operator::Logic(Logic::Eql),
				Some(Token::Symbol(Symbol::NotEqual)) => Operator::Logic(Logic::Neq),
				Some(Token::Symbol(Symbol::LessThan)) => Operator::Logic(Logic::Lth),
				Some(Token::Symbol(Symbol::LessThanOrEqual)) => Operator::Logic(Logic::Leq),
				Some(Token::Symbol(Symbol::GreaterThan)) => Operator::Logic(Logic::Gth),
				Some(Token::Symbol(Symbol::GreaterThanOrEqual)) => Operator::Logic(Logic::Geq),
				Some(Token::Symbol(Symbol::Compare)) => Operator::Logic(Logic::Cmp),

				Some(Token::Symbol(Symbol::Plus)) => Operator::Math(Math::Add),
				Some(Token::Symbol(Symbol::Hyphen)) => Operator::Math(Math::Sub),
				Some(Token::Symbol(Symbol::Asterisk)) => Operator::Math(Math::Mul),
				Some(Token::Symbol(Symbol::AsteriskAsterisk)) => Operator::Math(Math::Pow),
				Some(Token::Symbol(Symbol::Solidus)) => Operator::Math(Math::Div),
				Some(Token::Symbol(Symbol::PercentSign)) => Operator::Math(Math::Mod),

				Some(Token::Symbol(Symbol::PlusEqual)) => Operator::Assignment(Some(Math::Add)),
				Some(Token::Symbol(Symbol::HyphenEqual)) => Operator::Assignment(Some(Math::Sub)),
				Some(Token::Symbol(Symbol::AsteriskEqual)) => Operator::Assignment(Some(Math::Mul)),
				Some(Token::Symbol(Symbol::AsteriskAsteriskEqual)) => Operator::Assignment(Some(Math::Pow)),
				Some(Token::Symbol(Symbol::SolidusEqual)) => Operator::Assignment(Some(Math::Div)),
				Some(Token::Symbol(Symbol::PercentSignEqual)) => Operator::Assignment(Some(Math::Mod)),

				Some(Token::Symbol(Symbol::AndAnd)) => Operator::ShortCircuit(ShortCircuit::And),
				Some(Token::Symbol(Symbol::OrOr)) => Operator::ShortCircuit(ShortCircuit::Or),

				Some(_) => {
					parser.undo_next_token();
					return Ok(lhs);
				}
				None => return Ok(lhs)
			};

		if level.map_or(false, |level| level <= op) {
			todo!("actually handle order of operations: {:?}, {:?}", level, op);
			// parser.undo_next_token();
			// return Ok(lhs);
		}

		let rhs_primary = Primary::expect_parse(parser)?;
		let rhs = Self::parse_with(Expression::Primary(rhs_primary), parser, Some(op))?;

		Ok(Expression::BinaryOperator(Self { op, lhs: Box::new(lhs), rhs: Box::new(rhs) }))
	}
}

impl Compilable for BinaryOperator {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let target_ =
			if let Some(target) = target {
				target
			} else if let Operator::Assignment(_) = self.op {
				compiler.next_target()
			} else {
				self.lhs.compile(compiler, None)?;
				self.rhs.compile(compiler, None)?;
				return Ok(());
			};

		match self.op {
			Operator::Assignment(op) => compile_assignment(op, self.lhs, self.rhs, compiler, target),
			Operator::Math(op) => compile_math(op, self.lhs, self.rhs, compiler, target_),
			Operator::Logic(op) => compile_logic(op, self.lhs, self.rhs, compiler, target_),
			Operator::ShortCircuit(op) => compile_short_circuit(op, self.lhs, self.rhs, compiler, target_),
		}
	}
}

fn compile_assignment(op: Option<Math>, lhs: Box<Expression>, rhs: Box<Expression>, compiler: &mut Compiler, target: Option<Target>)
	-> Result<(), CompileError>
{
	match *lhs {
		Expression::Primary(Primary::Identifier(name)) => name.compile_assignment(op, rhs, compiler, target),
		Expression::Primary(Primary::GetAttr(getattr)) => getattr.compile_assignment(op, rhs, compiler, target),
		Expression::Primary(Primary::Index(index)) => index.compile_assignment(op, rhs, compiler, target),
		_ => Err(CompileError::InvalidLhsForAssignment)
	}
}

// TODO: make this take Option<Target>
fn compile_math(op: Math, lhs: Box<Expression>, rhs: Box<Expression>, compiler: &mut Compiler, target: Target)
	-> Result<(), CompileError>
{
	let rhs_target = compiler.next_target();
	lhs.compile(compiler, Some(target))?;
	rhs.compile(compiler, Some(rhs_target))?;

	match op {
		Math::Add => compiler.opcode(Opcode::Add),
		Math::Sub => compiler.opcode(Opcode::Subtract),
		Math::Mul => compiler.opcode(Opcode::Multiply),
		Math::Div => compiler.opcode(Opcode::Divide),
		Math::Mod => compiler.opcode(Opcode::Modulo),
		Math::Pow => compiler.opcode(Opcode::Power),
	}

	compiler.target(target);
	compiler.target(rhs_target);
	compiler.target(target);

	Ok(())
}

fn compile_logic(op: Logic, lhs: Box<Expression>, rhs: Box<Expression>, compiler: &mut Compiler, target: Target)
	-> Result<(), CompileError>
{
	let rhs_target = compiler.next_target();
	lhs.compile(compiler, Some(target))?;
	rhs.compile(compiler, Some(rhs_target))?;

	match op {
		Logic::Eql => compiler.opcode(Opcode::Equals),
		Logic::Neq => compiler.opcode(Opcode::NotEquals),
		Logic::Lth => compiler.opcode(Opcode::LessThan),
		Logic::Leq => compiler.opcode(Opcode::LessThanOrEqual),
		Logic::Gth => compiler.opcode(Opcode::GreaterThan),
		Logic::Geq => compiler.opcode(Opcode::GreaterThanOrEqual),
		Logic::Cmp => compiler.opcode(Opcode::Compare),
	}

	compiler.target(target);
	compiler.target(rhs_target);
	compiler.target(target);

	Ok(())
}

fn compile_short_circuit(
	op: ShortCircuit,
	lhs: Box<Expression>,
	rhs: Box<Expression>,
	compiler: &mut Compiler,
	target: Target
) -> Result<(), CompileError> {

	lhs.compile(compiler, Some(target))?;
	compiler.opcode(if op == ShortCircuit::And { Opcode::JumpIfFalse } else { Opcode::JumpIfTrue });
	compiler.target(target);
	let dst = compiler.defer_jump();

	rhs.compile(compiler, Some(target))?;
	dst.set_jump_to_current(compiler);

	Ok(())
}
