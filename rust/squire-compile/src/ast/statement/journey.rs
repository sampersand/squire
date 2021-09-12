use super::GenusDeclaration;
use crate::ast::{Expression, Statement, Statements};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Globals, Error as CompileError};

use squire_runtime::value::{Value, journey::UserDefined};
use squire_runtime::vm::Opcode;

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
	is_method: bool,
	patterns: Vec<Pattern>
}

#[derive(Debug)]
struct Pattern {
	args: Arguments,
	guard: Option<Expression>,
	return_genus: Option<GenusDeclaration>,
	body: Statements,
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
					parser
						.guard(TokenKind::Symbol(Symbol::Equal))?
						.map(|_| Expression::expect_parse(parser))
						.transpose()?
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
		Self::parse_without_keyword(parser, name, true).map(Some)
	}
}

impl Journey {
	pub fn parse_without_keyword<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>, name: String, is_method: bool) -> Result<Self, ParseError> {
		let mut patterns = Vec::new();

		loop {
			let args = Arguments::expect_parse(parser)?;
			let body =
				if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
					vec![Statement::Expression(Expression::expect_parse(parser)?)]
				} else {
					Statements::expect_parse(parser)?
				};

			patterns.push(Pattern { args, body, guard: None, return_genus: None });

			if parser.guard(TokenKind::Symbol(Symbol::Comma))?.is_none() {
				break;
			}
		};

		Ok(Self { name, patterns, is_method })
	}
}

fn compile_check(
	args: Arguments,
	_guard: Option<Expression>,
	check_target: Target,
	is_method: bool,
	compiler: &mut Compiler,
) -> Result<(), CompileError> {
	if is_method {
		compiler.define_local("soul".to_string());
	}

	for arg in args.normal {
		let local = compiler.define_local(arg.name.to_string());

		if let Some(genus) = arg.genus {
			genus.check(local, compiler)?;
		}

		if arg.default.is_some() {
			todo!();
		}

		compiler.opcode(Opcode::LoadConstant);
		let t = compiler.get_constant(true.into());
		compiler.constant(t);
		compiler.target(check_target);
	}

	Ok(())
}

impl Journey {
	pub fn build_journey(self, globals: Globals) -> Result<UserDefined, CompileError> {
		let mut compiler = Compiler::with_globals(globals);
		let mut pattern_bodies = Vec::with_capacity(self.patterns.len());

		let check_target = compiler.temp_target();

		// check for patterns
		for Pattern { args, guard, return_genus, body } in self.patterns {
			compile_check(args, guard, check_target, self.is_method, &mut compiler)?;

			compiler.opcode(Opcode::JumpIfTrue);
			compiler.target(check_target);
			let body_dst = compiler.defer_jump();

			pattern_bodies.push((body, return_genus, body_dst));
		}

		// if nothing matches, throw an error.
		let no_patterns_found = compiler.get_constant("No patterns matched".to_string().into());
		compiler.opcode(Opcode::Throw);
		compiler.constant(no_patterns_found);


		// setup the genus bodies
		let return_target = compiler.next_target();

		for (body, return_genus, body_dst) in pattern_bodies {
			body_dst.set_jump_to_current(&mut compiler);
			body.compile(&mut compiler, Some(return_target))?;

			// if there's a return type given, check it.
			if let Some(return_genus) = return_genus {
				let genus_target = compiler.temp_target();

				return_genus.compile(&mut compiler, Some(genus_target))?;

				compiler.opcode(Opcode::CheckMatches);
				compiler.target(return_target);
				compiler.target(genus_target);
			}

			compiler.opcode(Opcode::Return);
			compiler.target(return_target);
		}

		#[cfg(feature="debug_assertions")]
		compiler.opcode(Opcode::Illegal);

		Ok(UserDefined::new(self.name.clone(), self.is_method, vec!["todo".into()], compiler.finish()))
	}
}

impl Compilable for Journey {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let name = self.name.clone();
		let journey = Value::Journey(self.build_journey(compiler.globals().clone())?.into());
		let global = compiler.define_global(name, Some(journey))?;

		if let Some(target) = target {
			compiler.opcode(Opcode::LoadGlobal);
			compiler.global(global);
			compiler.target(target);
		}

		Ok(())
	}
}
