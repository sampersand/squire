use super::GenusDeclaration;
use crate::ast::{Expression, Statement, Statements};
use crate::value::{Value, journey::UserDefined};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Globals, Error as CompileError};
use crate::runtime::Opcode;

#[derive(Debug)]
pub struct Argument {
	name: String,
	genus: Option<GenusDeclaration>,
	default: Option<Expression>
}

#[derive(Debug)]
pub struct Arguments {
	normal: Vec<Argument>,
	vararg: Option<String>,
	varkwarg: Option<String>,
	return_genus: Option<GenusDeclaration>
}

#[derive(Debug)]
pub struct Journey {
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
			return_genus: None
		};

		while let Some(name) = parser.guard_identifier()? {
			arguments.normal.push(Argument {
				name,
				genus: GenusDeclaration::parse(parser)?,
				default: 
					if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
						Some(Expression::expect_parse(parser)?)
					} else {
						None
					}
			});

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

		arguments.return_genus = GenusDeclaration::parse(parser)?;

		Ok(Some(arguments))
	}
}

impl Parsable for Journey {
	const TYPE_NAME: &'static str = Keyword::Journey.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Journey))?.is_none() {
			return Ok(None);
		}

		let name = parser.expect_identifier()?;
		Self::parse_without_keyword(parser, name).map(Some)
	}
}

impl Journey {
	pub fn parse_without_keyword<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>, name: String) -> Result<Self, ParseError> {
		let args = Arguments::expect_parse(parser)?;

		let body =
			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				vec![Statement::Expression(Expression::expect_parse(parser)?)]
			} else {
				Statements::expect_parse(parser)?
			};

		Ok(Self { name, args, body })
	}

	pub fn build_journey(mut self, globals: Globals, is_method: bool) -> Result<UserDefined, CompileError> {
		let mut body_compiler = Compiler::with_globals(globals);

		if is_method {
			self.args.normal.insert(0, Argument { name: "soul".into(), genus: None, default: None });
		}

		if self.args.vararg.is_some() || self.args.varkwarg.is_some() {
			todo!();
		}

		let mut arg_names = Vec::new();
		for arg in self.args.normal {
			arg_names.push(arg.name.clone());
			let local = body_compiler.define_local(arg.name);

			if let Some(genus) = arg.genus {
				genus.check(local, &mut body_compiler)?;
			}

			if arg.default.is_some() {
				todo!();
			}
		}

		let return_target = body_compiler.next_target();
		self.body.compile(&mut body_compiler, Some(return_target))?;

		if let Some(return_genus) = self.args.return_genus {
			return_genus.check(return_target, &mut body_compiler)?;
		}

		// todo: what if `return_target` is just used for scratch?.
		body_compiler.opcode(Opcode::Return);
		body_compiler.target(return_target);

		Ok(crate::value::journey::UserDefined::new(self.name.clone(), is_method, arg_names, body_compiler.finish()))
	}
}

impl Compilable for Journey {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let name = self.name.clone();
		let journey = Value::Journey(self.build_journey(compiler.globals().clone(), false)?.into());
		let global = compiler.define_global(name, Some(journey))?;

		if let Some(target) = target {
			compiler.opcode(Opcode::LoadGlobal);
			compiler.global(global);
			compiler.target(target);
		}

		Ok(())
	}
}
