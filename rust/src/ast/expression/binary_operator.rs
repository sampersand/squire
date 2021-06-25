use crate::parse::{Error as ParseError, Parsable, Parser};
//use crate::parse::token::{/*Token, TokenKind,*/ Keyword};

#[derive(Debug)]
pub struct BinaryOperator {

}

impl Parsable for BinaryOperator {
	const TYPE_NAME: &'static str = "<binary operator>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		// match parser.guard(TokenKind::Identifier)? {
		// 	Token::Identifier(label)
		// }
		let _ = parser; todo!();
	}
}
