use crate::parse::{Error as ParseError, Parsable, Parser};

mod grouping;
mod literal;
mod lambda;
mod identifier;
mod index;
mod getattr;
mod array;
mod codex;
mod unary_op;
mod function_call;

pub use grouping::Grouping;
pub use literal::Literal;
pub use lambda::Lambda;
pub use identifier::Identifier;
pub use index::Index;
pub use getattr::GetAttr;
pub use array::Array;
pub use codex::Codex;
pub use unary_op::UnaryOp;
pub use function_call::FunctionCall;

#[derive(Debug)]
pub enum Primary {
	Grouping(Grouping),
	Literal(Literal),
	Lambda(Lambda),
	Identifier(Identifier),
	Array(Array),
	Codex(Codex),
	UnaryOp(UnaryOp),

	Index(Index),
	GetAttr(GetAttr),
	FunctionCall(FunctionCall),
}


impl Parsable for Primary {
	const TYPE_NAME: &'static str = "<primary>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		macro_rules! try_parse {
			($($name:ident),*) => (
				$(if let Some(token) = $name::parse(parser)? {
					Self::$name(token)
				} else)* {
					return Ok(None);
				}
			);
		}

		let mut primary = try_parse!(Literal, Lambda, Identifier, Array, Codex, UnaryOp, Grouping);

		loop {
			match Index::parse_with(primary, parser)? {
				Ok(index) => primary = Self::Index(index),
				Err(primary_) => match GetAttr::parse_with(primary_, parser)? {
					Ok(getattr) => primary = Self::GetAttr(getattr),
					Err(primary_) => match FunctionCall::parse_with(primary_, parser)? {
						Ok(function_call) => primary = Self::FunctionCall(function_call),
						Err(primary_) => return Ok(Some(primary_))
					}
				}
			}
		}

	}
}
