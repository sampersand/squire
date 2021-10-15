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
		Self::parse_without_keyword(parser, Some(name), false, true).map(Some)
	}
}

impl Journey {
	pub fn parse_without_keyword<I: Iterator<Item=char>>(
		parser: &mut Parser<'_, I>,
		name: Option<String>,
		is_method: bool,
		include_patterns: bool
	) -> Result<Self, ParseError> {
		let mut patterns = Vec::new();

		loop {
			let args = Arguments::expect_parse(parser)?;
			let body =
				if parser.guard(TokenKind::Symbol(Symbol::Arrow))?.is_some() {
					vec![Statement::Expression(Expression::expect_parse(parser)?)]
				} else {
					Statements::expect_parse(parser)?
				};

			patterns.push(Pattern { args, body, guard: None, return_genus: None });

			if !include_patterns || parser.guard(TokenKind::Symbol(Symbol::Comma))?.is_none() {
				break;
			}
		};

		Ok(Self {
			name: name.unwrap_or_else(|| "<lambda>".to_string()),
			patterns,
			is_method
		})
	}
}


impl Journey {
	pub fn build_journey(self, globals: Globals) -> Result<UserDefined, CompileError> {
		let mut compiler = Compiler::with_globals(globals);
		let mut pattern_bodies = Vec::with_capacity(self.patterns.len());

		let check_target = compiler.temp_target();

		if self.is_method {
			compiler.define_local("soul".to_string());
		}

		for Pattern { ref args, .. } in self.patterns.iter() {
			for arg in args.normal.iter() {
				compiler.define_local(arg.name.to_string());
			}
		}

		// check for patterns
		for Pattern { args, guard, return_genus, body } in self.patterns {
			let mut jumps = Vec::new();

			for arg in args.normal {
				let local = compiler.define_local(arg.name.to_string());

				if let Some(genus) = arg.genus {
					genus.check(local, check_target, &mut compiler)?;
					compiler.opcode(Opcode::JumpIfFalse);
					compiler.target(check_target);
					jumps.push(compiler.defer_jump());
				}

				if arg.default.is_some() {
					todo!();
				}
			}

			if let Some(guard) = guard {
				let guard_target = compiler.next_target();
				guard.compile(&mut compiler, Some(guard_target))?;

				compiler.opcode(Opcode::JumpIfFalse);
				compiler.target(guard_target);
				jumps.push(compiler.defer_jump());
			}

			compiler.opcode(Opcode::Jump);
			let jump_dst = compiler.defer_jump();
			pattern_bodies.push((body, return_genus, jump_dst));

			for target in jumps {
				target.set_jump_to_current(&mut compiler);
			}
		}

		// if nothing matches, throw an error.
		let no_patterns_found = compiler.get_constant("No patterns matched".to_string().into());
		compiler.opcode(Opcode::LoadConstant);
		compiler.constant(no_patterns_found);
		compiler.target(Compiler::SCRATCH_TARGET);
		compiler.opcode(Opcode::Throw);
		compiler.target(Compiler::SCRATCH_TARGET);


		// setup the genus bodies
		let return_target = compiler.next_target();

		for (body, return_genus, body_dst) in pattern_bodies {
			body_dst.set_jump_to_current(&mut compiler);
			body.compile(&mut compiler, Some(return_target))?;

			// if there's a return type given, check it.
			// ^^ note this doesn't actually check for early returns.
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

		#[cfg(debug_assertions)]
		compiler.illegal();

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
