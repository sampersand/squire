use crate::ast::{Expression, Statements, Statement};
use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Keyword, Symbol, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

#[derive(Debug)]
pub struct Fork {
	condition: Expression,
	paths: Vec<Path>,
	alas: Option<Statements>
}

#[derive(Debug)]
struct Path {
	exprs: Vec<Expression>, // should have at least one
	body: Statements

}

impl Fork {
	fn parse_paths<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Path, ParseError> {
		let mut exprs = vec![Expression::expect_parse(parser)?];
		parser.expect(TokenKind::Symbol(Symbol::Colon))?;

		while parser.guard(TokenKind::Keyword(Keyword::Case))?.is_some() {
			exprs.push(Expression::expect_parse(parser)?);
			parser.expect(TokenKind::Symbol(Symbol::Colon))?;
		}

		Ok(Path { exprs, body: parser.collect(Statement::parse)? })
	}

}

impl Parsable for Fork {
	const TYPE_NAME: &'static str = Keyword::Switch.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Switch))?.is_none() {
			return Ok(None);
		}

		let condition = Expression::expect_parse(parser)?;
		parser.expect(TokenKind::LeftParen(ParenKind::Curly))?;

		let mut paths = Vec::new();
		let mut alas = None;

		while parser.guard(TokenKind::RightParen(ParenKind::Curly))?.is_none() {
			match parser.expect([TokenKind::Keyword(Keyword::Case), TokenKind::Keyword(Keyword::Alas)])? {
				Token::Keyword(Keyword::Alas) if alas.is_some() => return Err(parser.error("an `alas` was already given")),
				Token::Keyword(Keyword::Alas) => {
					parser.expect(TokenKind::Symbol(Symbol::Colon))?;
					alas = Some(parser.collect(Statement::parse)?)
				},
				Token::Keyword(Keyword::Case) => paths.push(Self::parse_paths(parser)?),
				_ => unreachable!()
			}
		}

		Ok(Some(Self { condition, paths, alas }))
	}
}

impl Compilable for Fork {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;

		let condition_target = target.unwrap_or_else(|| compiler.next_target());
		let temp_target = compiler.next_target();

		self.condition.compile(compiler, Some(condition_target))?;

		let mut path_locations = Vec::with_capacity(self.paths.len());

		let mut paths_bodies = Vec::with_capacity(self.paths.len());

		for path in self.paths {
			debug_assert!(!path.exprs.is_empty());
			let mut path_jumps = Vec::with_capacity(path.exprs.len());

			for expr in path.exprs {
				expr.compile(compiler, Some(temp_target))?;

				compiler.opcode(Opcode::Equals);
				compiler.target(condition_target);
				compiler.target(temp_target);
				compiler.target(temp_target);
				compiler.opcode(Opcode::JumpIfTrue);
				compiler.target(temp_target);
				path_jumps.push(compiler.defer_jump());
			}

			path_locations.push(path_jumps);
			paths_bodies.push(path.body);
		}

		compiler.opcode(Opcode::Jump);
		let alas_target = compiler.defer_jump();

		let mut jumps_to_end = Vec::with_capacity(paths_bodies.len());

		for (path_body, locations) in paths_bodies.into_iter().zip(path_locations) {
			debug_assert!(!locations.is_empty());

			for location in locations {
				location.set_jump_to_current(compiler);
			}

			path_body.compile(compiler, target)?;
			compiler.opcode(Opcode::Jump);
			jumps_to_end.push(compiler.defer_jump());
		}

		// if we don't have an alas, this will be the same as the end
		alas_target.set_jump_to_current(compiler);
		if let Some(alas) = self.alas {
			alas.compile(compiler, target)?;
		}

		for jump_to_end in jumps_to_end {
			jump_to_end.set_jump_to_current(compiler);
		}

		Ok(())
	}
}
