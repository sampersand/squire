use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Symbol, Keyword, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};
use squire_runtime::vm::Opcode;

use std::collections::HashMap;
use crate::ast::statement::journey::Journey;
use crate::ast::expression::Expression;
use squire_runtime::value::Value;

#[derive(Debug)]
pub struct Form {
	name: String,
	parents: Vec<String>,

	essences: HashMap<String, Option<Expression>>,
	functions: Vec<Journey>,

	matter_names: Vec<String>,
	changes: Vec<Journey>,
	imitate: Option<Journey>
}

impl Form {
	fn parse_essence<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let essences =
			parser.take_separated(
				TokenKind::Symbol(Symbol::Comma), 
				TokenKind::Symbol(Symbol::Endline),
				|parser| {
					let name = parser.expect_identifier()?;

					if parser.guard(TokenKind::Symbol(Symbol::Equal))?.is_some() {
						Ok((name, Some(Expression::expect_parse(parser)?)))
					} else {
						Ok((name, None))
					}
				}
			)?;

		// unimplemented: error on duplicate field names

		for (name, init) in essences {
			if self.essences.contains_key(&name) {
				return Err(parser.error("class field already declared"));
			} else {
				self.essences.insert(name, init)
			};
		}

		Ok(())

	}

	fn parse_recall<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let name = parser.expect_identifier()?;
		self.functions.push(Journey::parse_without_keyword(parser, Some(name), false, true)?);

		Ok(())
	}

	fn parse_matter<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let matter_names =
			parser.take_separated(
				TokenKind::Symbol(Symbol::Comma), 
				TokenKind::Symbol(Symbol::Endline),
				Parser::expect_identifier
			)?;


		for name in matter_names {
			if self.matter_names.contains(&name) {
				return Err(parser.error("field already declared"));
			} else {
				self.matter_names.push(name);
			}
		}

		Ok(())
	}

	fn parse_change<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let name = parser.expect_identifier_or_operator()?;
		Ok(self.changes.push(Journey::parse_without_keyword(parser, Some(name), true, true)?))
	}

	fn parse_imitate<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		if self.imitate.is_some() {
			Err(parser.error("cannot define two 'imitate's"))
		} else {
			self.imitate = Some(Journey::parse_without_keyword(parser, Some("imitate".to_string()), true, true)?);
			Ok(())
		}
	}
}

impl Parsable for Form {
	const TYPE_NAME: &'static str = Keyword::Form.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Form))?.is_none() {
			return Ok(None)
		}

		let mut class = Self {
			name: parser.expect_identifier()?,
			parents :Vec::default(),
			essences: HashMap::default(),
			functions: Vec::default(),
			matter_names: Vec::default(),
			changes: Vec::default(),
			imitate: None
		};

		// if a parents list is given, parse them.
		if parser.guard(TokenKind::Symbol(Symbol::Colon))?.is_some() {
			loop {
				match parser.guard([TokenKind::Identifier, TokenKind::LeftParen(ParenKind::Curly)])? {
					Some(Token::Identifier(parent)) => {
						class.parents.push(parent);

						match parser.expect([TokenKind::Symbol(Symbol::Comma), TokenKind::LeftParen(ParenKind::Curly)])? {
							Token::Symbol(Symbol::Comma) => continue,
							Token::LeftParen(ParenKind::Curly) => {
								parser.undo_next_token();
								break;
							},
							_ => unreachable!()
						}
					},
					Some(Token::LeftParen(ParenKind::Curly)) => {
						parser.undo_next_token();
						break;
					},
					Some(_) => unreachable!(),
					None => break
				}
			}
		}

		if parser.take_paren_group(ParenKind::Curly, |parser| {
			const VALID_TOKENS: [TokenKind; 6] = [
				TokenKind::Keyword(Keyword::Essence),
				TokenKind::Keyword(Keyword::Recall),
				TokenKind::Keyword(Keyword::Matter),
				TokenKind::Keyword(Keyword::Change),
				TokenKind::Keyword(Keyword::Imitate),
				TokenKind::Symbol(Symbol::Endline),
			];

			match parser.expect(VALID_TOKENS)? {
				Token::Keyword(Keyword::Essence) => class.parse_essence(parser),
				Token::Keyword(Keyword::Recall) => class.parse_recall(parser),
				Token::Keyword(Keyword::Matter) => class.parse_matter(parser),
				Token::Keyword(Keyword::Change) => class.parse_change(parser),
				Token::Keyword(Keyword::Imitate) => class.parse_imitate(parser),
				Token::Symbol(Symbol::Endline) => Ok(()),
				_ => unreachable!()
			}
		})?.is_none() {
			return Err(parser.error("expected a `{` after from declaration"));
		}

		Ok(Some(class))
	}
}

impl Compilable for Form {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		let mut builder = squire_runtime::value::Form::builder(self.name);

		let globals = compiler.globals();

		for parent in self.parents {
			match globals.borrow().get(&parent) {
				Some((_, Some(Value::Form(parent)))) => builder.add_parent(parent.clone()),
				Some((_, Some(other))) => return Err(CompileError::ParentNotAForm(other.clone())),
				_ => return Err(CompileError::ParentNotDeclared(parent)),
			}
		}

		for func in self.functions {
			builder.add_recall(func.build_journey(globals.clone())?)?;
		}

		let mut essence_initializers = Vec::new();
		for (name, initializer) in self.essences {
			builder.add_essence(name.clone())?;

			if let Some(initializer) = initializer {
				essence_initializers.push((name, initializer));
			}
		}

		for matter_name in self.matter_names {
			builder.add_matter(matter_name)?;
		}

		for change in self.changes {
			builder.add_change(change.build_journey(globals.clone())?)?;
		}


		if let Some(imitate) = self.imitate {
			builder.add_imitate(imitate.build_journey(globals.clone())?)?;
		}

		let form = builder.build();
		let global = compiler.define_global(form.name().to_string(), Some(Value::Form(form.into())))?;

		// if we have neither matter_names to initialize nor a destination, we're done.
		if essence_initializers.is_empty() && target.is_none() {
			return Ok(());
		}

		let target = 
			if essence_initializers.is_empty() {
				target.unwrap_or(Compiler::SCRATCH_TARGET)
			} else {
				target.unwrap_or_else(|| compiler.next_target())
			};

		compiler.opcode(Opcode::LoadGlobal);
		compiler.global(global);
		compiler.target(target);

		for (name, initializer) in essence_initializers {
			initializer.compile(compiler, Some(Compiler::SCRATCH_TARGET))?;
			let name_constant = compiler.get_constant(name.into());

			compiler.opcode(Opcode::SetAttribute);
			compiler.target(target);
			compiler.constant(name_constant);
			compiler.target(Compiler::SCRATCH_TARGET);
		}

		Ok(())
	}
}
