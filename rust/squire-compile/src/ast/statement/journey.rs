use super::GenusDeclaration;
use crate::ast::{Expression, Statement, Statements};
use squire_runtime::value::{Value, journey::UserDefined};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, Literal, LiteralKind, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Globals, Error as CompileError};
use squire_runtime::vm::Opcode;

#[derive(Debug)]
pub struct ArgumentPattern {
	name: String,
	genus: Option<GenusDeclaration>,
	default: Option<Expression>
}

#[derive(Debug)]
enum SplatKind {
	Named(String),
	Ignored, // simply ignore extra arguments
	None
}

#[derive(Debug)]
pub struct Arguments {
	positional: Vec<ArgumentPattern>,
	splat: SplatKind,

	keyword_only: Vec<ArgumentPattern>,
	splatsplat: SplatKind,

	return_genus: Option<GenusDeclaration>
}

#[derive(Debug)]
pub struct Journey {
	name: String,
	patterns: Vec<Pattern>
}

#[derive(Debug)]
struct Pattern {
	args: Arguments,
	guard: Option<Expression>,
	body: Statements
}

impl Parsable for ArgumentPattern {
	const TYPE_NAME: &'static str = "<argument pattern>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let name =
			if let Some(name) = parser.guard_identifier()? {
				name
			} else {
				return Ok(None)
			};

		let genus = GenusDeclaration::parse(parser)?;
		let default =
			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				Some(Expression::expect_parse(parser)?)
			} else {
				None
			};

		Ok(Some(Self { name, genus, default }))
	}
}

impl Parsable for Arguments {
	const TYPE_NAME: &'static str = "<arguments>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::LeftParen(ParenKind::Round))?.is_none() {
			return Ok(None);
		}

		let mut arguments = Self {
			positional: Vec::new(),
			keyword_only: Vec::new(),
			splat: SplatKind::None,
			splatsplat: SplatKind::None,
			return_genus: None
		};

		// lol no gotos, silly.
		'done: loop {
			// check for positional and default arguments.
			while let Some(normal) = ArgumentPattern::parse(parser)? {
				arguments.positional.push(normal);

				if !parser.guard_comma()? {
					break 'done;
				}
			}

			// check for splat argument
			if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
				arguments.splat =
					match parser.guard([TokenKind::Literal(LiteralKind::Ni), TokenKind::Identifier])? {
						Some(Token::Literal(Literal::Ni)) => SplatKind::None,
						Some(Token::Identifier(name)) => SplatKind::Ignored,
						Some(_) => unreachable!(),
						None => SplatKind::Ignored, // defaults to empty string
					};

				if !parser.guard_comma()? {
					break 'done;
				}
			}

			// check for keyword-only arguments.
			while let Some(normal) = ArgumentPattern::parse(parser)? {
				arguments.keyword_only.push(normal);

				if !parser.guard_comma()? {
					break 'done;
				}
			}

			// check for splat argument
			if parser.guard(TokenKind::Symbol(Symbol::Asterisk))?.is_some() {
				arguments.splatsplat =
					match parser.guard([TokenKind::Literal(LiteralKind::Ni), TokenKind::Identifier])? {
						Some(Token::Literal(Literal::Ni)) => SplatKind::None,
						Some(Token::Identifier(name)) => SplatKind::Ignored,
						Some(_) => unreachable!(),
						None => SplatKind::Ignored, // defaults to empty string
					};

				let _ = parser.guard_comma()?;
			}

			break 'done;
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

impl Parsable for Pattern {
	const TYPE_NAME: &'static str = "<pattern>";

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		let args = 
			if let Some(args) = Arguments::parse(parser)? {
				args
			} else {
				return Ok(None);
			};

		let guard =
			if parser.guard(TokenKind::Keyword(Keyword::If))?.is_some() {
				Some(Expression::expect_parse(parser)?)
			} else {
				None
			};

		let body = 
			if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
				vec![Statement::Expression(Expression::expect_parse(parser)?)]
			} else {
				Statements::expect_parse(parser)?
			};

		Ok(Some(Self { args, guard, body }))
	}
}

impl Journey {
	pub fn parse_without_keyword<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>, name: String) -> Result<Self, ParseError> {
		let mut patterns = Vec::new();

		while let Some(pattern) = Pattern::parse(parser)? {
			patterns.push(pattern);

			if parser.guard(TokenKind::Symbol(Symbol::Comma))?.is_none() {
				break;
			}
		}

		if patterns.is_empty() {
			return Err(parser.error("no patterns given"));
		}

		Ok(Self { name, patterns })
	}

	pub fn build_journey(mut self, globals: Globals, is_method: bool) -> Result<UserDefined, CompileError> {
		let mut body_compiler = Compiler::with_globals(globals);

		if is_method {
			for pattern in &mut self.patterns {
				pattern.args.normal.insert(0, ArgumentPattern { name: "soul".into(), genus: None, default: None });

				if pattern.args.vararg.is_some() || pattern.args.varkwarg.is_some() {
					todo!();
				}
			}
		}

		let mut arg_names = Vec::new();
		let last_pattern = self.patterns.pop().unwrap(); // this is here as a stopgap until we get multiple patterns.

		for arg in last_pattern.args.normal {
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
		last_pattern.body.compile(&mut body_compiler, Some(return_target))?;

		if let Some(return_genus) = last_pattern.args.return_genus {
			return_genus.check(return_target, &mut body_compiler)?;
		}

		// todo: what if `return_target` is just used for scratch?.
		body_compiler.opcode(Opcode::Return);
		body_compiler.target(return_target);

		Ok(squire_runtime::value::journey::UserDefined::new(self.name.clone(), is_method, arg_names, body_compiler.finish()))
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
