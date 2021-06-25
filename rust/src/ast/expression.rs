use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Literal};


#[derive(Debug)]
pub enum Primary {

}

#[derive(Debug)]
pub enum Expression {
	Literal(Literal)
// struct expression {
// 	enum {
// 		SQ_PS_EFNCALL,
// 		SQ_PS_EASSIGN,
// 		SQ_PS_EARRAY_ASSIGN,
// 		SQ_PS_EMATH
// 	} kind;

// 	union {
// 		struct function_call *fncall;
// 		struct assignment *asgn;
// 		struct index_assign *ary_asgn;
// 		struct bool_expression *math;
// 	};
// };

}

impl Parsable for Expression {
	const TYPE_NAME: &'static str = "Expression";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		 // todo
		if let Some(literal) = parser.guard(TokenKind::Literal)? {
			if let Token::Literal(literal) = literal {
				Ok(Some(Self::Literal(literal)))
			} else {
				unreachable!()
			}
		} else {
			Ok(None)
		}
	}
}


impl Parsable for Primary {
	const TYPE_NAME: &'static str = "<primary>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		 // todo
		 todo!()
		// if let Some(literal) = parser.guard(TokenKind::Literal)? {
		// 	if let Token::Literal(literal) = literal {
		// 		Ok(Some(Self::Literal(literal)))
		// 	} else {
		// 		unreachable!()
		// 	}
		// } else {
		// 	Ok(None)
		// }
	}
}
