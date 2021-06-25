use crate::parse::{Error as ParseError, Parsable, Parser};
//use crate::parse::token::{/*Token, TokenKind,*/ Keyword};

#[derive(Debug)]
pub struct Assignment {

}

impl Parsable for Assignment {
	const TYPE_NAME: &'static str = "<assignment>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		// match parser.guard(TokenKind::Identifier)? {
		// 	Token::Identifier(label)
		// }
		let _ = parser; todo!();
	}
}
