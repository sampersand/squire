use crate::ast::{Statements, Statement};
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::ParenKind;

#[derive(Debug)]
pub struct Grouping(Statements);

impl Parsable for Grouping {
	const TYPE_NAME: &'static str = "<grouping>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		Ok(parser.take_paren_group(ParenKind::Round, Statement::expect_parse)?.map(Self))
	}
}
