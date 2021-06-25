use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{/*Token, TokenKind,*/ Keyword};

#[derive(Debug)]
pub struct ComeFrom {

}

#[derive(Debug)]
pub struct Label {

}

impl Parsable for Label {
	const TYPE_NAME: &'static str = "<label>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		// match parser.guard(TokenKind::Identifier)? {
		// 	Token::Identifier(label)
		// }
		let _ = parser; todo!();
	}
}

impl Parsable for ComeFrom {
	const TYPE_NAME: &'static str = Keyword::ComeFrom.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let _ = parser; todo!();
	}
}
