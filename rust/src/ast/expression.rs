use crate::parse::{Error as ParseError, Parsable, Parser};

mod primary;
mod binary_operator;

pub use primary::Primary;
pub use binary_operator::BinaryOperator;

#[derive(Debug)]
pub enum Expression {
	Primary(Primary),
	BinaryOperator(BinaryOperator),
}

impl Parsable for Expression {
	const TYPE_NAME: &'static str = "<expression>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if let Some(primary) = Primary::parse(parser)? {
			BinaryOperator::parse_with(Expression::Primary(primary), parser, None).map(Some)
		} else {
			Ok(None)
		}
	}
}
