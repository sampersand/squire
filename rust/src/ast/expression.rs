use crate::parse::{Error as ParseError, Parsable, Parser};

mod primary;
mod assignment;
mod binary_operator;

pub use primary::Primary;
pub use assignment::Assignment;
pub use binary_operator::BinaryOperator;

#[derive(Debug)]
pub enum Expression {
	Primary(Primary),
	Assignment(Assignment),
	BinaryOperator(BinaryOperator),
}

impl Parsable for Expression {
	const TYPE_NAME: &'static str = "<expression>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let primary = 
			if let Some(primary) = Primary::parse(parser)? {
				primary
			} else {
				return Ok(None);
			};


			// todo
		Ok(Some(Self::Primary(primary)))
		// if let Some(assignment) = Parser
	}
}
