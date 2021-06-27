pub mod statement;
pub mod expression;
pub use statement::{Statement, Statements};
pub use expression::Expression;

use crate::parse::{Parser, Parsable, Error as ParseError};

pub fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Statements, ParseError> {
	parser.collect(Statement::parse)
}
