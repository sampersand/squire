use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::ast::Expression;
use crate::parse::token::{Token, Symbol};
use std::cmp::Ordering;

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
	pub fn parse_with<I>(
		lhs: Expression,
		parser: &mut Parser<'_, I>,
		level: Option<Operator>
	)
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

		if level.map_or(false, |level| level < op) {
			parser.undo_next_token();
			return Ok(lhs);
		}

		let rhs_primary = super::Primary::expect_parse(parser)?;
		let rhs = Self::parse_with(Expression::Primary(rhs_primary), parser, Some(op))?;

		Ok(Expression::BinaryOperator(Self { op, lhs: Box::new(lhs), rhs: Box::new(rhs) }))
	}
}
