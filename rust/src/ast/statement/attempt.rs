use crate::ast::Statements;
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{TokenKind, Keyword};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};


#[derive(Debug)]
pub struct Attempt {
	body: Statements,

	exception: String,
	catch: Statements,

	finally: Option<Statements>
}

impl Parsable for Attempt {
	const TYPE_NAME: &'static str = Keyword::Try.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Try))?.is_none() {
			return Ok(None);
		}

		let body = Statements::expect_parse(parser)?;

		parser.expect(TokenKind::Keyword(Keyword::Alas))?;
		let exception = parser.expect_identifier()?;
		let catch = Statements::expect_parse(parser)?;

		let finally =
			if parser.guard(TokenKind::Keyword(Keyword::Finally))?.is_some() {
				Some(Statements::expect_parse(parser)?)
			} else {
				None
			};

		Ok(Some(Self { body, exception, catch, finally }))
	}
}

impl Compilable for Attempt {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let _ = (compiler, target); todo!();
	}
}
