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
		use crate::runtime::Opcode;

		compiler.opcode(Opcode::Attempt);
		let set_handler = compiler.defer_jump();
		let error = compiler.define_local(self.exception);
		compiler.target(error);

		for statement in self.body {
			statement.compile(compiler, target)?;
		}

		compiler.opcode(Opcode::PopHandler);
		compiler.opcode(Opcode::Jump);
		let end_of_handler = compiler.defer_jump();
		set_handler.set_jump_to_current(compiler);

		for statement in self.catch {
			statement.compile(compiler, target)?;
		}


		end_of_handler.set_jump_to_current(compiler);

		Ok(())
	}
}
