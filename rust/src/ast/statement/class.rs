use crate::parse::{Parser, Parsable, Error as ParseError};
use crate::parse::token::{Token, TokenKind, Symbol, Keyword, ParenKind};
use crate::compile::{Compiler, Compilable, Target, Error as CompileError};

use std::collections::HashMap;
use crate::ast::statement::function::Function;
use crate::ast::expression::Expression;
use crate::value::{Value, Form};

#[derive(Debug)]
pub struct Class {
	name: String,

	class_fields: HashMap<String, Option<Expression>>,
	functions: Vec<Function>,

	fields: Vec<String>,
	methods: Vec<Function>,
	constructor: Option<Function>
}

impl Class {
	fn parse_class_field<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let class_fields =
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

		// todo: error on duplicate field names

		for (name, init) in class_fields {
			if self.class_fields.contains_key(&name) {
				return Err(parser.error("class field already declared"));
			} else {
				self.class_fields.insert(name, init)
			};
		}

		Ok(())

	}

	fn parse_class_function<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		self.functions.push(Function::parse_without_keyword(parser)?);

		Ok(())
	}

	fn parse_field<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let fields =
			parser.take_separated(
				TokenKind::Symbol(Symbol::Comma), 
				TokenKind::Symbol(Symbol::Endline),
				Parser::expect_identifier
			)?;


		for name in fields {
			if self.fields.contains(&name) {
				return Err(parser.error("field already declared"));
			} else {
				self.fields.push(name);
			}
		}

		Ok(())
	}

	fn parse_method<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let _ = parser; todo!();
	}

	fn parse_constructor<I: Iterator<Item=char>>(&mut self, parser: &mut Parser<'_, I>) -> Result<(), ParseError> {
		let _ = parser; todo!();
	}
}

impl Parsable for Class {
	const TYPE_NAME: &'static str = Keyword::Class.repr();

	fn parse<I: Iterator<Item=char>>(parser: &mut Parser<'_, I>) -> Result<Option<Self>, ParseError> {
		if parser.guard(TokenKind::Keyword(Keyword::Class))?.is_none() {
			return Ok(None)
		}

		let mut class = Self {
			name: parser.expect_identifier()?,
			class_fields: HashMap::default(),
			functions: Vec::default(),
			fields: Vec::default(),
			methods: Vec::default(),
			constructor: None
		};

		parser.take_paren_group(ParenKind::Curly, |parser| {
			const VALID_TOKENS: [TokenKind; 6] = [
				TokenKind::Keyword(Keyword::ClassField),
				TokenKind::Keyword(Keyword::ClassFn),
				TokenKind::Keyword(Keyword::Field),
				TokenKind::Keyword(Keyword::Method),
				TokenKind::Keyword(Keyword::Constructor),
				TokenKind::Symbol(Symbol::Endline),
			];

			match parser.expect(VALID_TOKENS)? {
				Token::Keyword(Keyword::ClassField) => class.parse_class_field(parser),
				Token::Keyword(Keyword::ClassFn) => class.parse_class_function(parser),
				Token::Keyword(Keyword::Field) => class.parse_field(parser),
				Token::Keyword(Keyword::Method) => class.parse_method(parser),
				Token::Keyword(Keyword::Constructor) => class.parse_constructor(parser),
				Token::Symbol(Symbol::Endline) => Ok(()),
				_ => unreachable!()
			}
		})?;

		Ok(Some(class))
	}
}

impl Compilable for Class {
	fn compile(self, compiler: &mut Compiler, target: Option<Target>) -> Result<(), CompileError> {
		use crate::runtime::Opcode;

		let mut builder = Form::builder(self.name);

		let globals = compiler.globals();

		for func in self.functions {
			builder.add_recall(func.build_journey(globals.clone())?)?;
		}

		let form = builder.build();
		let global = compiler.define_global(form.name().to_string(), Some(Value::Form(form.into())))?;

		if let Some(target) = target {
			compiler.opcode(Opcode::LoadGlobal);
			compiler.global(global);
			compiler.target(target);
		}

		Ok(())
	}
}
