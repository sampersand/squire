use crate::ast::{Expression, expression::Primary, Statements};
use crate::parse::{Error as ParseError, Parsable, Parser};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, ParenKind};

pub type Type = Primary;

#[derive(Debug)]
pub struct Argument {
	name: String,
	kind: Option<Type>,
	default: Option<Expression>
}

#[derive(Debug)]
pub struct Arguments {
	normal: Vec<Argument>,
	vararg: Option<String>,
	varkwarg: Option<String>,
	return_type: Option<Type>
}

#[derive(Debug)]
pub struct Function {
	name: String,
	args: Arguments,
	body: Statements
}

impl Parsable for Arguments {
	const TYPE_NAME: &'static str = "<arguments>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Round))?.is_none() {
			return Ok(None);
		}

		let mut arguments = Self {
			normal: Vec::new(),
			vararg: None,
			varkwarg: None,
			return_type: None
		};

		while let Some(name) = parser.guard_identifier()? {
			let mut argument = Argument { name, kind: None, default: None };

			if parser.guard(TokenKind::Symbol(Symbol::Colon))?.is_some() {
				argument.kind = Some(Type::expect_parse(parser)?);
			}

			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				argument.default = Some(Expression::expect_parse(parser)?);
			}

			arguments.normal.push(argument);

			if parser.guard(TokenKind::Symbol(Symbol::Comma))?.is_none() {
				break;
			}
		}

		if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
			arguments.vararg = Some(parser.expect_identifier()?);

			if let Token::RightParen(_) = parser.expect([
				TokenKind::Symbol(Symbol::Comma),
				TokenKind::RightParen(ParenKind::Round)
			])? {
				parser.undo_next_token();
			}
		}

		if parser.guard(TokenKind::Symbol(Symbol::AsteriskAsterisk))?.is_some() {
			arguments.varkwarg = Some(parser.expect_identifier()?);

			parser.guard(TokenKind::Symbol(Symbol::Comma))?;
		}

		parser.expect(TokenKind::RightParen(ParenKind::Round))?;

		if parser.guard(TokenKind::Symbol(Symbol::Colon))?.is_some() {
			arguments.return_type = Some(Type::expect_parse(parser)?);
		}

		Ok(Some(arguments))
	}
}

impl Parsable for Function {
	const TYPE_NAME: &'static str = Keyword::Function.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Function))?.is_none() {
			return Ok(None);
		}

		let name = parser.expect_identifier()?;
		let args = Arguments::expect_parse(parser)?;
		let body  = Statements::expect_parse(parser)?;

		Ok(Some(Self { name, args, body }))
	}
}
